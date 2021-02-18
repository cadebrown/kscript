#!/usr/bin/env ks
""" qsort.ks - quicksort implementation

Not extremely efficient -- but some call it beautiful!

@author: Cade Brown <cade@kscript.org>
"""

import os
import getarg

p = getarg.Parser("qsort", "0.1.0", "Sorts standard input according to the quicksort algorithm", ["Cade Brown <cade@kscript.org>"])

args = p.parse()

func qsort(vals) {
    if len(vals) < 2, ret vals

    piv = vals[0]
    L = list(filter(x -> x < piv, vals))
    M = list(filter(x -> x == piv, vals))
    R = list(filter(x -> x > piv, vals))

    ret qsort(L) + M + qsort(R)
}

inp = map(float, filter(os.stdin)) as list
print (qsort(inp))

