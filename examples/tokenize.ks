#!/usr/bin/env ks
""" tokenize.ks - Example of applying tokenization to an input, and outputting a stream of tokens


@author: Cade Brown <cade@kscript.org>
"""

import os
import getarg
import gram

p = getarg.Parser("tokenize", "0.1.0", "Transforms the input into a stream of tokens", ["Cade Brown <cade@kscript.org>"])

p.flag("ascii", ["--ascii"], "Restrict token rules to ASCII-only")

args = p.parse()

# Define the kinds of tokens we will recognize
K = enum.make("K", [
    ('LPAR',           1),
    ('RPAR',           2),
    ('LBRK',           3),
    ('RBRK',           4),
    ('LBRC',           5),
    ('RBRC',           6),
    ('LARW',           7),
    ('RARW',           8),
    ('LT',             9),
    ('GT',            11),
    ('LE',            12),
    ('GE',            13),
    ('LTLT',          14),
    ('GTGT',          15),
    ('EQ',            16),
    ('NE',            17),
    ('EQEQ',          18),
    ('SEMI',          19),
    ('COLON',         20),
    ('COMMA',         21),
    ('DOT',           22),
    ('PLUS',          23),
    ('MINUS',         24),
    ('STAR',          25),
    ('SLASH',         26),
    ('PERCENT',       27),
    ('CARET',         28),
    ('PIPE',          29),
    ('PIPEPIPE',      30),
    ('AND',           31),
    ('ANDAND',        32),
    ('QUES',          33),
    ('EXCL',          34),

    # Values
    ('INT',          101),
    ('FLOAT',        102),
    ('STRING',       103),
    ('NAME',         104),

])

# Define the tokenizer/lexer, which is defined by regular expressions
#
# The lexer will take the rule with the longest match in the current input
#   stream, or of multiple rules match, it will use the earlier one
#
# SEE: https://en.wikipedia.org/wiki/Regular_expression
L = gram.Lexer([
    (`\(`, K.LPAR),
    (`\)`, K.RPAR),
    (`\[`, K.LBRK),
    (`\]`, K.RBRK),
    (`\{`, K.LBRC),
    (`\}`, K.RBRC),
    (`->`, K.LARW),
    (`<-`, K.RARW),
    (`<`,  K.LT),
    (`>`,  K.GT),
    (`<=`, K.LE),
    (`>=`, K.GE),
    (`<<`, K.LTLT),
    (`>>`, K.GTGT),
    (`=`,  K.EQ),
    (`!=`, K.NE),
    (`==`, K.EQEQ),
    (`\;`, K.SEMI),
    (`\:`, K.COLON),
    (`\,`, K.COMMA),
    (`\.`, K.DOT),

    (`\+`, K.PLUS),
    (`\-`, K.MINUS),
    (`\*`, K.STAR),
    (`\/`, K.SLASH),
    (`\%`, K.PERCENT),
    (`\^`, K.CARET),
    (`\|`, K.PIPE),
    (`\|\|`, K.PIPEPIPE),
    (`\&`, K.AND),
    (`\&\&`, K.ANDAND),

    (`\?`, K.QUES),
    (`\!`, K.EXCL),

    (`\d+`, K.INT),

    (`\d\.\d*|\.\d+`, K.FLOAT),

    (`"(\\.|[^"\\])*\"`, K.STRING),
    (`'(\\.|[^'\\])*\'`, K.STRING),
    (`\`(\\.|[^\`\\])*\``, K.STRING),
    
    (`[a-zA-Z_][a-zA-Z_0-9]*` if args.ascii else `[[: alpha :]_][[: alpha :]_[: digit :]]*`, K.NAME),


    # kscript/bash style comments
    (`#.*`, none),

    # C-style comments
    #(`/\*(\*[^/]|[^*])*\*/`, none),

    # C++ style comments
    #(`//.*`, none)

    # ignore space
    (`\s`, none),

    # (logical) line continuation
    (`\\\n`, none),
])

# run through the lexer
for tok in L {
    print (repr(tok))
    #print (str(tok.kind).split('.')[-1] + ':', repr(tok.val))
}
