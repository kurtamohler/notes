Notes on the script [./reproducer-0.py](./reproducer-0.py) which reproduces
a Meta-internal failure when run with a PyTorch built from [kurtamohler/pytorch
branch
storage-pyobject-preservation-4-copy-0](https://github.com/kurtamohler/pytorch/tree/storage-pyobject-preservation-4-copy-0)
with the patch
[./storage-preservation-logs-4.patch](./storage-preservation-logs-4.patch)
applied on top of it to get extra log messages from the machinery that handles
the storage PyObject.

## Full log

Below is the full log of a run of the reproducer script. Lines beginning with
`>` are printed from Python, and they provide information about what functions
the Python interpreter ran. The other lines are printed from C++.

<details><summary>Click to expand</summary>

```
> a = torch.tensor([1.])
> s0 = a.untyped_storage()
[/home/kurtamohler/develop/pytorch-0/torch/csrc/autograd/generated/python_variable_methods.cpp:1498] (tensor PyObject: 0x7f73eb1bacc0) in THPVariable_storage
[/home/kurtamohler/develop/pytorch-0/torch/csrc/autograd/generated/python_variable_methods.cpp:1504] (tensor PyObject: 0x7f73eb1bacc0) storage data: 0x55fd8da3c340
[/home/kurtamohler/develop/pytorch-0/torch/csrc/DynamicTypes.cpp:64] (0x55fd8da3c340, interpreter: 0x55fd8bc9d1a0) in createPyObject
[/home/kurtamohler/develop/pytorch-0/torch/csrc/Storage.cpp:107] (0x55fd8da3c340, interpreter: 0x55fd8bc9d1a0) in THPStorage_Wrap
[/home/kurtamohler/develop/pytorch-0/torch/csrc/Storage.cpp:159] (0x55fd8da3c340) PyObjectSlot is empty, use_count:2
[/home/kurtamohler/develop/pytorch-0/torch/csrc/Storage.cpp:164] (0x55fd8da3c340) maybe uninitialized
[/home/kurtamohler/develop/pytorch-0/torch/csrc/Storage.cpp:43] (0x55fd8da3c340, interpreter: 0x55fd8bc9d1a0) in THPStorage_NewWithStorage, arg type->tp_name: torch.storage.UntypedStorage
[/home/kurtamohler/develop/pytorch-0/torch/csrc/Storage.cpp:80] (0x55fd8da3c340), creating PyObject of type torch.storage.UntypedStorage
[/home/kurtamohler/develop/pytorch-0/torch/csrc/Storage.cpp:84] (0x55fd8da3c340), created storage PyObject of type torch.storage.UntypedStorage at 0x7f73ea8b1c00
[/home/kurtamohler/develop/pytorch-0/torch/csrc/Storage.cpp:93] (0x55fd8da3c340), not hermetic
[/home/kurtamohler/develop/pytorch-0/torch/csrc/DynamicTypes.cpp:79] (0x55fd8da3c340) type_name: torch.storage.UntypedStorage, PyObject*: 0x7f73ea8b1c00
> my_func()
> in my_func - hermetic
> global s0
> del s0
[/home/kurtamohler/develop/pytorch-0/torch/csrc/Storage.cpp:241] (PyObject: 0x7f73ea8b1c00, interpreter: 0x55fd8bc9d1a0) in THPStorage_subclass_dealloc
[/home/kurtamohler/develop/pytorch-0/torch/csrc/Storage.cpp:249] (PyObject: 0x7f73ea8b1c00) hermetic
[/home/kurtamohler/develop/pytorch-0/torch/csrc/Storage.cpp:269] (PyObject: 0x7f73ea8b1c00) PyObjectSlot does not have PyObject
[/home/kurtamohler/develop/pytorch-0/torch/csrc/Storage.cpp:194] (PyObject: 0x7f73ea8b1c00, interpreter: 0x55fd8bc9d1a0) in THPStorage_tryPreserve
[/home/kurtamohler/develop/pytorch-0/torch/csrc/Storage.cpp:172] (PyObject: 0x7f73ea8b1c00, interpreter: 0x55fd8bc9d1a0) in THPStorage_isPreservable
[/home/kurtamohler/develop/pytorch-0/torch/csrc/Storage.cpp:178] (PyObject: 0x7f73ea8b1c00) c10::Storage data: 0x55fd8da3c340
[/home/kurtamohler/develop/pytorch-0/torch/csrc/Storage.cpp:182] (PyObject: 0x7f73ea8b1c00) PyObjectSlot has a different PyObject: 0, storage.use_count: 2
[/home/kurtamohler/develop/pytorch-0/torch/csrc/Storage.cpp:278] (PyObject: 0x7f73ea8b1c00) deallocating storage PyObject
> outside my_func
> s0 = a.untyped_storage()
[/home/kurtamohler/develop/pytorch-0/torch/csrc/autograd/generated/python_variable_methods.cpp:1498] (tensor PyObject: 0x7f73eb1bacc0) in THPVariable_storage
[/home/kurtamohler/develop/pytorch-0/torch/csrc/autograd/generated/python_variable_methods.cpp:1504] (tensor PyObject: 0x7f73eb1bacc0) storage data: 0x55fd8da3c340
[/home/kurtamohler/develop/pytorch-0/torch/csrc/DynamicTypes.cpp:64] (0x55fd8da3c340, interpreter: 0x55fd8bc9d1a0) in createPyObject
[/home/kurtamohler/develop/pytorch-0/torch/csrc/Storage.cpp:107] (0x55fd8da3c340, interpreter: 0x55fd8bc9d1a0) in THPStorage_Wrap
[/home/kurtamohler/develop/pytorch-0/torch/csrc/Storage.cpp:134] (0x55fd8da3c340) has PyObject
[/home/kurtamohler/develop/pytorch-0/torch/csrc/Storage.cpp:137] (0x55fd8da3c340) PyObjectSlot points to PyObject at 0x7f73ea8b1c00
Segmentation fault (core dumped)
```

</details>

## Summary

Here is an abbreviated version of the above log, which highlights some of the important parts:

```
> a = torch.tensor([1.])
> s0 = a.untyped_storage()
...
[/home/kurtamohler/develop/pytorch-0/torch/csrc/Storage.cpp:84] (0x55fd8da3c340), created storage PyObject of type torch.storage.UntypedStorage at 0x7f73ea8b1c00
...
> my_func()
> in my_func - hermetic
> global s0
> del s0
...
[/home/kurtamohler/develop/pytorch-0/torch/csrc/Storage.cpp:249] (PyObject: 0x7f73ea8b1c00) hermetic
...
[/home/kurtamohler/develop/pytorch-0/torch/csrc/Storage.cpp:278] (PyObject: 0x7f73ea8b1c00) deallocating storage PyObject
> outside my_func
> s0 = a.untyped_storage()
[/home/kurtamohler/develop/pytorch-0/torch/csrc/autograd/generated/python_variable_methods.cpp:1498] (tensor PyObject: 0x7f73eb1bacc0) in THPVariable_storage
[/home/kurtamohler/develop/pytorch-0/torch/csrc/autograd/generated/python_variable_methods.cpp:1504] (tensor PyObject: 0x7f73eb1bacc0) storage data: 0x55fd8da3c340
[/home/kurtamohler/develop/pytorch-0/torch/csrc/DynamicTypes.cpp:64] (0x55fd8da3c340, interpreter: 0x55fd8bc9d1a0) in createPyObject
[/home/kurtamohler/develop/pytorch-0/torch/csrc/Storage.cpp:107] (0x55fd8da3c340, interpreter: 0x55fd8bc9d1a0) in THPStorage_Wrap
[/home/kurtamohler/develop/pytorch-0/torch/csrc/Storage.cpp:134] (0x55fd8da3c340) has PyObject
[/home/kurtamohler/develop/pytorch-0/torch/csrc/Storage.cpp:137] (0x55fd8da3c340) PyObjectSlot points to PyObject at 0x7f73ea8b1c00
Segmentation fault (core dumped)
```

When we call `a.untyped_storage()`, a new storage PyObject is allocated to 0x7f73ea8b1c00.

Inside the hermetic function `my_func`, we obtain access to that PyObject with
the `global` keyword. Then we delete the reference. If `my_func` was
non-hermetic, the storage PyObject would just get preserved. But instead, it
gets deallocated. The HermeticPyObjectTLS assumes that any object it has access
to was allocated within the hermetic context, so it assumes that it must
deallocate it when its Python refcount goes to 0.

Then after `my_func` returns, we call `a.untyped_storage()` again. The tensor's
internal `c10::StorageImpl`'s `PyObjectSlot` still thinks its `PyObject*` is
valid, so `THPStorage_Wrap` tries to return that pointer. Since the memory at
that location has been deallocated, the behavior that happens afterward is
undefined. In this case, a segfault happened.

This is reproducer probably doesn't do exactly what the Meta-internal model is
doing to produce the failure. It's possible that there is another way to make
this situation happen without using `global`. However, the fix for this
reproducer should also fix the internal failure.
