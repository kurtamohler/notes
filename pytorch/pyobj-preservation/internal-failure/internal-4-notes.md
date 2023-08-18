Notes on a Meta-internal model run with the patch
[./storage-preservation-logs-4.patch](./storage-preservation-logs-4.patch)
applied on top of my PR https://github.com/pytorch/pytorch/pull/103907.

### Storage PyObject is created (`storage = tensor.untyped_storage()`)

```
1795 [torch/csrc/autograd/generated/python_variable_methods.cpp:1498] (tensor PyObject: 0x7f00de78e450) in THPVariable_storage
1796 [torch/csrc/autograd/generated/python_variable_methods.cpp:1504] (tensor PyObject: 0x7f00de78e450) storage data: 0x7f01fc69b600
1797 [torch/csrc/DynamicTypes.cpp:64] (0x7f01fc69b600, interpreter: 0x7f026b8c6160) in createPyObject
1798 [torch/csrc/Storage.cpp:107] (0x7f01fc69b600, interpreter: 0x7f026b8c6160) in THPStorage_Wrap
1799 [torch/csrc/Storage.cpp:159] (0x7f01fc69b600) PyObjectSlot is empty, use_count:3
1800 [torch/csrc/Storage.cpp:164] (0x7f01fc69b600) maybe uninitialized
1801 [torch/csrc/Storage.cpp:43] (0x7f01fc69b600, interpreter: 0x7f026b8c6160) in THPStorage_NewWithStorage, arg type->tp_name: torch.storage.UntypedStorage
1802 [torch/csrc/Storage.cpp:80] (0x7f01fc69b600), creating PyObject of type torch.storage.UntypedStorage
1803 [torch/csrc/Storage.cpp:84] (0x7f01fc69b600), created storage PyObject of type torch.storage.UntypedStorage at 0x7f00df0a2340
1804 [torch/csrc/Storage.cpp:93] (0x7f01fc69b600), not hermetic
1805 [torch/csrc/DynamicTypes.cpp:79] (0x7f01fc69b600) type_name: torch.storage.UntypedStorage, PyObject*: 0x7f00df0a2340
```

The storage PyObject is at address 0x7f00df0a2340. It was created from a context where HermeticPyObjectTLS is disabled.

### Storage PyObject's refcout reaches 0 and is preserved (`del storage`)

```
3535 [torch/csrc/Storage.cpp:241] (PyObject: 0x7f00df0a2340, interpreter: 0x7f026b8c6160) in THPStorage_subclass_dealloc
3536 [torch/csrc/Storage.cpp:255] (PyObject: 0x7f00df0a2340) PyObjectSlot has PyObject, interpreter: 0x7f026b8c6160, PyObject*: 0x7f00df0a2340
3537 [torch/csrc/Storage.cpp:265] (PyObject: 0x7f00df0a2340) PyObjectSlot does not own PyObject
3538 [torch/csrc/Storage.cpp:194] (PyObject: 0x7f00df0a2340, interpreter: 0x7f026b8c6160) in THPStorage_tryPreserve
3539 [torch/csrc/Storage.cpp:172] (PyObject: 0x7f00df0a2340, interpreter: 0x7f026b8c6160) in THPStorage_isPreservable
3540 [torch/csrc/Storage.cpp:178] (PyObject: 0x7f00df0a2340) c10::Storage data: 0x7f01fc69b600
3541 [torch/csrc/Storage.cpp:201] (PyObject: 0x7f00df0a2340) preserving PyObject
3542 [torch/csrc/Storage.cpp:205] (PyObject: 0x7f00df0a2340) PyObject type: torch.storage.UntypedStorage
3543 [torch/csrc/Storage.cpp:221] (PyObject: 0x7f00df0a2340) PyObjectSlot points to PyObject at 0x7f00df0a2340
3544 [torch/csrc/Storage.cpp:224] (PyObject: 0x7f00df0a2340) PyObjectSlot's PyObject type: torch.storage.UntypedStorage
```

This is the same PyObject at address 0x7f00df0a2340 from earlier.

### Storage PyObject is resurrected (`storage = tensor.untyped_storage()`)

```
3635 [torch/csrc/autograd/generated/python_variable_methods.cpp:1498] (tensor PyObject: 0x7f00df099400) in THPVariable_storage
3636 [torch/csrc/autograd/generated/python_variable_methods.cpp:1504] (tensor PyObject: 0x7f00df099400) storage data: 0x7f01fc69b600
3637 [torch/csrc/DynamicTypes.cpp:64] (0x7f01fc69b600, interpreter: 0x7f026b8c6160) in createPyObject
3638 [torch/csrc/Storage.cpp:107] (0x7f01fc69b600, interpreter: 0x7f026b8c6160) in THPStorage_Wrap
3639 [torch/csrc/Storage.cpp:134] (0x7f01fc69b600) has PyObject
3640 [torch/csrc/Storage.cpp:137] (0x7f01fc69b600) PyObjectSlot points to PyObject at 0x7f00df0a2340
3641 [torch/csrc/Storage.cpp:139] (0x7f01fc69b600) PyObjectSlot's PyObject type: torch.storage.UntypedStorage
3642 [torch/csrc/Storage.cpp:143] (0x7f01fc69b600) PyObject was preserved, resurrecting
3643 [torch/csrc/DynamicTypes.cpp:79] (0x7f01fc69b600) type_name: torch.storage.UntypedStorage, PyObject*: 0x7f00df0a2340
```

Again, the same PyObject at 0x7f00df0a2340

### Storage PyObject's refcout reaches 0 again but causes deallocation (`del storage`)

```
3837 [torch/csrc/Storage.cpp:241] (PyObject: 0x7f00df0a2340, interpreter: 0x7f026b8c6160) in THPStorage_subclass_dealloc
3838 [torch/csrc/Storage.cpp:249] (PyObject: 0x7f00df0a2340) hermetic
3839 [torch/csrc/Storage.cpp:269] (PyObject: 0x7f00df0a2340) PyObjectSlot does not have PyObject
3840 [torch/csrc/Storage.cpp:194] (PyObject: 0x7f00df0a2340, interpreter: 0x7f026b8c6160) in THPStorage_tryPreserve
3841 [torch/csrc/Storage.cpp:172] (PyObject: 0x7f00df0a2340, interpreter: 0x7f026b8c6160) in THPStorage_isPreservable
3842 [torch/csrc/Storage.cpp:178] (PyObject: 0x7f00df0a2340) c10::Storage data: 0x7f01fc69b600
3843 [torch/csrc/Storage.cpp:182] (PyObject: 0x7f00df0a2340) PyObjectSlot has a different PyObject: 0, storage.use_count: 2
3844 [torch/csrc/Storage.cpp:278] (PyObject: 0x7f00df0a2340) deallocating storage PyObject
```

This is still the same PyObject at 0x7f00df0a2340, but somehow the
HermeticPyObjectTLS is enabled.

When HermeticPyObjectTLS is enabled, all PyObjects are supposed to be fully
managed by the hermetic context. Any storage or tensor PyObject that the
hermetic context has access to is supposed to have been created only while
HermeticPyObjectTLS was enabled. When a tensor or storage PyObject's refcount
goes to 0 in the hermetic context, they are supposed to be unconditionally
deallocated.

So in this case, the hermetic context accidentally has access to a storage
PyObject that was created in a non-hermetic context. The hermetic context
deallocates it when its refcount goes to 0, which invalidates all pointers to
this PyObject that exist in the non-hermetic context--namely the `PyObject*`
stored in the PyObjectSlot of the `c10::StorageImpl`.

### Storage PyObject is accessed again after being deallocated (`tensor.untyped_storage()`)

```
5392 [torch/csrc/autograd/generated/python_variable_methods.cpp:1498] (tensor PyObject: 0x7f00df099400) in THPVariable_storage
5393 [torch/csrc/autograd/generated/python_variable_methods.cpp:1504] (tensor PyObject: 0x7f00df099400) storage data: 0x7f01fc69b600
5394 [torch/csrc/DynamicTypes.cpp:64] (0x7f01fc69b600, interpreter: 0x7f026b8c6160) in createPyObject
5395 [torch/csrc/Storage.cpp:107] (0x7f01fc69b600, interpreter: 0x7f026b8c6160) in THPStorage_Wrap
5396 [torch/csrc/Storage.cpp:134] (0x7f01fc69b600) has PyObject
5397 [torch/csrc/Storage.cpp:137] (0x7f01fc69b600) PyObjectSlot points to PyObject at 0x7f00df0a2340
5398 [torch/csrc/Storage.cpp:139] (0x7f01fc69b600) PyObjectSlot's PyObject type: dict
```

Since the PyObject at 0x7f00df0a2340 was deallocated, this behavior is
undefined. In this case, a Python dict was apparently allocated to this
address.
