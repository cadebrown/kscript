#!/usr/bin/env ks
""" calc.ks - (infix) calculator

This shows how to write an infix calculator, using more advanced parts of the 'gram' module

@author: Cade Brown <cade@kscript.org>
"""


import os
import getarg
import re
import gram

p = getarg.Parser("calc", "0.1.0", "Infix calculator", ["Cade Brown <cade@kscript.org>"])

# parse the args, throwing an error if something incorrect was given
# or, if `-h` or `--help` are given, print out a usage message and exit successfully
args = p.parse()

K = enum.make("K", [
    ('INT',       1),
    ('FLOAT',     2),
    ('STRING',    3),
    ('NAME',      4),

    ('LPAR',    100),
    ('RPAR',    101),
    ('LBRK',    102),
    ('RBRK',    103),
    ('LBRC',    104),
    ('RBRC',    105),

    ('DOT',     120),
    ('COMMA',   121),
    ('COLON',   123),
    ('SEMI',    124),

    ('PLUS',    200),
    ('MINUS',   201),
    ('STAR',    202),
    ('SLASH',   203),
])

L = gram.Lexer([
    # handle numbers
    (`0x[0-9a-fA-F]+`, K.INT),
    (`0o[0-7]+`, K.INT),
    (`0b[01]+`, K.INT),
    (`\d+`, K.INT),

    (`\d\.\d*`, K.FLOAT),
    (`\.\d+`, K.FLOAT),

    (`\"(\\.|[^"\\])*\"`, K.STRING),
    
    (`[[: alpha :]][[: alpha :]0-9]*`, K.NAME),

    (`\(`, K.LPAR),
    (`\)`, K.RPAR),
    (`\[`, K.LBRK),
    (`\]`, K.RBRK),
    (`\{`, K.LBRC),
    (`\}`, K.RBRC),

    (`\.`, K.DOT),
    (`\,`, K.COMMA),
    (`\:`, K.COLON),
    (`\;`, K.SEMI),

    (`\+`, K.PLUS),
    (`\-`, K.MINUS),
    (`\*`, K.STAR),
    (`\/`, K.SLASH),

    # ignore comment
    (`#.*`, none),

    # ignore space
    (`\s`, none),
])


# specify our grammar, which has rules, and a list of productions that the rule matches
# each production is a list of either strings (which name the rule it matches) or integral values
#   representing the token type (i.e. 'K.*' values)
# This grammar is a little awkard, because we can't use left recursion with a recursive descent parser
#   so we manually augment the grammar to allow for left factoring
# TODO: automate this with a function in 'gram'?
G = gram.Grammar({
    'PROG': [
        ['STMT', 'PROG'],
        []
    ],

    'STMT': [
        ['EXPR', K.SEMI],
        [K.SEMI],
    ],

    'EXPR': [
        ['E0'],
    ],

    'E0': [
        ['E1', 'E0_p'],
    ],
    
    'E0_p': [
        [K.PLUS, 'E1', 'E0_p'],
        [K.MINUS, 'E1', 'E0_p'],
        [],
    ],

    'E1': [
        ['E2', 'E1_p'],
    ],
    
    'E1_p': [
        [K.STAR, 'E2', 'E1_p'],
        [K.SLASH, 'E2', 'E1_p'],
        [],
    ],

    'E2': [
        ['E3', 'E2_p'],
    ],
    
    'E2_p': [
        [K.LPAR, 'ARGS', K.RPAR, 'E1_p'],
        [],
    ],

    'E3': [
        ['ATOM'],
    ],

    'ATOM': [
        [K.INT],
        [K.FLOAT],
        [K.STRING],
        [K.NAME],
        [K.LPAR, 'EXPR', K.RPAR],
    ],

    'ARGS': [
        ['EXPR', K.COMMA, 'ARGS'],
        ['EXPR'],
        [],
    ],
})

"""
G = gram.Grammar({
    'EXPR': [
        ['E0'],
    ],

    'E0': [
        ['E0', K.PLUS, 'E1'],
        ['E0', K.MINUS, 'E1'],
        ['E1'],
    ],

    'E1': [
        ['ATOM'],
    ],

    'ATOM': [
        [K.INT],
    ]
})
"""

# function to make prettier output for trees
func pprint(tree, condense=true, ind='') {
    while condense && type(tree) == tuple && len(tree[1]) == 1 {
        tree = tree[1][0]
    }

    if type(tree) == gram.Token {
        print(ind + repr(tree))
        #print(ind + repr(tree.val))
    } else {
        if !condense || len(tree[1]) > 0 {
            print(ind + tree[0])
            for sub in tree[1] {
                pprint(sub, condense, ind + '  ')
            }
        }
    }
}



# This is where things get difficult. We need to generate a parser for our language. The grammar is abstract,
#   but the parser will have to generate tables/specific algorithms for the grammar
P = gram.ParserRD(G)
tree = P.parse(L)

pprint(tree, false)




"""

# function to evaluate a parse tree
func run(tree) {
    if type(tree) == gram.Token {
        if tree.kind == K.INT {
            ret int(str(tree))
        } else {
        }
    } elif tree[0] == 'PROG' {
        ret [*map(run, tree[1])]
    } elif tree[0] == 'STMT' {
        ret [*map(run, tree[1])]
    } elif tree[0] == 'EXPR' {
        ret run(tree[1][0])
    } elif tree[0] == 'E0' {
        lhs = run(tree[1][0])
        rhs = tree[1][1][1]
        if rhs {
            print (rhs)
            ret lhs + run(rhs[1])
        } else {
            ret lhs
        }
    } elif tree[0] == 'E1' {
        ret run(tree[1][0])
    } elif tree[0] == 'E2' {
        ret run(tree[1][0])
        ret [*map(run, tree[1])]
    } elif tree[0] == 'E3' {
        ret run(tree[1][0])
        ret [*map(run, tree[1])]
    } elif tree[0] == 'ATOM' {
        ret run(tree[1][0])
        ret [*map(run, tree[1])]
    }
}


print (run(tree))

"""
