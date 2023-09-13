# Python `logging` module

These are my notes on how the builtin `logging` module in Python works.

## Links

[Implementation of `logging`](https://github.com/python/cpython/tree/main/Lib/logging)

[Documentation for `logging`](https://docs.python.org/3/library/logging.html)

## Notes on API and implementation

The user creates logs through
[`Logger`](https://github.com/python/cpython/blob/d12b3e3152b1858e91c4890d0bf3a3b574a3ff6f/Lib/logging/__init__.py#L1485)
objects

The client should always create `Logger` instances through the
[`getLogger()`](https://github.com/python/cpython/blob/d12b3e3152b1858e91c4890d0bf3a3b574a3ff6f/Lib/logging/__init__.py#L2163)
function.

Each Logger has a unique name. Calling `getLogger()` twice with the same name
will return the same Logger instance twice.

Logger names are dot-separated hierarchical. So the names "a", "a.a", "a.b",
"a.b.c", "b", "b.a", and "b.b" would be organized in a hierarchy like so:

```
root
|-- a
|   |-- a.a
|   └-- a.b
|       └-- a.b.c
└-- b
    |-- b.a
    └-- b.b
```

If a log is emitted under a child Logger whose level has been set to
`logging.NOTSET`, then the log is propagated to the parent log if the Logger's
`propagate` property is set to `True`.

`Logger` keeps a reference to its parent in `Logger.parent`. If there is no
existing user-defined parent `Logger` it's just the root Logger.

Any time something is logged,
a [`LogRecord`](https://github.com/python/cpython/blob/d12b3e3152b1858e91c4890d0bf3a3b574a3ff6f/Lib/logging/__init__.py#L290)
is created for it:

`LogRecord` holds the name of the logger, the log message, the log level, and
a handful of other things.

`LogRecord`s are created internally by the `debug/warning/error/...` etc.
functions. But you can also obtain an instance of a `LogRecord` with
[`makeLogRecord()`](https://github.com/python/cpython/blob/6c13e13b13bebfdde8ad4019536499820cdfc926/Lib/logging/__init__.py#L421).

[`Handler`](https://github.com/python/cpython/blob/d12b3e3152b1858e91c4890d0bf3a3b574a3ff6f/Lib/logging/__init__.py#L922)
dispatches logs to a specific destination.

`Handler` is a parent class which cannot actually emit logs. A subclass is
needed for that. `StreamHandler` is one example, which writes the logs to some
arbitrary stream object. There is also a `FileHandler`, which writes the logs
to a file, specified by its filename. Of course `StreamHandler` can output to
a file as well if the stream given to it is a file stream.

Handlers can be added to a Logger with
[`Logger.addHandler`](https://github.com/python/cpython/blob/6c13e13b13bebfdde8ad4019536499820cdfc926/Lib/logging/__init__.py#L1700),
and the handlers are kept under the list `Logger.handlers`

[`Filter`](https://github.com/python/cpython/blob/d12b3e3152b1858e91c4890d0bf3a3b574a3ff6f/Lib/logging/__init__.py#L773)
enables various filtering capabilities.

A [`Filterer`](https://github.com/python/cpython/blob/6c13e13b13bebfdde8ad4019536499820cdfc926/Lib/logging/__init__.py#L810)
can be assigned multiple Filters with `Filterer.addFilter`, and the Filter will
filter out any LogRecords that match. `Logger` and `Handler` are both child
classes of `Filterer`, so Filters can be added to either of them.

An instance of
[`Manager`](https://github.com/python/cpython/blob/d12b3e3152b1858e91c4890d0bf3a3b574a3ff6f/Lib/logging/__init__.py#L1356)
keeps track of the whole hierarchy of loggers.

There is normally just one instance of Manager at `Logger.manager`.

`Manager.getLogger` is called when we call the top-level `getLogger` function.

`Manager.loggerDict` is a dict where keys are the existing logger names and
values are the loggers themselves. If we create a child logger without first
creating its parent, a `PlaceHolder` object for the parent is added to
`Manager.loggerDict`. Then if we try to `getLogger` the parent, the
`PlaceHolder` is replaced with a new `Logger` instance.

[`LoggerAdapter`](https://github.com/python/cpython/blob/6c13e13b13bebfdde8ad4019536499820cdfc926/Lib/logging/__init__.py#L1876)
can be wrapped around a `Logger` to automatically add information to any logs
that are emitted through it.

[`Formatter`s](https://github.com/python/cpython/blob/d12b3e3152b1858e91c4890d0bf3a3b574a3ff6f/Lib/logging/__init__.py#L550)
are used to convert a `LogRecord` to text:

A Formatter can be assigned to a Handler, under `Handler.formatter`, using
`Handler.setFormatter`.

## Emitting a message

Let's look through all that needs to happen in order to emit a new message.
First, we import `logging` and create a `Logger`:

```python
import logging
logger = logging.getLogger('my_logger')
```

`logging.getLogger` calls `Logger.manager.getLogger`, which does the following:

```python
    def getLogger(self, name):
        ...
        rv = None
        ...
        _acquireLock()
        try:
            if name in self.loggerDict:
                rv = self.loggerDict[name]
                if isinstance(rv, PlaceHolder):
                    ph = rv
                    rv = (self.loggerClass or _loggerClass)(name)
                    rv.manager = self
                    self.loggerDict[name] = rv
                    self._fixupChildren(ph, rv)
                    self._fixupParents(rv)
            else:
                rv = (self.loggerClass or _loggerClass)(name)
                rv.manager = self
                self.loggerDict[name] = rv
                self._fixupParents(rv)
        finally:
            _releaseLock()
        return rv
```

If the Logger's name is not found, it is created and added to the loggerDict.
If it is found, but it's a PlaceHolder, it is created and the PlaceHolder is
replaced. If it's found and it's actually a Logger, it's just returned.

A lock is taken out before reading/writing the loggerDict because the client
code could be multi-threaded, using the
[`threading`](https://docs.python.org/3/library/threading.html) library. Two
threads will share the same internal state of the `logging` module, so the
locks serialize `logging` function calls coming from different threads to
prevent race conditions.

After we have the logger, we can emit a log message.

```python
logger.error('my error message')
```

This calls
[`Logger.error`](https://github.com/python/cpython/blob/5dcbbd8861e618488d95416dee8ea94577e3f4f0/Lib/logging/__init__.py#L1556).

Note that `Logger.error/debug/warning/...` etc. are specialized for the
corresponding log level, and they have a specialized implementation. In the
general case,
[`Logging.log`](https://github.com/python/cpython/blob/5dcbbd8861e618488d95416dee8ea94577e3f4f0/Lib/logging/__init__.py#L1592)
can be called, which has an argument `level` which specifies which level to
create the log with. `Logger.log` checks whether the `level` arg is the
correct type, which the specialized functions do not need to do, making them
slightly more performant.

In all the specialized functions and the general `Logger.log` function,
[`self.isEnabledFor(level)`](https://github.com/python/cpython/blob/5dcbbd8861e618488d95416dee8ea94577e3f4f0/Lib/logging/__init__.py#L1788)
is called to check if the level in question is enabled for this Logger. This
will depend on a few things, including `Logger.level` (which, again, is set by
`Logger.setLevel()`), `Logger.disabled`, and `Logger.manager.disable`.

If the log level is not enabled, then we completely avoid creating the log and
return from the specialized (`Logger.error/debug/...`) or general
(`Logger.log`) logging function without doing anything else.

If the log level in question is enabled, then
[`Logger._log`](https://github.com/python/cpython/blob/5dcbbd8861e618488d95416dee8ea94577e3f4f0/Lib/logging/__init__.py#L1658)
is called:

```python
    def _log(self, level, msg, args, exc_info=None, extra=None, stack_info=False,
             stacklevel=1):

        # fn, lno, func, sinfo, and exec_info are set by the removed code
        ...

        record = self.makeRecord(self.name, level, fn, lno, msg, args,
                                 exc_info, func, extra, sinfo)
        self.handle(record)
```

This creates a `LogRecord` for the log by calling
[`Logger.makeRecord()`](https://github.com/python/cpython/blob/5dcbbd8861e618488d95416dee8ea94577e3f4f0/Lib/logging/__init__.py#L1643)
and then [`Logger.handle`](https://github.com/python/cpython/blob/6c13e13b13bebfdde8ad4019536499820cdfc926/Lib/logging/__init__.py#L1684) is called to handle that record.

```python
    def handle(self, record):
        ...
        if self.disabled:
            return
        maybe_record = self.filter(record)
        if not maybe_record:
            return
        if isinstance(maybe_record, LogRecord):
            record = maybe_record
        self.callHandlers(record)
```

[`Filterer.filter`](https://github.com/python/cpython/blob/6c13e13b13bebfdde8ad4019536499820cdfc926/Lib/logging/__init__.py#L835)
applies any filters to the LogRecord. The filters may either silence the
LogRecord by making `Filterer.filter` return False, or they may modify the
LogRecord by returning a different LogRecord.

Then `Logger.handle` calls `Logger.callHandlers`, which gives the LogRecord to
any handlers that the Logger or its parent Loggers have:

```python
    def callHandlers(self, record):
        ...
        c = self
        found = 0
        while c:
            for hdlr in c.handlers:
                found = found + 1
                if record.levelno >= hdlr.level:
                    hdlr.handle(record)
            if not c.propagate:
                c = None    #break out
            else:
                c = c.parent
        if (found == 0):
            if lastResort:
                if record.levelno >= lastResort.level:
                    lastResort.handle(record)
            elif raiseExceptions and not self.manager.emittedNoHandlerWarning:
                sys.stderr.write("No handlers could be found for logger"
                                 " \"%s\"\n" % self.name)
                self.manager.emittedNoHandlerWarning = True
```

If there is no handler,
[`_StderrHandler`](https://github.com/python/cpython/blob/6c13e13b13bebfdde8ad4019536499820cdfc926/Lib/logging/__init__.py#L1290)
is used, which is just a child class of `StreamHandler` that uses `sys.stderr`
as the stream. This will be the default behavior if the client does not set the
Handlers for any Loggers.

For each Handler found,
[`Handler.handle`](https://github.com/python/cpython/blob/5dcbbd8861e618488d95416dee8ea94577e3f4f0/Lib/logging/__init__.py#L1014)
is called to handle the record.

```python
    def handle(self, record):
        ...
        rv = self.filter(record)
        if isinstance(rv, LogRecord):
            record = rv
        if rv:
            self.acquire()
            try:
                self.emit(record)
            finally:
                self.release()
        return rv
```

Notice that the Handler will also apply its own filters on top of the ones that
the Logger applied earlier.

`Handler.emit` is meant to be overridden by the child classes of Handler.  For
[`StreamHandler.emit`](https://github.com/python/cpython/blob/6c13e13b13bebfdde8ad4019536499820cdfc926/Lib/logging/__init__.py#L1151),
we have:

```python
    def emit(self, record):
        ...
        try:
            msg = self.format(record)
            stream = self.stream
            ...
            stream.write(msg + self.terminator)
            self.flush()
        except RecursionError:  # See issue 36272
            raise
        except Exception:
            self.handleError(record)
```

See that the
[`Handler.format`](https://github.com/python/cpython/blob/6c13e13b13bebfdde8ad4019536499820cdfc926/Lib/logging/__init__.py#L991)
is called on the record to convert it to text.

If the Handler does not have a Formatter assigned to it, the default case is to
use
[`_defaultFormatter`](https://github.com/python/cpython/blob/6c13e13b13bebfdde8ad4019536499820cdfc926/Lib/logging/__init__.py#L729)
which is just set to `Formatter()`. It simply extracts the `message` from the
`LogRecord`, and nothing else. For instance:

```python
>>> import logging
>>> logging._defaultFormatter.format(logging.makeLogRecord({'level': logging.ERROR, 'msg': 'my message', 'name': 'logger name', 'fn': 'my fn', 'lno': 'my lno'}))
'my message'
```

Finally, after the `LogRecord` is formatted, the the text is written to the
stream with `stream.write()`.
