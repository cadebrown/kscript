#!/usr/bin/env ks
""" t_graph.ks - Testing the 'graph' type

@author: Cade Brown <cade@kscript.org>
"""

A = 'A'
B = 'B'
C = 'C'

g0 = graph([A, B, C], [(0, 1), (1, 2), (2, 0)])

print (g0)

print (graph(object))



