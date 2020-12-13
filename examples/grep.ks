#!/usr/bin/env ks
""" grep.ks - implementation of the 'grep' program

Searches through files

@author: Cade Brown <cade@kscript.org>
"""

import os
import getarg

p = getarg.Parser("grep", "0.1.0", "Search through files and print lines which match a regular expression", ["Cade Brown <cade@kscript.org>"])

p.pos("expr", "Expression to use")
p.pos("files", "List of files to output", -1, os.path)
p.flag("recurse", ['-r', '--recurse'], "Recursively search subdirectories")
p.flag("all", ['-a', '--all'], "Search through all files, including hidden directories")

args = p.parse()

# Compile the regular expression that is being searched
pat = regex(args.expr)

func search(p) {
    # Ignore hidden files
    if !args.all {
        l = p.last()
        if len(l) > 1 && l.startswith('.'), ret
    }

    if p.isdir() {
       # (ds, fs) = p.listdir()
        # Check files
        for f in fs {
            search(p / f)
        }

        # Check subdirectories
        if args.recurse {
            for d in ds {
                search(p / d)
            }
        }
    } else {
        # Check lines of the file
        fp = open(p, 'r')
        prefix = str(p) + ':'

        for line in fp {
            if pat.matches(line) {
                print (prefix, line)
            }
        }
        close(fp)
    }
}

# Search all arguments
for p in map(os.path, args.files) {
    search(p)
}
