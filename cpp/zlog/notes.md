# Notes on zlog

[zlog GitHub repo](https://github.com/HardySimpson/zlog)
[zlog documentation](http://hardysimpson.github.io/zlog/UsersGuide-EN.html)

Below are my notes about the concepts and implementation of zlog.

## Client API

### Log levels

When a log is generated, it is given a specific log level. This essentially the
same concept of levels that the Python `logging` module uses. The levels are
integer values that correspond to their priority. Ordered from lowest to
highest priority, the predefined log levels are:

  * DEBUG - 20
  * INFO - 40
  * NOTICE - 60
  * WARN - 80
  * ERROR - 100
  * FATAL - 120

There are macro functions to emit each of these levels (there are other macros
of different form as well):

  * `zlog_debug()`
  * `zlog_info()`
  * `zlog_notice()`
  * `zlog_warn()`
  * `zlog_error()`
  * `zlog_fatal()`

The signatures of each of these are `zlog_*(category, format, ...)`

`format` is a format string, and the `...` args are the arguments to fill in to
the format string, just like the arguments to `printf()`.

`category` is which log category to emit the log under.

### Log categories

Each log is emitted under a category. Log category is a similar concept to
`logging.Logger`s in Python, except that categories are not organized
hierarchically.

In C code we can create a new category with:

```c
zlog_category_t* zlog_get_category(const char* cname)
```

`zlog_get_category()` is quite similar to `logging.getLogger()` in Python. If
there is no existing category of the name given in `cname`, then a new category
is created for it and stored in a container internally to zlog. If the category
does exist already, it's just returned. So multiple calls with the same
category name will give pointers to the same object.

### Log rules

When we initialize zlog with `int zlog_init(const char* confpath)`, `confpath`
specifies the path to a configuration file. In this file, we can specify
logging rules.

Logging rules determine where log messages will be written. Rules are of the
form `<category expression>.<level expression> <output action>`.

When a log is generated, a given rule will be applied to it if:

  * The log's category matches the rule's category expression.

  * The log's level matches the rule's level expression.

If those two conditions are met, then the log will be emitted using the
rule's output action.

Some of the things that can be given as the output action are:

  * The path of a file, e.g. `"/tmp/my_log.txt"`. The log message will be
    appended to that file.

  * One of the standard streams, `>stdout`, `>stderr`, or `>syslog`. The log
    message will just be written to that stream.

  * A pipe, e.g. `|cat`

If there are multiple rules in the config file that match the category and
level of a given log, then all of those rules are applied. So this means that
the one log message can be emitted multiple times with different output
actions.

## Implementation

### `zlog_init`

[`zlog_init`](https://github.com/HardySimpson/zlog/blob/21a47854af38e51d63411e66bc249c47f4294901/src/zlog.c#L163)
initializes zlog. It takes out a `pthread` lock to prevent multiple threads
from clobbering the internal zlog state. Then it ensures that zlog has not
already been initialized. Then it calls
[`zlog_init_inner`](https://github.com/HardySimpson/zlog/blob/21a47854af38e51d63411e66bc249c47f4294901/src/zlog.c#L114),
which contains the meat of the initialization process.

[Here](https://github.com/HardySimpson/zlog/blob/21a47854af38e51d63411e66bc249c47f4294901/src/zlog.c#L138),
[`zlog_conf_new`](https://github.com/HardySimpson/zlog/blob/21a47854af38e51d63411e66bc249c47f4294901/src/conf.c#L119)
is called to load up the contents of the config file into
a [`zlog_conf_t`](https://github.com/HardySimpson/zlog/blob/21a47854af38e51d63411e66bc249c47f4294901/src/conf.h#L16)
object. There is a global pointer to this config object named
[`zlog_env_conf`](https://github.com/HardySimpson/zlog/blob/21a47854af38e51d63411e66bc249c47f4294901/src/zlog.c#L29)

[Here](https://github.com/HardySimpson/zlog/blob/21a47854af38e51d63411e66bc249c47f4294901/src/zlog.c#L144),
[`zlog_category_table_new`](https://github.com/HardySimpson/zlog/blob/21a47854af38e51d63411e66bc249c47f4294901/src/category_table.c#L40)
is called to create
a [`zc_hashtable_t`](https://github.com/HardySimpson/zlog/blob/21a47854af38e51d63411e66bc249c47f4294901/src/zc_hashtable.h#L22),
which will hold all of the log categories that are created and accessed by
a client calls to `zlog_get_category()`. There is a global pointer to this
table of categories named
[`zlog_env_categories`](https://github.com/HardySimpson/zlog/blob/21a47854af38e51d63411e66bc249c47f4294901/src/zlog.c#L31)
