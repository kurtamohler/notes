import torch
import torchrl
from torchrl.envs.transforms import Transform
from tensordict import TensorDict, TensorDictBase
from tensordict.nn import dispatch
from tensordict.utils import _zip_strict

from torchrl.envs import TransformedEnv
from torchrl.envs import UnityMLAgentsEnv
from torchrl.envs.transforms.transforms import _apply_to_composite


class BatchAgentsTransform(Transform):

    def _call(self, tensordict: TensorDictBase) -> TensorDictBase:
        for in_key, out_key in _zip_strict(self.in_keys, self.out_keys):
            value = tensordict.get(in_key, default=None)
            if value is not None:
                observation = self._apply_transform(value)
                tensordict.set(
                    out_key,
                    observation,
                )
            elif not self.missing_tolerance:
                raise KeyError(
                    f"{self}: '{in_key}' not found in tensordict {tensordict}"
                )
        return tensordict


    @dispatch(source="in_keys", dest="out_keys")
    def forward(self, tensordict: TensorDictBase) -> TensorDictBase:
        """Reads the input tensordict, and for the selected keys, applies the transform."""
        for in_key, out_key in _zip_strict(self.in_keys, self.out_keys):
            data = tensordict.get(in_key, None)
            if data is not None:
                data = self._apply_transform(data)
                tensordict.set(out_key, data)
            elif not self.missing_tolerance:
                raise KeyError(f"'{in_key}' not found in tensordict {tensordict}")
        return tensordict

    # The transform must also modify the data at reset time
    def _reset(
        self, tensordict: TensorDictBase, tensordict_reset: TensorDictBase
    ) -> TensorDictBase:
        return self._call(tensordict_reset)

    # _apply_to_composite will execute the observation spec transform across all
    # in_keys/out_keys pairs and write the result in the observation_spec which
    # is of type ``Composite``
    #@_apply_to_composite
    #def transform_observation_spec(self, observation_spec):
    #    return Bounded(
    #        low=-1,
    #        high=1,
    #        shape=observation_spec.shape,
    #        dtype=observation_spec.dtype,
    #        device=observation_spec.device,
    #    )

env = UnityMLAgentsEnv(registered_name="3DBall")

try:

    agent_keys = [keys[:-1] for keys in env.action_keys]
    observation_keys = [keys + ("VectorSensor_size8",) for keys in agent_keys]
    done_keys = [keys + ("done",) for keys in agent_keys]
    truncated_keys = [keys + ("truncated",) for keys in agent_keys]
    terminated_keys = [keys + ("terminated",) for keys in agent_keys]
    reward_keys = [keys + ("reward",) for keys in agent_keys]
    print(agent_keys)
    print(env.action_keys)

    transformed_env = TransformedEnv(
        env,
        #BatchAgentsTransform(
        #    in_keys=agent_keys,
        #    out_keys=["agents"]
        #),
        torchrl.envs.Compose(
            #torchrl.envs.UnsqueezeTransform(
            #    dim=-1,
            #    #in_keys=observation_keys,
            #    in_keys=agent_keys,
            #),
            torchrl.envs.CatTensors(
                in_keys=observation_keys,
                out_key=("agents", "observation"),
                del_keys=True,
                unsqueeze_if_oor=True,
                dim=1,
            ),
            #torchrl.envs.CatTensors(
            #    in_keys=reward_keys,
            #    out_key=("agents", "reward"),
            #    del_keys=True,
            #    dim=0,
            #),
        )
    )

    print(transformed_env.reset())
    #print(transformed_env.observation_spec)
    #print(transformed_env.action_spec)
    print()
    #print(transformed_env.rollout(10))

finally:
    transformed_env.close()

