#!/usr/bin/env ks
""" examples/m/trig.ks - example of trigonometric functions

@author: Cade Brown <cade@kscript.org>
"""

import { tau, sin, cos, tan } from m

# How many iterations per loop (evenly around a circle)
N = 8

# Print some basic trig values
for theta in map(range(N), x -> x * (tau / N)) {
    print(
        'sin({theta}) == {sin(theta)}', 
        'cos({theta}) == {cos(theta)}', 
        'tan({theta}) == {tan(theta)}'
    )
}
