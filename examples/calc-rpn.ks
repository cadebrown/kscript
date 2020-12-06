#!/usr/bin/env ks
""" calc-rpn.ks - RPN calculator

This shows an example of how to write an RPN calculator, using the builtin modules

@author: Cade Brown <cade@kscript.org>
"""

import os
import getarg
import re
import gram

p = getarg.Parser("calc-rpn", "0.1.0", "RPN-style calculator", ["Cade Brown <cade@kscript.org>"])

# parse the args, throwing an error if something incorrect was given
# or, if `-h` or `--help` are given, print out a usage message and exit successfully
args = p.parse()


# -- Custom Functions (operate on stack) --


func my_print(stk) {
    x = stk.pop()
    print (x)
}

# -- --

stkstk = [[]]
vars = {
    "print": my_print
}

L = gram.Lexer([
    # handle numbers
    (`\d+`, s       -> stkstk[-1].push(int(s))),
    (`\d+\.\d+`, s  -> stkstk[-1].push(float(s))),
    (`\d+\.\d+i`, s -> stkstk[-1].push(complex(s))),

    # variables
    (`[[:alpha:]_][[:alpha:]_[:digit:]]*`, s -> stkstk[-1].push(vars[s])),

    # operations
    ("+", _  -> stkstk[-1].push(stkstk[-1].pop(-2) +  stkstk[-1].pop())),
    ("-", _  -> stkstk[-1].push(stkstk[-1].pop(-2) -  stkstk[-1].pop())),
    ("*", _  -> stkstk[-1].push(stkstk[-1].pop(-2) *  stkstk[-1].pop())),
    ("/", _  -> stkstk[-1].push(stkstk[-1].pop(-2) /  stkstk[-1].pop())),
    ("%", _  -> stkstk[-1].push(stkstk[-1].pop(-2) %  stkstk[-1].pop())),
    ("^", _  -> stkstk[-1].push(stkstk[-1].pop(-2) ** stkstk[-1].pop())),
    ("<", _  -> stkstk[-1].push(stkstk[-1].pop(-2) <  stkstk[-1].pop())),
    ("<=", _ -> stkstk[-1].push(stkstk[-1].pop(-2) <= stkstk[-1].pop())),
    (">", _  -> stkstk[-1].push(stkstk[-1].pop(-2) >  stkstk[-1].pop())),
    (">=", _ -> stkstk[-1].push(stkstk[-1].pop(-2) >= stkstk[-1].pop())),
    ("==", _ -> stkstk[-1].push(stkstk[-1].pop(-2) == stkstk[-1].pop())),
    ("!=", _ -> stkstk[-1].push(stkstk[-1].pop(-2) != stkstk[-1].pop())),
    ("()", _ -> stkstk[-1].push(stkstk[-1].pop()(stkstk[-1]))),

    ("[", _ -> stkstk.push([])),
    ("]", _ -> stkstk[-2].push(stkstk.pop())),

    # skip whitespace
    (`\s`, none)
])

# run through the lexer
for tok in L {
    print ("TOK:", tok) 
}

print (stkstk[-1])
