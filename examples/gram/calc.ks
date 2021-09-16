#!/usr/bin/env ks
""" examples/gram/calc.ks - example of using 'gram' module to create a basic calculator

Uses the Lexer type, to turn a text-based input stream into a stream of tokens
Then, uses the RDParser (Recursive Descent Parser) along with a basic rule set to define
  a programmatic parser (functions given to rules are executed with either the token, if the
  rule was just a token type, or the value of whatever other function was ran to get the value)

If you just want a parse tree, check out 'gram.Tree' (you can just pass it as the function,
  its constructor will accept tokens and build a parse tree automatically!)

We also use the rules 'gram.Any' and 'gram.Rep', which will match any of the child rules, or
  any number (including 0) repetition of the rules (and yields a list of all times it was returned)


@author: Cade Brown <cade@kscript.org>
"""

import { stdin } from os
import { Lexer, RDParser, Any, Rep} from gram

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

# Create AST types
type AST {
    func __init(self, sub, val=none) {
        self.sub = sub
        self.val = val
    }
    
    func __repr(self) {
        ret '{type(self)}({", ".join(self.sub)}, {self.val})'
    }

    func __str(self) {
        ret repr(self)
    }

    func __call(self, ctx) {
        throw Error("Not implemented!")
    }
}


# <val>
type ValAST {
    func __call(self, ctx) {
        ret self.val
    }
}


# <name ref>
type NameAST {
    func __call(self, ctx) {
        # Name is the value
        name = self.val

        # Return the value we have for the name
        ret ctx[name]
    }
}

# <assignment>
type AssignAST {
    func __call(self, ctx) {
        # TODO:  implement context saving
        L, R = self.sub

        # Calculate the value
        val = R(ctx)
        if L is NameAST {
            # Assign to name
            ctx[L.val] = val 
        } else {
            throw Error("Bad type of AST for assignment")
        }

        # Also, yield the value
        ret val
    }
}

# L + R
type AddAST {
    func __call(self, ctx) {
        L, R = self.sub
        ret L(ctx) + R(ctx)
    }
}

# L - R
type SubAST {
    func __call(self, ctx) {
        L, R = self.sub
        ret L(ctx) - R(ctx)
    }
}

# L * R
type MulAST {
    func __call(self, ctx) {
        L, R = self.sub
        ret L(ctx) * R(ctx)
    }
}

# L / R
type DivAST {
    func __call(self, ctx) {
        L, R = self.sub
        ret L(ctx) / R(ctx)
    }
}

# L // R
type FloordivAST {
    func __call(self, ctx) {
        L, R = self.sub
        ret L(ctx) // R(ctx)
    }
}

# L % R
type DivAST {
    func __call(self, ctx) {
        L, R = self.sub
        ret L(ctx) % R(ctx)
    }
}

# L ** R
type PowAST {
    func __call(self, ctx) {
        L, R = self.sub
        ret L(ctx) ** R(ctx)
    }
}

# L || R
type OrAST {
    func __call(self, ctx) {
        L, R = self.sub

        # Try left, try to short circuit
        val = L(ctx)
        if val, ret val

        # Otherwise, run the right
        ret R(ctx)
    }
}

# L && R
type AndAST {
    func __call(self, ctx) {
        L, R = self.sub

        # Try left, try to short circuit
        val = L(ctx)
        if !val, ret val

        # Otherwise, run the right
        ret R(ctx)
    }
}

# L == R
type EqAST {
    func __call(self, ctx) {
        L, R = self.sub
        ret L(ctx) == R(ctx)
    }
}

# L != R
type NeAST {
    func __call(self, ctx) {
        L, R = self.sub
        ret L(ctx) != R(ctx)
    }
}

# L < R
type LtAST {
    func __call(self, ctx) {
        L, R = self.sub
        ret L(ctx) < R(ctx)
    }
}

# L <= R
type LeAST {
    func __call(self, ctx) {
        L, R = self.sub
        ret L(ctx) <= R(ctx)
    }
}

# L > R
type GtAST {
    func __call(self, ctx) {
        L, R = self.sub
        ret L(ctx) > R(ctx)
    }
}

# L >= R
type GeAST {
    func __call(self, ctx) {
        L, R = self.sub
        ret L(ctx) >= R(ctx)
    }
}

# L << R
type LshAST {
    func __call(self, ctx) {
        L, R = self.sub
        ret L(ctx) << R(ctx)
    }
}

# L >> R
type RshAST {
    func __call(self, ctx) {
        L, R = self.sub
        ret L(ctx) >> R(ctx)
    }
}

# L | R
type BinOrAST {
    func __call(self, ctx) {
        L, R = self.sub
        ret L(ctx) | R(ctx)
    }
}

# L & R
type BinAndAST {
    func __call(self, ctx) {
        L, R = self.sub
        ret L(ctx) & R(ctx)
    }
}

# L ^ R
type BinXorAST {
    func __call(self, ctx) {
        L, R = self.sub
        ret L(ctx) ^ R(ctx)
    }
}


# Fold left binary operator
func fold_lbop(L, Rs) {
    # Start result as the left hand side
    res = L

    # Iterate over the right-hand-sides, and apply
    for op, R in batch(Rs, 2) {
        # Get the operator function
        fn = fnmap[op]

        # Now, apply it
        res = fn(res, R)
    }

    # Return built tree
    ret res
}


# Fold right binary operator
func fold_rbop(Ls, R) {
    # Start result as the right hand side
    res = R

    # Iterate over the right-hand-sides, and apply
    for L, op in batch(Ls, 2)[::-1] {
        # Get the operator function
        fn = fnmap[op]

        # Now, apply it with children
        res = fn([res, R])
    }

    # Return built tree
    ret res
}


# Mapping token types to AST types, for folding binary operators
fnmap = {
    K.EQ:       AssignAST,
    K.PIPEPIPE: OrAST,
    K.ANDAND:   AndAST,
    K.EQEQ:     EqAST,
    K.NE:       NeAST,
    K.LT:       LtAST,
    K.GT:       GtAST,
    K.LE:       LeAST,
    K.GE:       GeAST,
    K.LTLT:     LshAST,
    K.GTGT:     GshAST,

    K.PIPE:     BinOrAST,
    K.AND:      BinAndAST,
    K.CARET:    BinXorAST,

    K.PLUS:     AddAST,
    K.MINUS:    SubAST,
    K.STAR:     MulAST,
    K.SLASH:    DivAST,
    K.SLASHSLASH: FloordivAST,
    K.PERCENT:  ModAST,
}


# Create a parser (specifically, a Recursive Descent parser (RDParser))
# RDParser is given a dict that describes the ruleset (i.e. the EBNF) of the language
#   we are recognizing
# Each entry in the rule set has the string name of the rule as the key
P = RDParser({
    'E0': {
        [Rep(['E1', K.EQ]), 'E1']: fold_rbop,
    },
    'E1': {
        ['E2', Rep([K.PIPEPIPE, 'E2'])]: fold_lbop,
    },
    'E2': {
        ['E3', Rep([K.ANDAND, 'E3'])]: fold_lbop,
    },
    'E3': {
        ['E4', Rep([Any([K.EQEQ, K.NE, K.LT, K.GT, K.LE, K.GE]), 'E4'])]: fold_lbop,
    },
    'E4': {
        ['E5', Rep([K.PIPE, 'E5'])]: fold_lbop,
    },
    'E5': {
        ['E6', Rep([K.AND, 'E6'])]: fold_lbop,
    },
    'E6': {
        ['E7', Rep([K.CARET, 'E7'])]: fold_lbop,
    },
    'E7': {
        ['E8', Rep([Any([K.LTLT, K.GTGT]), 'E8'])]: fold_lbop,
    },
    'E8': {
        ['E9', Rep([Any([K.PLUS, K.MINUS]), 'E9'])]: fold_lbop,
    },
    'E9': {
        ['E10', Rep([Any([K.STAR, K.SLASH, K.SLASHSLASH, K.PERCENT]), 'E10'])]: fold_lbop,
    },
    'E10': {
        'ATOM': val -> val,
    },

    'ATOM': {
        Any(['NAME', 'INT', 'FLOAT']): val -> val,
    },

    'NAME': {
        [K.NAME]: tok -> NameAST(str(tok)),
    },
    'INT': {
        [K.INT]: tok -> ValAST(int(str(tok))),
    },
    'FLOAT': {
        [K.FLOAT]: tok -> ValAST(float(str(tok))),
    },
})


# Create token iterable
toks = L(stdin)

# Context used for ASTs
ctx = {
    "a": 0,
    "b": 1,
    "c": 2,
}

# Now, iterate over 'E0's parsed in the tokens array
for ast in P('E0', toks, ctx) {

    # Call the AST, to get a value
    val = ast(ctx)

    # Now, evaluate it
    print('= {repr(val)}')
}
