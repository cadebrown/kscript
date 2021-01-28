#!/usr/bin/env ks
""" cat.ks - implementation of the 'cat' program

@author: Cade Brown <cade@kscript.org>
"""

import os
import getarg

p = getarg.Parser("cat", "0.1.0", "Concatenate contents of files", ["Cade Brown <cade@kscript.org>"])

p.pos("files", "List of files to output", -1)

args = p.parse()

for file in args.files {
    fp = open(file, 'r')

    for line in fp {
        print(line)
    }

    fp.close()
}
