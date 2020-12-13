#!/usr/bin/env ks
""" t_str.ks - test 'str' class and operations

Test string literals and operations

@author: Cade Brown <cade@kscript.org>
"""

assert "a" == 'a'
assert "abcdefg" == 'abcdefg'
assert "abc" + "def" == 'abc' + 'def'

assert '\x61' == 'a'
assert '\u0061' == 'a'
assert '\U00000061' == 'a'
assert '\N[LATIN SMALL LETTER A]' == 'a'

assert ord('a') == 0x61
assert chr(0x61) == 'a'
assert chr(ord('a')) == 'a'

alphabet = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ"

for x in alphabet {
    assert ord(x) in range(256)
}


