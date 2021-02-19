#!/usr/bin/env ks
""" bitset.ks - example of using the 'util.Bitset' class

@author: Cade Brown <cade@kscript.org>
"""

# Extra utility types
import util


# Create from a list of integers
x = util.Bitset([1, 2, 3, 4])

# Or, equivalently, an integer with bits set
x = util.Bitset(0b11110)

# Prints the number of items
print (len(x))

