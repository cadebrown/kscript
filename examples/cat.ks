#!/usr/bin/env ks
""" examples/cat.ks - implementation of the 'cat' program, which concatenates output from multiple files


@author: Cade Brown <cade@kscript.org>
"""

import { argv } from os

# Iterate through all files given as commandline arguments
for file in argv[1:] {
    # Using 'with' will automatically close the file
    with fp = open(file, 'r') {
        for line in fp {
            print(line)
        }
    }
}
