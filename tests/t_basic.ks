#!/usr/bin/env ks
""" t_basic.ks - Basic input tests

Test basic input, including literals, truthiness, and equivalences

@author: Cade Brown <cade@kscript.org>
"""

# Test input formats and numeric identities among types

assert 2.0 == 2
assert 2.0 == 2.
assert 2.0 == 2 + 0i
assert 1e0 == 1
assert 1e2 == 100
assert 1e-2 == .01

assert 1.2345 == 1.2345
assert 1.2345 != 1.2344

# Test bases

assert 0xFF == 255
assert 0xFF.00 == 255
assert 0xFF.A == 255 + 10 / 16
assert 0x.8 == 0d.5 == 0o.4 == 0b.1
assert 0x1 == 0d1 == 0o1 == 0b1

assert 0b110 == 6

assert 0xFFFF == 2 ** 16 - 1

assert 10 ** 2 == 100
assert 10.0 ** 2 == 100

for x in range(10) {
    assert abs(x) == x
    assert abs(-x) == x
}

# Test truthiness

assert !0
assert 1
assert 2
assert 3

assert !0.0
assert 1.0
assert 2.0
assert 1.
assert 2.

assert !0.0i
assert 1.0i
assert 2.0i
assert 1.i
assert 2.i

assert true
assert int(true)
assert bool(1)

assert "a"
assert !""

assert ![]
assert [1]

assert (1,)
assert !()

assert !set()
assert {1}
assert {1, 2, 3}

assert !{}
assert {1:2}
assert {1:2, 2:3, 3:4}

assert ![*range(0)]
assert [*range(1)]
