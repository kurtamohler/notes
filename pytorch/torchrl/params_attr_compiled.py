from tensordict import from_module, TensorDictParams
import torch.nn

module = torch.nn.Module()
module.params = torch.nn.Parameter(torch.randn(3))
params2 = from_module(module).data.clone()
params2 *= 0
params2 = TensorDictParams(params2)
# Isolate the inner tensordict
params2 = params2._param_td

@torch.compile(fullgraph=True)
def func(z, params2):
    with params2.to_module(module):
        out = z + module.params
    return out

print(func(torch.zeros(()), params2))