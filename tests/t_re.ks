#!/usr/bin/env ks
""" t_re.ks - Regex features

@author: Cade Brown <cade@kscript.org>
"""

ret 

assert !`ab+`.exact('a')
assert `ab+`.exact('ab')
assert `ab+`.exact('abbbbb')
assert !`ab+`.exact('abbbbba')

