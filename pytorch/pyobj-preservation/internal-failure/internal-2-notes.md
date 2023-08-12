Edward ran the patch
[./storage-preservation-logs-2.patch](./storage-preservation-logs-2.patch) on
top of my PR https://github.com/pytorch/pytorch/pull/103907. This patch prints
some logs to get more information about an Meta-internal model failure where
`Tensor.untyped_storage()` is incorrectly returning a `dict`. The following are
several relevant sections of the logs and my notes about them. The numbers in
front of each quoted line is the line number within the log output file.

## Create storage PyObject from Tensor (`Tensor.untyped_storage()`)

```
1795 [torch/csrc/autograd/generated/python_variable_methods.cpp:1498] (tensor PyObject: 0x7f31d78c96d0) in THPVariable_storage
1796 [torch/csrc/autograd/generated/python_variable_methods.cpp:1504] (tensor PyObject: 0x7f31d78c96d0) storage data: 0x7f32f55f8080
```

`THPVariable_storage` has been called on a tensor at address 0x7f31d78c96d0,
which must have come from a `Tensor.untyped_storage()` call in Python. The
`c10::Storage`'s data pointer points to address 0x7f32f55f8080.

```
1797 [torch/csrc/DynamicTypes.cpp:63] (0x7f32f55f8080) in createPyObject
1798 [torch/csrc/Storage.cpp:107] (0x7f32f55f8080) in THPStorage_Wrap
```

`THPVariable_storage` calls `createPyObject`, which calls `THPStorage_Wrap`,
which will either create a new Python storage or return the Python storage that
is held in the `c10::Storage`'s PyObjectSlot.

```
1799 [torch/csrc/Storage.cpp:159] (0x7f32f55f8080)
```

This log indicates that the PyObjectSlot is empty.

```
1800 [torch/csrc/Storage.cpp:164] (0x7f32f55f8080)
```

This log indicates that the `c10::Storage` has a use count greater than 1.
So there is likely at least one more tensor that shares this storage.

```
1801 [torch/csrc/Storage.cpp:43] (0x7f32f55f8080) in THPStorage_NewWithStorage, arg type->tp_name: torch.storage.UntypedStorage
1802 [torch/csrc/Storage.cpp:80] (0x7f32f55f8080), creating PyObject of type torch.storage.UntypedStorage
1803 [torch/csrc/Storage.cpp:84] (0x7f32f55f8080), created storage PyObject of type torch.storage.UntypedStorage at 0x7f31d78d3fc0
1804 [torch/csrc/Storage.cpp:93] (0x7f32f55f8080), not hermetic
```

Since the PyObjectSlot was empty, `THPStorage_NewWithStorage` is called to
create a new Python storage object of type `UntypedStorage`. The object is at
address 0x7f31d78d3fc0.

```
1805 [torch/csrc/DynamicTypes.cpp:78] (0x7f32f55f8080) type_name: torch.storage.UntypedStorage, PyObject*: 0x7f31d78d3fc0
```

This log is printed at the end of `createPyObject`, and it confirms that the
`UntypedStorage` at address 0x7f31d78d3fc0 is being returned to the Python
context.

## Preserve the storage PyObject

```
3209 [torch/csrc/Storage.cpp:241] (PyObject: 0x7f31d78d3fc0) in THPStorage_subclass_dealloc
```

Later on in the trace, we see this `THPStorage_subclass_dealloc` call on the
storage PyObject at address 0x7f31d78d3fc0. This indicates that the Python
storage object's references have been deleted. It needs to be either preserved
or deallocated.

```
3210 [torch/csrc/Storage.cpp:194] (PyObject: 0x7f31d78d3fc0) in THPStorage_tryPreserve
3211 [torch/csrc/Storage.cpp:172] (PyObject: 0x7f31d78d3fc0) in THPStorage_isPreservable
3212 [torch/csrc/Storage.cpp:178] (PyObject: 0x7f31d78d3fc0) c10::Storage data: 0x7f32f55f8080
3213 [torch/csrc/Storage.cpp:201] (PyObject: 0x7f31d78d3fc0) preserving PyObject
```

The `THPStorage_isPreservable` call determined that the storage PyObject should
be preserved because the PyObjectSlot matches the PyObject and the
`c10::Storage` has a use count greater than 1, so the PyObject may be needed
again in the future.

```
3214 [torch/csrc/Storage.cpp:205] (PyObject: 0x7f31d78d3fc0) PyObject type: torch.storage.UntypedStorage
3215 [torch/csrc/Storage.cpp:221] (PyObject: 0x7f31d78d3fc0) PyObjectSlot points to PyObject at 0x7f31d78d3fc0
3216 [torch/csrc/Storage.cpp:224] (PyObject: 0x7f31d78d3fc0) PyObjectSlot's PyObject type: torch.storage.UntypedStorage
```

During the storage PyObject preservation, these logs confirm that the type of
the PyObject is correct and that the PyObjectSlot points to the correct
PyObject.

## Resurrect the storage PyObject (`Tensor.untyped_storage()`)

```
3289 [torch/csrc/autograd/generated/python_variable_methods.cpp:1498] (tensor PyObject: 0x7f31d7995630) in THPVariable_storage
3290 [torch/csrc/autograd/generated/python_variable_methods.cpp:1504] (tensor PyObject: 0x7f31d7995630) storage data: 0x7f32f55f8080
```

A little later on in the log, there is a call to `THPVariable_storage` on
a different tensor than the earlier one, again presumably from
a `Tensor.untyped_storgage()` call in Python. But this tensor is at address
0x7f31d7995630 instead of 0x7f31d7995630. This tensor's `c10::Storage` points
to the same data as that of the earlier tensor, at address 0x7f32f55f8080.

```
3291 [torch/csrc/DynamicTypes.cpp:63] (0x7f32f55f8080) in createPyObject
3292 [torch/csrc/Storage.cpp:107] (0x7f32f55f8080) in THPStorage_Wrap
```

Similar to before, `createPyObject` is called, which calls `THPStorage_Wrap`,
to get a PyObject for the storage.

```
3293 [torch/csrc/Storage.cpp:134] (0x7f32f55f8080) has PyObject
3294 [torch/csrc/Storage.cpp:137] (0x7f32f55f8080) PyObjectSlot points to PyObject at 0x7f31d78d3fc0
3295 [torch/csrc/Storage.cpp:139] (0x7f32f55f8080) PyObjectSlot's PyObject type: torch.storage.UntypedStorage
3296 [torch/csrc/Storage.cpp:143] (0x7f32f55f8080) PyObject was preserved, resurrecting
3297 [torch/csrc/DynamicTypes.cpp:78] (0x7f32f55f8080) type_name: torch.storage.UntypedStorage, PyObject*: 0x7f31d78d3fc0
```

Unlike the earlier `THPStorage_Wrap` call, this time the PyObjectSlot contains
a PyObject. This is the `UntypedStorage` at address 0x7f31d78d3fc0 which was
created during the earlier `THPStorage_Wrap` call and then subsequently
preserved. So now it gets resurrected.

## Original tensor's storage was replaced at some point

```
3302 [torch/csrc/autograd/generated/python_variable_methods.cpp:1498] (tensor PyObject: 0x7f31d78c96d0) in THPVariable_storage
3303 [torch/csrc/autograd/generated/python_variable_methods.cpp:1504] (tensor PyObject: 0x7f31d78c96d0) storage data: 0x7f32f55f8140
```

Here, the tensor at address 0x7f31d78c96d0 (from the first
`THPVariable_storage` call above) now has a different storage. Probably
something like `Tensor.set_(storage)` was called on this tensor earlier.

## Deallocate storage PyObject

```
3482 [torch/csrc/Storage.cpp:241] (PyObject: 0x7f31d78d3fc0) in THPStorage_subclass_dealloc
3483 [torch/csrc/Storage.cpp:194] (PyObject: 0x7f31d78d3fc0) in THPStorage_tryPreserve
3484 [torch/csrc/Storage.cpp:172] (PyObject: 0x7f31d78d3fc0) in THPStorage_isPreservable
3485 [torch/csrc/Storage.cpp:178] (PyObject: 0x7f31d78d3fc0) c10::Storage data: 0x7f32f55f8080
3486 [torch/csrc/Storage.cpp:182] (PyObject: 0x7f31d78d3fc0) PyObjectSlot has a different PyObject: 0, storage.use_count: 2
```

Now we're back in `THPStorage_subclass_dealloc` for the Python storage at
address 0x7f31d78d3fc0, because the refcount has gone to 0 again. Here we see
that the PyObjectSlot's PyObject is apparently a nullptr. To be clear, it may
not actually be nullptr--it's just that `PyObjectSlot::check_pyobj` either
returned either `c10::nullopt` or `c10::optional<PyObject*>(nullptr)`. This
can happen in any of these cases:

  1. The interpreter assigned to the PyObjectSlot does not match the current
     process's Python interpreter (or one of the multiple interpreters in the
     case of MultiPy)

  2. The previous case did not happen and
     `c10::impl::HermeticPyObjectTLS::get_state()` returns true

  3. The previous two cases did not happen and the `PyObject*` in the PyObjectSlot
     is genuinely `nullptr`.


I believe that case 1 must be what happened, but I should confirm that for sure
with extra log messages. But if I'm right, then it is still possible (and
I think most likely) that the `PyObject*` in the PyObjectSlot does actually
match the PyObject that `THPStorage_subclass_dealloc` is being called on
(because the PyObject's address matches the address that `THPVariable_storage`
gets called on later on in the trace). And if that's true, then since the use
count of the `c10::Storage` is greater than 1, this PyObject should not be
deallocated, because any `PyObject*` associated with that other live C++
reference would then point to undefined memory.

```
3487 [torch/csrc/Storage.cpp:251] (PyObject: 0x7f31d78d3fc0) c10::Storage data: 0x7f32f55f8080, use_count: 2
3488 [torch/csrc/Storage.cpp:262] (PyObject: 0x7f31d78d3fc0) deallocating storage PyObject
```

And here we see that the PyObject does get deallocated.

## Access the storage PyObject that was deallocated (`Tensor.untyped_storage()`)

```
5026 [torch/csrc/autograd/generated/python_variable_methods.cpp:1498] (tensor PyObject: 0x7f31d7995630) in THPVariable_storage
5027 [torch/csrc/autograd/generated/python_variable_methods.cpp:1504] (tensor PyObject: 0x7f31d7995630) storage data: 0x7f32f55f8080
5028 [torch/csrc/DynamicTypes.cpp:63] (0x7f32f55f8080) in createPyObject
5029 [torch/csrc/Storage.cpp:107] (0x7f32f55f8080) in THPStorage_Wrap
5030 [torch/csrc/Storage.cpp:134] (0x7f32f55f8080) has PyObject
5031 [torch/csrc/Storage.cpp:137] (0x7f32f55f8080) PyObjectSlot points to PyObject at 0x7f31d78d3fc0
5032 [torch/csrc/Storage.cpp:139] (0x7f32f55f8080) PyObjectSlot's PyObject type: dict
```

Then here we see that the same PyObject at 0x7f31d78d3fc0 is being accessed
again, after it was deallocated. In this case, the deallocated memory was
evidently reallocated by something else and overwritten, because now CPython
interprets that section of memory as a `dict` rather than an `UntypedStorage`.
