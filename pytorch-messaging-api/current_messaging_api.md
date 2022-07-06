# PyTorch's current messaging API

At the moment, PyTorch has some APIs in place to make a lot of aspects of
message logging simple, from the perspective of a developer working on PyTorch.
Messages can be either printouts, warnings, or errors.

Errors are created with the standard `raise` statement in Python
([documentation](https://docs.python.org/3/tutorial/errors.html#raising-exceptions)).
In C++, PyTorch offers macros for creating errors (which are listed later in
this document). After a C++ function is called from Python, any errors that
were generated get converted to Python errors.

Warnings are created with `warnings.warn` in Python
([documentation](https://docs.python.org/3/library/warnings.html)). In C++,
PyTorch offers macros for creating warnings (which are listed later in this
document). After a C++ function is called from Python, any warnings that were
generated get converted to Python warnings.

Printouts (or what may be called "Info" severity messages in the future) are
created with just `print` in Python and `std::cout` in C++. So PyTorch doesn't
actually have any APIs for info messages yet.

PyTorch's C++ warning/error macros are declared in
[`c10/util/Exception.h`](https://github.com/pytorch/pytorch/blob/72e4aab74b927c1ba5c3963cb17b4c0dce6e56bf/c10/util/Exception.h).

# PyTorch C++ Errors

In C++, there are several different types of errors that can be used.
Typically, PyTorch developers don't deal with error classes directly. Instead,
they use macros that offer a concise interface.

## C++ error macros

Each of the error macros evaluate a boolean conditional expression, `cond`. If
the condition is false, the error is raised, and whatever extra arguments are
in `...` get concatenated into the error message with `operator<<`.

| Macro                                    | C++ Error class                |
| ---------------------------------------- | ------------------------------ |
| `TORCH_CHECK(cond, ...)`                 | `c10::Error`                   |
| `TORCH_CHECK_WITH(error_t, cond, ...)`   | caller specifies `error_t` arg |
| `TORCH_CHECK_LINALG(cond, ...)`          | `c10::LinAlgError`             |
| `TORCH_CHECK_INDEX(cond, ...)`           | `c10::IndexError`              |
| `TORCH_CHECK_VALUE(cond, ...)`           | `c10::ValueError`              |
| `TORCH_CHECK_TYPE(cond, ...)`            | `c10::TypeError`               |
| `TORCH_CHECK_NOT_IMPLEMENTED(cond, ...)` | `c10::NotImplementedError`     |

There is some documentation on error macros [here](https://github.com/pytorch/pytorch/blob/72e4aab74b927c1ba5c3963cb17b4c0dce6e56bf/c10/util/Exception.h#L344-L362)

The reason why C++ preprocessor macros are used, rather that function calls, is
to ensure that the compiler can optimize for the `cond == true` branch. In
other words, if an error does not get raised, overhead is minimized.

## C++ error classes

The primary error class in C++ is `c10::Error`. Documentation and declaration
are
[here](https://github.com/pytorch/pytorch/blob/72e4aab74b927c1ba5c3963cb17b4c0dce6e56bf/c10/util/Exception.h#L21-L28).
`c10::Error` is a subclass of `std::exception`.

There are other error classes which are child classes of `c10::Error`, defined
[here](https://github.com/pytorch/pytorch/blob/72e4aab74b927c1ba5c3963cb17b4c0dce6e56bf/c10/util/Exception.h#L195-L236).

When these errors propagate to Python, they are each converted to a different
Python error class:

| C++ error class                 | Python error class         |
| ------------------------------- | -------------------------- |
| `c10::Error`                    | `RuntimeError`             |
| `c10::IndexError`               | `IndexError`               |
| `c10::ValueError`               | `ValueError`               |
| `c10::TypeError`                | `TypeError`                |
| `c10::NotImplementedError`      | `NotImplementedError`      |
| `c10::EnforceFiniteError`       | `ExitException`            |
| `c10::OnnxfiBackendSystemError` | `ExitException`            |
| `c10::LinAlgError`              | `torch.linalg.LinAlgError` |


# PyTorch C++ Warnings

When warnings propagate to Python, they are converted to a Python
`UserWarning`. Whatever is in `...` will get concatenated into the warning
message using `operator<<`.

* `TORCH_WARN(...)`
  - [Definition](https://github.com/pytorch/pytorch/blob/72e4aab74b927c1ba5c3963cb17b4c0dce6e56bf/c10/util/Exception.h#L515-L530)

* `TORCH_WARN_ONCE(...)`
  - [Definition](https://github.com/pytorch/pytorch/blob/72e4aab74b927c1ba5c3963cb17b4c0dce6e56bf/c10/util/Exception.h#L557-L562)
  - This macro only generates a warning the first time it is encountered during
    run time.


# Implementation details

## C++ to Python Error Translation

`c10::Error` and its subclasses are translated into their corresponding Python
errors [in `CATCH_CORE_ERRORS`](https://github.com/pytorch/pytorch/blob/72e4aab74b927c1ba5c3963cb17b4c0dce6e56bf/torch/csrc/Exceptions.h#L54-L100).

However, not all of the `c10::Error` subclasses in the table above appear here.
I'm not sure yet what's up with that. I wonder if the ones that don't appear
here don't actually work?

`CATCH_CORE_ERRORS` is included within the `END_HANDLE_TH_ERRORS` macro that
every Python-bound function uses for handling errors. For instance,
`THPVariable__is_view` uses the error handling macro
[here](https://github.com/pytorch/pytorch/blob/72e4aab74b927c1ba5c3963cb17b4c0dce6e56bf/tools/autograd/templates/python_variable_methods.cpp#L76).


### `torch::PyTorchError`

There's also an extra error class in `CATCH_CORE_ERRORS`,
`torch::PyTorchError`. I'm not sure what it's for yet. It has several
overloads:

* `torch::IndexError`
* `torch::TypeError`
* `torch::ValueError`
* `torch::NotImplementedError`
* `torch::AttributeError`
* `torch::LinAlgError`


## C++ to Python Warning Translation

The conversion of warnings from C++ to Python is described [here](https://github.com/pytorch/pytorch/blob/72e4aab74b927c1ba5c3963cb17b4c0dce6e56bf/torch/csrc/Exceptions.h#L25-L48)


# Misc Notes

[PyTorch Developer Podcast - Python exceptions](https://pytorch-dev-podcast.simplecast.com/episodes/python-exceptions)
explains how C++ errors/warnings are converted to Python. TODO: listen to it
again and take notes.

