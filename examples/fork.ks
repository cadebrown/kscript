#!/usr/bin/env ks
""" fork.ks - Shows how to 'fork()' the process and launch another one (on Unix)


NOTE: Only available on UNIX-like OSes

@author: Gregory Croisdale <greg@kscript.org>
"""


import os

if pid = os.fork() {
    print("I am the parent of %s" % (pid, ))
} else {
    print("I am the child")
}

