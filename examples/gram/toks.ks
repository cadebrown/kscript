#!/usr/bin/env ks
""" examples/gram/toks.ks - example of using 'gram' module to tokenize a stream

@author: Cade Brown <cade@kscript.org>
"""

import { stdin } from os
import { Lexer } from gram

# Define the kinds of tokens that can be encountered
enum K {
    LPAR
    RPAR
    LBRK
    RBRK
    LBRC
    RBRC
    LARW
    RARW
    LT
    GT
    LE
    GE
    LTLT
    GTGT
    EQ
    NE
    EQEQ
    SEMI
    COLON
    COMMA
    DOT
    PLUS
    MINUS
    STAR
    SLASH
    PERCENT
    CARET
    PIPE
    PIPEPIPE
    AND
    ANDAND
    QUES
    EXCL

    INT
    FLOAT
    STRING
    NAME
}

# Create a lexer from regex patterns
L = Lexer({
    `\(`: K.LPAR,
    `\)`: K.RPAR,
    `\[`: K.LBRK,
    `\]`: K.RBRK,
    `\{`: K.LBRC,
    `\}`: K.RBRC,
    `->`: K.LARW,
    `<-`: K.RARW,
    `<`:  K.LT,
    `>`:  K.GT,
    `<=`: K.LE,
    `>=`: K.GE,
    `<<`: K.LTLT,
    `>>`: K.GTGT,
    `=`:  K.EQ,
    `!=`: K.NE,
    `==`: K.EQEQ,
    `\;`: K.SEMI,
    `\:`: K.COLON,
    `\,`: K.COMMA,
    `\.`: K.DOT,

    `\+`: K.PLUS,
    `\-`: K.MINUS,
    `\*`: K.STAR,
    `\/`: K.SLASH,
    `\%`: K.PERCENT,
    `\^`: K.CARET,
    `\|`: K.PIPE,
    `\|\|`: K.PIPEPIPE,
    `\&`: K.AND,
    `\&\&`: K.ANDAND,

    `\?`: K.QUES,
    `\!`: K.EXCL,

    `\d+`: K.INT,

    `\d\.\d*|\.\d+`: K.FLOAT,

    `"(\\.|[^"\\])*\"`: K.STRING,
    `'(\\.|[^'\\])*\'`: K.STRING,
    `\`(\\.|[^\`\\])*\``: K.STRING,
    
    `[[: alpha :]_][[: alpha :]_[: digit :]]*`: K.NAME,

    # kscript/bash style comments
    `#.*`: none,

    # C-style comments
    #`/\*(\*[^/]|[^*])*\*/`: none,

    # C++ style comments
    #`//.*`: none,

    # ignore space
    `\s`: none,

    # (logical) line continuation
    `\\\n`: none,
})

# Now, iterate over the lexer, applied to 'stdin'
# This will convert the input stream to tokens, with annotated metadata,
#   according to the regex definitions. When multiple rules match,
#   the lexer uses the one with the longest match, or the first rule defined
for tok in L(stdin) {
    print(repr(tok))
}
