#!/usr/bin/env ks
""" examples/iters.ks - examples of builtin iterables


@author: Cade Brown <cade@kscript.org>
"""

# We can use 'range' to produce a range of integers
# NOTE: range(10) == range(0, 10) == range(0, 10, 1)
# The full form is 'range(start, stop, step)'
for i in range(10) {
    print ('{i}')
}

# We can use 'batch(it, num)' to turn 'it' (another iterable) into groups
#   of 'num'
for x, y in batch(range(10), 2) {
    print ('{x}, {y}')
}

# We can also use 'unbach(it)' to expand an iterable into a single linear list
# For example, it can take a list-of-lists and make it into a single list, efficiently
#    (it also works with any iterable!)
for i in unbatch([(0, 1), (2, 3, 4), (5,), (6, 7, 8, 9)]) {
    print (i)
}

# We can use 'zip' to combine 2 iterables into many tuples containing 1 from each iterable
#   at a time
for a, i in zip(["a", "b", "c"], [0, 1, 2]) {
    print ('{repr(a)}, {i}')
}


