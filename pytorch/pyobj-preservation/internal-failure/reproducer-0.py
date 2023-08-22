import torch
from torch.library import Library, impl
import sys 
import builtins

# NOTE: Need to flush stdout in order to maintain the order of prints from C++
# and Python contexts
def print(*args, **kwargs):
    sys.stdout.flush()
    builtins.print(*args, **kwargs)
    sys.stdout.flush()

my_lib = Library("my_lib", "DEF")
my_lib.define('my_func() -> None')

print('> a = torch.tensor([1.])')
a = torch.tensor([1.])

print('> s0 = a.untyped_storage()')
s0 = a.untyped_storage()

@impl(my_lib, 'my_func', '') 
def my_func():
    print('> in my_func - hermetic')
    print('> global s0')
    global s0

    print('> del s0')
    del s0

print('> my_func()')
torch.ops.my_lib.my_func()

print('> outside my_func')
print('> s0 = a.untyped_storage()')
s0 = a.untyped_storage()
