#!/usr/bin/env ks
""" t_range.ks - test 'range' class and operations

Test string literals and operations

@author: Cade Brown <cade@kscript.org>
"""


for n in range(10) {
    assert len(range(n)) == n
}

for n in range(20) {
    for s in range(1, 10) {
        assert len(range(0, n, s)) == n // s
    }
}

# whole universe of ints we care about
univ = [*range(-256, 256)]

for n in range(20) {
    for s in range(1, 4) {
        r = range(0, n, s)
        for i in univ {
            assert (i in r) == (0 <= i < n && i % s == 0)
        }
    }
}
