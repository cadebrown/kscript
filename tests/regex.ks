#!/usr/bin/env ks
""" regex.ks - Test cases for the `regex` type

@author: Cade Brown <cade@kscript.org>
"""

ret 

assert !`ab+`.exact('a')
assert `ab+`.exact('ab')
assert `ab+`.exact('abbbbb')
assert !`ab+`.exact('abbbbba')

