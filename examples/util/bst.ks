#!/usr/bin/env ks
""" examples/util/bst.ks - example of using 'util.bst', which is a binary search tree

@author: Cade Brown <cade@kscript.org>
"""

import { bst } from util

# Create from a dict mapping
x = bst({
    "greg": 0,
    "cade": 1,
})

# Or, equivalently, a key-val notation
x = bitset(("greg", 0), ("cade", 1))

# Prints the number of entries
print(len(x))

# Loops over the entries
for k, v in x {
    print('{k}: {v}')
}

