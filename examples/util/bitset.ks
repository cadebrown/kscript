#!/usr/bin/env ks
""" examples/util/bitset.ks - example of using 'util.bitset', which is a set of unique, positive integers

@author: Cade Brown <cade@kscript.org>
"""

import { bitset } from util

# Create from a list of integers
x = bitset([1, 2, 3, 4])

# Or, equivalently, an integer with bits set
x = bitset(0b11110)

# Prints the number of items
print(len(x))

# Loops over the bits set
for i in x {
    print(i)
}

