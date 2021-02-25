#!/usr/bin/env ks
""" tree.ks - Shows a tree of the filesystem, similar to the 'tree' command


@author: Cade Brown <cade@kscript.org>
"""

import os


# Dumps a path out, recursively, with tree-like output
func dump(path, disp=none, dep=0) {
    disp = disp || path
    ind = '│   ' * dep
    printf('%s\n', disp)

    (dirs, files) = os.listdir(path)
    for i in len(files) as range {
        printf('%s%s── %s\n', ind, '└' if i == len(files) - 1 else '├', files[i])
    }
    for i in len(dirs) as range {
        printf('%s%s── ', ind, '└' if i == len(dirs) - 1 else '├')
        dump(os.path(path) / dirs[i] as str, dirs[i], dep+1)
    }
}

# Dumps out current directory
dump('.')
