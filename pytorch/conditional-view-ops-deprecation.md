# Conditional view deprecation

Currently in PyTorch, some operations conditionally return either a view or a copy of an input, depending on the strides of the input. An example of one of these operations is `torch.reshape`:

```python
>>> import torch
>>> a = torch.zeros(2, 2)
>>> a_reshaped = a.reshape(4)
>>> a.data_ptr() == a_reshaped.data_ptr()
True
>>> b = a.T
>>> b_reshaped = b.reshape(4)
>>> b.data_ptr() == b_reshaped.data_ptr()
False
```

Above, `a_reshaped` points to the same data as `a`, so it is a view. `b_reshaped` points to different data than `b`, so it is a copy.

For all conditional view-or-copy operations, the view condition has been deprecated, and in a future release these operations will always return a copy. This will cause some PyTorch applications to be backward incompatible.

## Deprecation warnings



Consider the two following code snippets. They both run the same code, but one shows the current behavior in PyTorch while the other shows the future behavior when `reshape` always returns a copy.

Current: 
```python
>>> a = torch.zeros(2, 2)
>>> a_reshaped = a.reshape(4)
>>> a.data_ptr() == a_reshaped.data_ptr()
True
>>> a_reshaped[0] += 1
>>> a_reshaped
tensor([1., 0., 0., 0.])
>>> a  # mutated because `a_reshaped` is a view
tensor([[1., 0.],
        [0., 0.]])
```

Future:
```python
>>> a = torch.zeros(2, 2)
>>> a_reshaped = a.reshape(4)
>>> a.data_ptr() == a_reshaped.data_ptr()
False
>>> a_reshaped[0] += 1
>>> a_reshaped
tensor([1., 0., 0., 0.])
>>> a  # not mutated because `a_reshaped` is a copy
tensor([[0., 0.],
        [0., 0.]])
```

However, there are cases where differences in current and future behavior like in the example above do not have any real backward incompatible consequences. Specifically, if `a` is never given as an input to an operation after the point when `a_reshaped` is mutated, then there is no possibility that the operations that the application calls will produce different numerical results with current versus future behavior. 


Remove view tag?