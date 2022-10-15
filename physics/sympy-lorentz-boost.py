import sympy as sp
import sympy.abc
import math


# `x` must depend on `sympy.abc.t`
def boost(v, x):
    # First, check that velocity and the derivative of x have magnitude <= 1
    assert abs(v) <= 1

    x_dot = sp.diff(x, sympy.abc.t)
    x_dot_min = sp.minimum(x_dot, sympy.abc.t, sp.Interval(-math.inf, math.inf))
    x_dot_max = sp.maximum(x_dot, sympy.abc.t, sp.Interval(-math.inf, math.inf))
    assert x_dot_min >= -1
    assert x_dot_max <= 1

    x_prime = (x - v * sympy.abc.t) / (1 - v**2)**0.5

    x_prime_sub_t = x_prime.subs(sympy.abc.t, sympy.abc.t * (1 - v**2)**0.5)

    print(x_prime_sub_t)
    print(sp.diff(x_prime_sub_t, sympy.abc.t))

    
v = 0.1

x = 0.9 * sp.sin(sp.abc.t)

x_prime = boost(v, x)

t0 = 10
x0 = x.subs(sp.abc.t, t0).evalf()
print(x0)





