#!/usr/bin/env ks
""" t_re.ks - Testing the math module (`m`)

@author: Cade Brown <cade@kscript.org>
"""

import m

assert m.sin(0) == 0
assert m.cos(0) == 1

assert 3 < m.pi < 4
assert 3.1415926 < m.pi < 3.1415927

assert 6 < m.tau < 7
assert abs(m.tau - 2 * m.pi) < 1e-6

assert abs(m.zeta(2) - m.pi ** 2 / 6) < 1e-6
assert abs(m.zeta(4) - m.pi ** 4 / 90) < 1e-6

for i in range(20), assert abs(m.sqrt(i) ** 2 - i) < 1e-6
for i in range(-20, 20), assert abs(m.cbrt(i) ** 3 - i) < 1e-6

assert m.gamma(0).isnan()
assert !m.gamma(1).isnan()
assert m.gamma(1) == m.gamma(2) == 1

assert m.zeta(1) == inf
assert m.zeta(0) == -.5
assert abs(m.zeta(-1) - -1 / 12) < 1e-6

for i in range(-2, -10, -2), assert m.zeta(i) == 0
