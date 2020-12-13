#!/usr/bin/env ks
""" t_re.ks - Regex module

@author: Cade Brown <cade@kscript.org>
"""

import re

assert !`ab+`.exact('a')
assert `ab+`.exact('ab')
assert `ab+`.exact('abbbbb')
assert !`ab+`.exact('abbbbba')



