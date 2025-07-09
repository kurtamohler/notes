import mlagents_envs
from mlagents_envs.registry import default_registry
import logging
import os
import sys

class SuppressStdout:
    def __enter__(self):
        self._original_stdout_fd = os.dup(sys.stdout.fileno())
        self._devnull = os.open(os.devnull, os.O_WRONLY)
        os.dup2(self._devnull, sys.stdout.fileno())

    def __exit__(self, exc_type, exc_value, traceback):
        os.dup2(self._original_stdout_fd, sys.stdout.fileno())
        os.close(self._original_stdout_fd)
        os.close(self._devnull)


logging.disable(logging.CRITICAL)

registered_names = list(default_registry.keys())

print(registered_names)

def print_behavior_specs(behavior_specs):
    print(behavior_specs)


def print_env_details(registered_name):
    with SuppressStdout():
        env = default_registry[registered_name].make(
            no_graphics=True,
            #log_folder="/tmp/nothing",
            #additional_args=[
            #    #"-nolog",
            #    "-logFile /dev/null",
            #    #"> /dev/null",
            #],
        )
    env.reset()

    try:
        print(env.behavior_specs)

    finally:
        env.close()


print_env_details(registered_names[0])