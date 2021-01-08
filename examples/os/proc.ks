#!/usr/bin/env ks

""" proc.ks - process management demo

@author: Gregory Croisdale <greg@kscript.org>
"""

import os

p = os.proc("yes")

p.kill()

print ("after")


"""p = os.proc("yes")

for i in range(1000) {}

print("")
print(p)

print(p.isalive())

print(p.signal(0))

print(p.isalive())

print(p.signal(9))

print(p.isalive())

p = os.proc("yes")

print(p.isalive())

print(p.join())

print(p.isalive())"""
