# `c10::impl::HermeticPyObjectTLS`

TLS stands for "thread local state", meaning a state that can store any
arbitrary information for a specific thread.

[This comment](https://github.com/pytorch/pytorch/blob/bdecdfd202df3fa25fd9998070fd19fee4b14971/c10/core/impl/HermeticPyObjectTLS.h#L9-L13)
in `c10/core/impl/HermeticPyObjectTLS.h` says:

> This TLS controls whether or not we permanently associate PyObject
> with Tensor the first time it is allocated.  When hermetic PyObject
> TLS is enabled (state is true), we DO NOT save PyObjects to Tensor,
> meaning you get a distinct PyObject whenever you execute the code in
> question.

However, with the addition of PyObject preservation for storages, and the
generalized PyObjectSlot, which both C++ Tensor and Storage now have, this
would be rewritten as:

> This TLS controls whether or not we permanently associate PyObject
> with PyObjectSlot the first time it is allocated.  When hermetic PyObject
> TLS is enabled (state is true), we DO NOT save PyObjects to PyObjectSlots,
> meaning you get a distinct PyObject whenever you execute the code in
> question.

The only place where the state of the `HermeticPyObjectTLS` is set is in the
constructor and destructor of
[`torch::impl::dispatch::EnableHermeticPyObject`](https://github.com/pytorch/pytorch/blob/4ee622476702fba3480572cbc712b552fd878605/torch/csrc/utils/python_dispatch.cpp#L76-L104).
`EnableHermeticPyObject` is just an RAII wrapper for setting the state to
`true` and then resetting to the previous value when it goes out of scope.

The only place where `EnableHermeticPyObject` is used is in
[`torch::impl::dispatch::PythonKernelHolder::operator()`](https://github.com/pytorch/pytorch/blob/4ee622476702fba3480572cbc712b552fd878605/torch/csrc/utils/python_dispatch.cpp#L172).

The only place where `PythonKernelHolder` is instantiated is within this
pybind11 function
[`torch._C._DispatchModule.impl`](https://github.com/pytorch/pytorch/blob/main/torch/csrc/utils/python_dispatch.cpp#L309-L338).
If the Python `func` provided to this function is not
`torch.library.fallthrough_kernel`, then it gets wrapped in
a `PythonKernelHolder` and it gets placed in `python_registrations_`.

In order to instantiate
[`torch._C._DispatchModule`](https://github.com/pytorch/pytorch/blob/28be2c674aa931b62c94978bef39273449b4b424/torch/_C/__init__.pyi.in#L1309),
we can call
[`torch._C._dispatch_library`](https://github.com/pytorch/pytorch/blob/28be2c674aa931b62c94978bef39273449b4b424/torch/_C/__init__.pyi.in#L1335-L1341).
Then we should be able to call `impl(...)` on it and provide our own Python
function.

```python
import torch

m = torch._C._dispatch_library('DEF', 'my_library', '')

def my_func():
    print('hello from my_func')

m.impl('my_func', 'CPU', my_func)
m.def_('my_func() -> None')
```

And supposedly, that should have created a `PythonKernelHolder` that wraps
`my_func` and puts it somewhere. How do I call it though?

Maybe the tests in
[`test/test_dispatch.py`](https://github.com/pytorch/pytorch/blob/bc662ffff9b4990db8decc972c9aaae93ff9c012/test/test_dispatch.py)
show how to do it.

There's also a public API in [`torch/library.py`](torch/library.py).

Ok now I have a working piece of code:

```python
import torch
from torch.library import Library, impl

my_lib = Library("my_lib", "DEF")

my_lib.define('my_func(Tensor self) -> None')

@impl(my_lib, 'my_func', 'CPU')
def my_func_cpu(self):
    print('in my_func_cpu!!!')

@impl(my_lib, 'my_func', 'CUDA')
def my_func_cuda(self):
    print('in my_func_cuda!!!')

torch.ops.my_lib.my_func(torch.tensor([]))
torch.ops.my_lib.my_func(torch.tensor([]).cuda())
```
