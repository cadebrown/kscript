#!/usr/bin/env ks

import os

if !(pid = os.fork()) {
    print(pid)
} else {
    print("I am the parent of %s" % (pid, ))
}

