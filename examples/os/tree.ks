#!/usr/bin/env ks
""" examples/os/tree.ks - example of a 'tree' utility to create a tree view of a directory


TODO: 

@author: Cade Brown <cade@kscript.org>
"""

import { argv, ls } from os

func dump(path, ind='') {
    # Query the path for all contents
    files, dirs = ls(path)

    for sub in files {
        print('{ind}+-{sub}')
    }

    for sub in dirs {
        print('{ind}+-{sub}')
        dump(os.path(path) / sub, ind + '  ')
    }
}

# Dump out the first argument (or, if there was an error, use the current directory)
dump(argv[1] ?? '.')
