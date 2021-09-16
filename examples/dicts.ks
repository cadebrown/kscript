#!/usr/bin/env ks
""" examples/dicts.ks - examples of how to use dictionaries (dicts) and dict-like interfaces


@author: Cade Brown <cade@kscript.org>
"""

# '{}' is the empty dict, with no mappings
x = {}
assert len(x) == 0

# You can use the 'is' operator to tell if an object is a dict or dict-like object
# NOTE: This is NOT the same as checking if 'isinst(x, dict)'. For example, 'util.bst()' behaves
#         like a dict but is not a subtype. You should use 'is' to test whether something has a 
#         dict-like interface
assert x is dict


# You can create a non-empty dict with 'key: val' mappings
# NOTE: dicts are ordered by insertion order... so whatever key appears first
#         is the first to be iterated over
x = {
    "name": "CADE E BROWN",
    "email": "cade@kscript.org",
    "is_author": true,
}
assert len(x) == 3

# You can assign new entries (or update existing ones) with '[]='
x["bio"] = "The primary author of kscript!"

assert len(x) == 4

# Re-assigning to an entry that already exists does NOT change the order!
x["name"] = "Cade Brown"


# You can iterate over a dict (or dict-like objects) by iterating over the entries:
for key, val in x {
    print ('{repr(key)}: {repr(val)}')
}

# Use '.keys()' to get a proxy list of keys
print (x.keys())
# Can iterate over it like a normal list
# NOTE: You can't modify '.keys()'!
for key in x.keys() {
    ...
}

# Similarly, there exists '.vals()' which gives a proxy list of values
print (x.vals())

# Even though the dictionaries have a different key order, they will compare equal!
assert {"a": 1, "b": 2} == {"b": 2, "a": 1}
