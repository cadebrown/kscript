#!/usr/bin/env ks
""" t_uni.ks - Unicode testes

Tests source file reading, as well as unicode strings

@author: Cade Brown <cade@kscript.org>
"""

assert "аре" + "гистрируйтесь" == "арегистрируй" + "тесь"
assert len("арегистрируйтесь") == 16
