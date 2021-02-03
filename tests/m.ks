#!/usr/bin/env ks
""" m.ks - Test cases for the `m` module

@author: Cade Brown <cade@kscript.org>
"""

import m

# PI constant
assert 3 < m.pi < 4
assert 3.1415926 < m.pi < 3.1415927

# TAU constant
assert 6 < m.tau < 7
assert abs(m.tau - 2 * m.pi) < 1e-6


# Arithmetic functions
assert m.abs(-1) == 1
assert m.floor(1.5) == 1
assert m.floor(-1.5) == -2

assert m.isclose(0, 0)
assert m.isclose(1, 1)
assert m.isclose(1e-10, 0)


# Basic trig
assert m.sin(0) == 0
assert m.cos(0) == 1
assert m.tan(0) == 0


# E constant
assert 2 < m.e < 3
assert .99 < m.log(m.e) < 1.01

# Gamma function, special values
assert m.gamma(0).isnan()
assert !m.gamma(1).isnan()
assert m.gamma(1) == m.gamma(2) == 1
assert m.isclose(m.gamma(3), 2)

# Zeta function, special values
assert m.zeta(1) == inf
assert m.zeta(0) == -.5
assert abs(m.zeta(2) - m.pi ** 2 / 6) < 1e-6
assert abs(m.zeta(4) - m.pi ** 4 / 90) < 1e-6
assert abs(m.zeta(-1) - -1 / 12) < 1e-6

for i in range(-2, -10, -2), assert m.zeta(i) == 0

# Check for backwards error
for i in range(20), assert abs(m.sqrt(i) ** 2 - i) < 1e-6


# GCD, Greatest Common Denominator
assert m.gcd(2, 3) == 1
assert m.gcd(3, 6) == 3
assert m.gcd(-3, 6) == 3
assert m.gcd(-3, 6) == m.gcd(3, -6) == m.gcd(-3, -6)

assert m.isclose(m.rad(180), m.pi)
assert m.isclose(m.deg(m.pi), 180)
