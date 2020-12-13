#!/usr/bin/env ks
""" t_uninames.ks - Unicode variable/function/type names

Tests whether the compiler can correctly handle unicode in identifiers

@author: Cade Brown <cade@kscript.org>
"""

друг = "My Very Good Friend"

assert type(друг) == str
assert друг[0] == 'M'

π = 3.1415926
assert 3 < π < 4

久有归天愿 = 4
终过鬼门关 = 5

assert 久有归天愿 + 终过鬼门关 == 9
