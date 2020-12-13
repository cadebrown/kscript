#!/usr/bin/env ks
""" qsort.ks - quicksort implementation

@author: Cade Brown <cade@kscript.org>
"""

import os
import getarg

p = getarg.Parser("qsort", "0.1.0", "Sorts ", ["Cade Brown <cade@kscript.org>"])

args = p.parse()

func qsort(vals) {
    if len(vals) < 2, ret vals

    piv = vals[0]
    L = list(filter(x -> x < piv, vals))
    R = list(filter(x -> x > piv, vals))

    ret qsort(L) + [piv] + qsort(R)
}

print (qsort(list(map(float, filter(os.stdin)))))
