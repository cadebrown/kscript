/* parser.c - implements the kscript compiler, which turns tokens into ASTs
 *
 * The internal implementation uses recursive descent parsing, on the grammar (given below in this file).
 *   The grammar is very close to an expression grammar completely, but still has some statement constructs
 * 
 * @author: Cade Brown <cade@kscript.org>
 */
#include <ks/impl.h>
#include <ks/compiler.h>
#include <ks/ucd.h>


/** Utilities **/

/* Dictionary of keyword constants */
static ks_dict kwconst = NULL;

/* Ast representing 'none' */
static ks_ast ast_none = NULL;

ks_Exception ks_syntax_error(ks_str fname, ks_str src, ks_tok tok, const char* fmt, ...) {
    ksio_StringIO sio = ksio_StringIO_new();

    va_list ap;
    va_start(ap, fmt);
    bool r = ksio_addv((ksio_BaseIO)sio, fmt, ap);
    assert(r);
    va_end(ap);

    ksio_addbuf((ksio_BaseIO)sio, 1, "\n");

    ks_tok_add((ksio_BaseIO)sio, fname, src, tok, true);

    ks_Exception res = ks_Exception_new_c(kst_SyntaxError, NULL, NULL, -1, "%.*s", (int)sio->len_b, sio->data);
    KS_DECREF(sio);
    return res;
}


/* kscript is best defined as an EBNF-like grammar, with precedence by order (similar to PEG grammars)
 * 
 * PROG    : STMT*
 * 
 * STMT    : 'import' NAME N
 *         | 'ret' EXPR? N
 *         | 'throw' EXPR? N
 *         | 'break' INT? N
 *         | 'cont' INT? N
 *         | 'if' EXPR BORC ('elif' BORC)* ('else' EXPR BORS)?
 *         | 'while' EXPR BORC ('else' EXPR BORS)?
 *         | 'for' EXPR BORC
 *         | 'try' BORS ('catch' EXPR (('as' | '->') EXPR)?)* ('finally' BORS)?
 *         | N
 *         | EXPR N
 * 
 * EXPR    : E0
 * 
 * (* Block *)
 * B       : '{' STMT* '}'
 * 
 * (* Block or comma statement *)
 * BORC    : B
 *         | ',' STMT
 * 
 * (* Block or statement (no comma needed) *)
 * BORS    : b
 *         | STMT
 * 
 * (* Newline/break *)
 * N       : '\n'
 *         | ';'
 * 
 * (* Precedence rules *)
 * E0      : E1 '=' E0
 *         | E1 '&=' E0
 *         | E1 '^=' E0
 *         | E1 '|=' E0
 *         | E1 '<<=' E0
 *         | E1 '>>=' E0
 *         | E1 '+=' E0
 *         | E1 '-=' E0
 *         | E1 '*=' E0
 *         | E1 '@=' E0
 *         | E1 '/=' E0
 *         | E1 '//=' E0
 *         | E1 '%=' E0
 *         | E1 '**=' E0
 *         | E1
 * 
 * E1      : E2 'if' E2 ('else' E1)?
 *         | E2
 * 
 * E2      : E2 '??' E3
 *         | E3
 * 
 * E3      : E2 '||' E3
 *         | E3
 * 
 * E4      : E4 '&&' E5
 * 
 * E5      : E5 '===' E6
 *         | E5 '==' E6
 *         | E5 '!=' E6
 *         | E5 '<' E6
 *         | E5 '<=' E6
 *         | E5 '>' E6
 *         | E5 '>=' E6
 *         | E5 'in' E6
 *         | E5 '!in' E6
 *         | E6
 * 
 *         | E7
 * 
 * E6      : E6 '|' E7
 *         | E7
 * 
 * E7      : E7 '^' E8
 *         | E8
 * 
 * E8      : E8 '&' E9
 *         | E9
 * 
 * E9      : E9 '<<' E10
 *         | E9 '>>' E10
 *         | E10
 * 
 * E10     : E10 '+' E11
 *         | E10 '-' E11
 *         | E11
 * 
 * E11     : E11 '*' E12
 *         | E11 '@' E12
 *         | E11 '/' E12
 *         | E11 '//' E12
 *         | E11 '%' E12
 *         | E12
 * 
 * E12     : E13 '**' E12
 *         | E13
 * 
 * E13     : '++' E13
 *         | '--' E13
 *         | '+' E13
 *         | '-' E13
 *         | '~' E13
 *         | '!' E13
 *         | '?' E13
 *         | E15
 * 
 * E14     : ATOM
 *         | '(' ')'
 *         | '[' ']'
 *         | '{' '}'
 *         | '(' ELEM (',' ELEM)* ','? ')'
 *         | '[' ELEM (',' ELEM)* ','? ']'
 *         | '{' ELEM (',' ELEM)* ','? '}'
 *         | '{' ELEMKV (',' ELEMKV)* ','? '}'
 *         | 'func' NAME? ('(' (PAR (',' PAR)* ','?)? ')')? B
 *         | 'type' NAME? ('extends' EXPR)? B
 *         | E14 '.' NAME
 *         | E14 '++'
 *         | E14 '--'
 *         | E14 '(' (ARG (',' ARG)*)? ','? ')'
 *         | E14 '[' (ARG (',' ARG)*)? ','? ']'
 * 
 * ATOM    : NAME
 *         | STR
 *         | REGEX
 *         | INT
 *         | FLOAT
 *         | '...'
 * 
 * ARG     : '*' EXPR
 *         | EXPR
 * 
 * ELEM    : '*' EXPR
 *         | EXPR
 * 
 * ELEMKV  : EXPR ':' EXPR
 * 
 * PAR     : '*' NAME
 *         | 
 * 
 * NAME    : ? unicode identifier ?
 * STR     : ? string literal ?
 * REGEX   : ? regex literal ?
 * INT     : ? integer literal ?
 * FLOAT   : ? floating point literal ?
 * 
 */

/* Parser flags, which affect how the parser treats certain constructs */
enum {
    PF_NONE   = 0x00,

    /* Do not consume any 'in' tokens at the highest level */
    PF_NO_IN  = 0x01,

    /* Do not consume any 'as' tokens at the highest level */
    PF_NO_AS  = 0x02,

};

/** Utilities **/

/* Token index */
#define toki (*tokip)

/* Current token */
#define TOK (toks[toki])

/* Check if a token equals a string literal */
#define TOK_EQ(_tok, _str) ((_tok).epos - (_tok).spos == (sizeof(_str) - 1) && strncmp(src->data + (_tok).spos, _str, sizeof(_str) - 1) == 0)

/* Check if a token is a specific C-string */
#define TOKIS(_str) TOK_EQ(TOK, _str)

/* Eat/skip the next token, yielding that token */
#define EAT() toks[toki++]

/* Check whether we are done yet */
#define DONE (toki >= n_toks - 1)

/* Match another rule (recursively) */
#define SUBF(_name, _flags) R_##_name(fname, src, n_toks, toks, tokip, _flags)
#define SUB(_name) SUBF(_name, flags)

/* Declare a rule for the recursive-descent parser */
#define RULE(_name) static ks_ast R_##_name(ks_str fname, ks_str src, ks_ssize_t n_toks, ks_tok* toks, int* tokip, int flags)

/* Skip line breaks */
#define SKIP_N() do { \
    while (!DONE && TOK.kind == KS_TOK_N) { \
        EAT(); \
    } \
} while (0)


/* Forward declarations */
RULE(PROG);
RULE(STMT);
RULE(B);
RULE(BORS);
RULE(BORC);
RULE(ATOM);

RULE(EXPR);
RULE(E0);
RULE(E1);
RULE(E1p);
RULE(E2);
RULE(E3);
RULE(E4);
RULE(E5);
RULE(E6);
RULE(E7);
RULE(E8);
RULE(E9);
RULE(E10);
RULE(E11);
RULE(E12);
RULE(E13);
RULE(E14);

/* Take a break/newline and return success */
static bool R_N(ks_str fname, ks_str src, ks_ssize_t n_toks, ks_tok* toks, int* tokip, int flags) {
    int k = TOK.kind;
    if (k == KS_TOK_N) {
        EAT();
        return true;
    } else if (k == KS_TOK_SEMI) {
        EAT();
        return true;
    } else if (k == KS_TOK_EOF || DONE) {
        return true;
    } else if (k == KS_TOK_RBRC || k == KS_TOK_RBRK || k == KS_TOK_RPAR) {
        return true;
    }

    KS_THROW_SYNTAX(fname, src, TOK, "Unexpected token, expected ';', newline, or EOF after");
    return false;
}


RULE(PROG) {
    ks_ast res = ks_ast_new(KS_AST_BLOCK, 0, NULL, KSO_NONE, KS_TOK_MAKE_EMPTY());

    SKIP_N();
    while (!DONE) {
        ks_ast sub = SUB(STMT);
        if (!sub) {
            KS_DECREF(res);
            return NULL;
        }

        ks_ast_pushn(res, sub);
        SKIP_N();
    }

    return res;
}

RULE(STMT) {
    ks_tok t = TOK;
    int k = t.kind;

    /* Check lookahead and see if it is a special construct */
    if (k == KS_TOK_BREAK) {
        EAT();

        if (!SUB(N)) {
            return NULL;
        }

        return ks_ast_newn(KS_AST_BREAK, 0, NULL, NULL, t);
    } else if (k == KS_TOK_CONT) {
        EAT();

        if (!SUB(N)) {
            return NULL;
        }

        return ks_ast_newn(KS_AST_CONT, 0, NULL, NULL, t);
    } else if (k == KS_TOK_RET) {
        EAT();

        if (TOK.kind == KS_TOK_SEMI || TOK.kind == KS_TOK_EOF || TOK.kind == KS_TOK_N) {
            if (!SUB(N)) return NULL;
            return ks_ast_new(KS_AST_RET, 1, &ast_none, NULL, t);
        } else {
            ks_ast sub = SUB(EXPR);
            if (!sub) return NULL;

            if (!SUB(N)) {
                KS_DECREF(sub);
                return NULL;
            }

            return ks_ast_newn(KS_AST_RET, 1, &sub, NULL, t);
        }
    } else if (k == KS_TOK_THROW) {
        EAT();

        if (TOK.kind == KS_TOK_SEMI || TOK.kind == KS_TOK_EOF || TOK.kind == KS_TOK_N) {
            if (!SUB(N)) return NULL;
            return ks_ast_new(KS_AST_THROW, 1, &ast_none, NULL, t);
        } else {
            ks_ast sub = SUB(EXPR);
            if (!sub) return NULL;

            if (!SUB(N)) {
                KS_DECREF(sub);
                return NULL;
            }

            return ks_ast_newn(KS_AST_THROW, 1, &sub, NULL, t);
        }
    } else if (k == KS_TOK_ASSERT) {
        EAT();

        ks_ast sub = SUB(EXPR);
        if (!sub) return NULL;

        if (!SUB(N)) {
            KS_DECREF(sub);
            return NULL;
        }

        return ks_ast_newn(KS_AST_ASSERT, 1, &sub, NULL, t);

    } else if (k == KS_TOK_IMPORT) {
        EAT();
        if (TOK.kind != KS_TOK_NAME) {
            KS_THROW_SYNTAX(fname, src, TOK, "Expected a valid module name for 'import' statement");
            return NULL;
        }
        ksio_StringIO sio = ksio_StringIO_new();
        ks_tok tt = TOK;

        ks_str s = ks_tok_str(src, EAT());
        ksio_add(sio, "%S", s);
        KS_DECREF(s);


        while (TOK.kind == KS_TOK_DOT) {
            EAT();

            if (TOK.kind != KS_TOK_NAME) {
                KS_THROW_SYNTAX(fname, src, TOK, "Expected a valid module name for 'import' statement");
                return NULL;
            }
            tt = TOK;

            s = ks_tok_str(src, EAT());
            ksio_add(sio, ".%S", s);
            KS_DECREF(s);
        }

        return ks_ast_newn(KS_AST_IMPORT, 0, NULL, (kso)ksio_StringIO_getf(sio), ks_tok_combo(t, tt));
        
    } else if (k == KS_TOK_IF) {
        EAT();

        ks_ast cond = SUB(EXPR);
        if (!cond) return NULL;

        ks_ast body = SUB(BORC);
        if (!body) {
            KS_DECREF(cond);
            return NULL;
        }

        ks_ast res = ks_ast_newn(KS_AST_IF, 2, (ks_ast[]){ cond, body }, NULL, t);

        /* Deepest node, which we should added clauses to */
        ks_ast deep = res;

        while (TOK.kind == KS_TOK_ELIF) {
            t = EAT();
            
            cond = SUB(EXPR);
            if (!cond) {
                KS_DECREF(res);
                return NULL;
            }

            body = SUB(BORC);
            if (!body) {
                KS_DECREF(cond);
                KS_DECREF(res);
                return NULL;
            }

            /* Construct branch of the deepest tree */
            ks_ast other = ks_ast_newn(KS_AST_IF, 2, (ks_ast[]){ cond, body }, NULL, t);
            ks_ast_pushn(deep, other);
            deep = other;

        }

        if (TOK.kind == KS_TOK_ELSE) {
            EAT();

            ks_ast other = SUB(BORS);
            if (!other) {
                KS_DECREF(res);
                return NULL;
            }

            ks_ast_pushn(deep, other);
        }

        return res;

    } else if (k == KS_TOK_WHILE) {
        EAT();

        ks_ast cond = SUB(EXPR);
        if (!cond) return NULL;

        ks_ast body = SUB(BORC);
        if (!body) {
            KS_DECREF(cond);
            return NULL;
        }

        ks_ast res = ks_ast_newn(KS_AST_WHILE, 2, (ks_ast[]){ cond, body }, NULL, t);

        /* Deepest node, which we should added clauses to */
        ks_ast deep = res;

        while (TOK.kind == KS_TOK_ELIF) {
            t = EAT();
            
            cond = SUB(EXPR);
            if (!cond) {
                KS_DECREF(res);
                return NULL;
            }

            body = SUB(BORC);
            if (!body) {
                KS_DECREF(cond);
                KS_DECREF(res);
                return NULL;
            }

            /* Construct branch of the deepest tree */
            ks_ast other = ks_ast_newn(KS_AST_IF, 2, (ks_ast[]){ cond, body }, NULL, t);
            ks_ast_pushn(deep, other);
            deep = other;

        }

        if (TOK.kind == KS_TOK_ELSE) {
            EAT();

            ks_ast other = SUB(BORS);
            if (!other) {
                KS_DECREF(res);
                return NULL;
            }

            ks_ast_pushn(deep, other);
        }

        return res;

    } else if (k == KS_TOK_FOR) {
        EAT();

        ks_ast to = SUBF(EXPR, PF_NO_IN);
        if (!to) return NULL;

        if (TOK.kind != KS_TOK_IN) {
            KS_THROW_SYNTAX(fname, src, TOK, "Expected 'in' here for for-loop");
            KS_DECREF(to);
            return NULL;
        }
        EAT();

        ks_ast it = SUB(EXPR);
        if (!it) {
            KS_DECREF(to);
            return NULL;
        }


        ks_ast body = SUB(BORC);
        if (!body) {
            KS_DECREF(to);
            KS_DECREF(it);
            return NULL;
        }

        ks_ast res = ks_ast_newn(KS_AST_FOR, 3, (ks_ast[]){ to, it, body }, NULL, t);

        /* Deepest node, which we should added clauses to */
        ks_ast deep = res;

        while (TOK.kind == KS_TOK_ELIF) {
            t = EAT();
            
            ks_ast cond = SUB(EXPR);
            if (!cond) {
                KS_DECREF(res);
                return NULL;
            }

            body = SUB(BORC);
            if (!body) {
                KS_DECREF(cond);
                KS_DECREF(res);
                return NULL;
            }

            /* Construct branch of the deepest tree */
            ks_ast other = ks_ast_newn(KS_AST_IF, 2, (ks_ast[]){ cond, body }, NULL, t);
            ks_ast_pushn(deep, other);
            deep = other;

        }

        if (TOK.kind == KS_TOK_ELSE) {
            EAT();

            ks_ast other = SUB(BORS);
            if (!other) {
                KS_DECREF(res);
                return NULL;
            }

            ks_ast_pushn(deep, other);
        }

        return res;


    } else if (k == KS_TOK_TRY) {
        EAT();

        ks_ast body = SUB(BORS);
        if (!body) return NULL;


        ks_ast res = ks_ast_newn(KS_AST_TRY, 1, &body, NULL, t);

        /* Parse 'catch' clauses */
        while (TOK.kind == KS_TOK_CATCH) {
            ks_tok t = EAT();
        
            ks_ast tp = SUBF(EXPR, (flags & PF_NO_AS));
            if (!tp) {
                KS_DECREF(res);
                return NULL;
            }

            ks_ast assign = NULL;
            
            if (TOK.kind == KS_TOK_AS) {
                /* If given 'as' we should assign it to an expression */
                EAT();
                assign = SUB(EXPR);
            }  else {
                /* Otherwise, don't assign to anything */
                assign = (ks_ast)KS_NEWREF(ast_none);
            }

            if (!assign) {
                KS_DECREF(res);
                KS_DECREF(tp);
                return NULL;
            }

            /* Parse body */
            body = SUB(BORC);
            if (!body) {
                KS_DECREF(res);
                KS_DECREF(tp);
                KS_DECREF(assign);
                return NULL;
            }

            /* Now, append  the 'else' clause to the result */
            ks_ast_pushn(res, tp);
            ks_ast_pushn(res, assign);
            ks_ast_pushn(res, body);
        }

        if (TOK.kind == KS_TOK_FINALLY) {
            /* Push finally clause */
            ks_tok t = EAT();

            body = SUB(BORS);
            if (!body) {
                KS_DECREF(res);
                return NULL;
            }

            ks_ast_pushn(res, body);
        }

        return res;

    } else if (k == KS_TOK_SEMI) {
        /* Empty block */
        t = EAT();
        return ks_ast_newn(KS_AST_BLOCK, 0, NULL, NULL, t);
    } else {
        /* Try expression */
        ks_ast sub = SUB(EXPR);
        if (!sub) return NULL;

        if (!SUB(N)) {
            KS_DECREF(sub);
            return NULL;
        }

        return sub;
    }

    assert(false);
    return NULL;
}


RULE(B) {
    ks_tok t = EAT();
    if (t.kind != KS_TOK_LBRC) {
        KS_THROW_SYNTAX(fname, src, t, "Unexpected token, expected '{' to begin block");
        return NULL;
    }

    ks_ast res = ks_ast_new(KS_AST_BLOCK, 0, NULL, NULL, t);

    SKIP_N();
    while (!DONE && TOK.kind != KS_TOK_RBRC) {
        ks_ast sub = SUB(STMT);
        if (!sub) {
            KS_DECREF(res);
            return NULL;
        }

        ks_ast_pushn(res, sub);
        SKIP_N();
    }

    if (TOK.kind != KS_TOK_RBRC) {
        KS_THROW_SYNTAX(fname, src, t, "Unexpected token, expected '}' to end block");
        return NULL;
    }
    res->tok = ks_tok_combo(res->tok, EAT());

    return res;
}

RULE(BORC) {
    if (TOK.kind == KS_TOK_LBRC) {
        return SUB(B);
    } else if (TOK.kind == KS_TOK_COM) {
        EAT();
        return SUB(STMT);
    } else {
        KS_THROW_SYNTAX(fname, src, TOK, "Unexpected token, expected '{...}' block or ',' and a statement");
        return NULL;
    }
}

RULE(BORS) {
    if (TOK.kind == KS_TOK_LBRC) {
        return SUB(B);
    } else {
        return SUB(STMT);
    }
}

RULE(EXPR) {
    return SUB(E0);
}

RULE(E0) {
    ks_ast res = SUB(E1);
    if (!res) return NULL;

    #define E0_CASE(_tokk, _astk) else if (TOK.kind == _tokk) { \
        ks_tok t = EAT(); \
        SKIP_N(); \
        ks_ast lhs = res, rhs = SUB(E0) /* Right associative, so call again on the right hand side */; \
        if (!rhs) { \
            KS_DECREF(res); \
            return NULL; \
        } \
        res = ks_ast_newn(_astk, 2, (ks_ast[]){ lhs, rhs }, NULL, t); \
    }

    if (false) {}

    E0_CASE(KS_TOK_ASSIGN, KS_AST_BOP_ASSIGN)
    E0_CASE(KS_TOK_AIOR, KS_AST_BOP_AIOR)
    E0_CASE(KS_TOK_AXOR, KS_AST_BOP_AXOR)
    E0_CASE(KS_TOK_AAND, KS_AST_BOP_AAND)
    E0_CASE(KS_TOK_ALSH, KS_AST_BOP_ALSH)
    E0_CASE(KS_TOK_ARSH, KS_AST_BOP_ARSH)
    E0_CASE(KS_TOK_AADD, KS_AST_BOP_AADD)
    E0_CASE(KS_TOK_ASUB, KS_AST_BOP_ASUB)
    E0_CASE(KS_TOK_AMUL, KS_AST_BOP_AMUL)
    E0_CASE(KS_TOK_ADIV, KS_AST_BOP_ADIV)
    E0_CASE(KS_TOK_AMOD, KS_AST_BOP_AMOD)
    E0_CASE(KS_TOK_APOW, KS_AST_BOP_APOW)
    
    return res;
    #undef E0_CASE
}



RULE(E1) {
    ks_ast res = SUB(E1p);
    if (!res) return NULL;

    if (TOK.kind == KS_TOK_IF) {
        /* Conditional expression */
        EAT();
        SKIP_N();
        ks_ast cond = SUB(E1p);
        if (!cond) {
            KS_DECREF(res);
            return NULL;
        }

        if (TOK.kind == KS_TOK_ELSE) {
            EAT();
            SKIP_N();

            /* Handle 'else' branch */
            ks_ast other = SUB(E1);
            if (!other) {
                KS_DECREF(res);
                KS_DECREF(cond);
                return NULL;
            }

            return ks_ast_newn(KS_AST_COND, 3, (ks_ast[]){ cond, res, other }, NULL, KS_TOK_MAKE_EMPTY());
        } else {
            return ks_ast_newn(KS_AST_COND, 3, (ks_ast[]){ cond, res, ast_none }, NULL, KS_TOK_MAKE_EMPTY());
        }
    }
    return res;
}


/* 'E2 (as E1p)* */
RULE(E1p) {
    ks_ast res = SUB(E2);
    if (!res) return NULL;

    while (TOK.kind == KS_TOK_AS && !(flags & PF_NO_AS)) {
        /* Conditional expression */
        EAT();
        SKIP_N();
        ks_ast other = SUB(E2);
        if (!other) {
            KS_DECREF(res);
            return NULL;
        }
        ks_tok t = ks_tok_combo( other->tok, res->tok);
        res = ks_ast_newn(KS_AST_CALL, 2, (ks_ast[]){ other, res }, NULL, t);
        res->tok = t;
    }
    return res;
}


/* Macro for a left-associative rule. Varargs are the 'if/else' body selecting the type based on token */
#define RULE_BOP_LA(_rule, _next, ...) RULE(_rule) { \
    ks_ast res = SUB(_next); \
    if (!res) return NULL; \
    while (true) { \
        int k = 0; \
        __VA_ARGS__ \
        else break; /* not valid match */ \
        ks_tok t = EAT(); \
        SKIP_N(); \
        ks_ast lhs = res, rhs = SUB(_next); \
        if (!rhs) { \
            KS_DECREF(res); \
            return NULL; \
        } \
        res = ks_ast_newn(k, 2, (ks_ast[]){ lhs, rhs }, NULL, t); \
    } \
    return res; \
}

RULE_BOP_LA(E2, E3,
/**/ if (TOK.kind == KS_TOK_QUESQUES) k = KS_AST_BOP_QUESQUES;
)
RULE_BOP_LA(E3, E4,
/**/ if (TOK.kind == KS_TOK_OROR) k = KS_AST_BOP_OROR;
)
RULE_BOP_LA(E4, E5,
/**/ if (TOK.kind == KS_TOK_ANDAND) k = KS_AST_BOP_ANDAND;
)


/* Comparisons, which are special because they aren't associative
 * The result is sometimes a rich comparison, instead of a binary node
 */
RULE(E5) {
    ks_ast sub = SUB(E6);
    if (!sub) return NULL;

    ks_list res = ks_list_new(0, NULL);
    ks_list_push(res, (kso)sub);
    KS_DECREF(sub);

    /* special case, we must build a rich comparison */
    ks_list cmps = ks_list_new(0, NULL);

    while (true) {
        int k = 0;
        /**/ if (TOK.kind == KS_TOK_LT) k = KS_AST_BOP_LT;
        else if (TOK.kind == KS_TOK_LE) k = KS_AST_BOP_LE;
        else if (TOK.kind == KS_TOK_GT) k = KS_AST_BOP_GT;
        else if (TOK.kind == KS_TOK_GE) k = KS_AST_BOP_GE;
        else if (TOK.kind == KS_TOK_EQ) k = KS_AST_BOP_EQ;
        else if (TOK.kind == KS_TOK_NE) k = KS_AST_BOP_NE;
        else if (TOK.kind == KS_TOK_IN && !(flags & PF_NO_IN)) k = KS_AST_BOP_IN;
        else break; /* not valid */

        /* Skip token */
        ks_tok t = EAT();
        SKIP_N();
        ks_ast rhs = SUB(E7);
        if (!rhs) {
            KS_DECREF(res);
            KS_DECREF(cmps);
            return NULL;
        }

        /* Add to results */
        ks_list_pushu(res, (kso)rhs);
        ks_list_pushu(cmps, (kso)ks_int_new(k));
    }

    if (res->len == 1) {
        /* no conditional operators */
        ks_ast rr = (ks_ast)res->elems[0];
        KS_INCREF(rr);
        KS_DECREF(res);
        KS_DECREF(cmps);
        return rr;
    } else if (res->len == 2) {
        /* one conditional operator, so no need for rich comparison */
        ks_cint k;
        if (!kso_get_ci(cmps->elems[0], &k)) {
            KS_DECREF(cmps);
            KS_DECREF(res);
            return NULL;
        }
        ks_ast rr = ks_ast_new(k, res->len, (ks_ast*)res->elems, NULL, ((ks_ast)res->elems[0])->tok);
        KS_DECREF(res);
        KS_DECREF(cmps);
        return rr;
    } else {
        /* rich comparison needed */
        ks_tuple rcs = ks_tuple_new(cmps->len, cmps->elems);
        ks_ast rr = ks_ast_new(KS_AST_RICHCMP, res->len, (ks_ast*)res->elems, (kso)rcs, KS_TOK_MAKE_EMPTY());
        KS_DECREF(rcs);
        KS_DECREF(res);
        KS_DECREF(cmps);
        return rr;
    }
}

RULE_BOP_LA(E6, E7,
/**/ if (TOK.kind == KS_TOK_IOR) k = KS_AST_BOP_IOR;
)
RULE_BOP_LA(E7, E8,
/**/ if (TOK.kind == KS_TOK_XOR) k = KS_AST_BOP_XOR;
)
RULE_BOP_LA(E8, E9,
/**/ if (TOK.kind == KS_TOK_AND) k = KS_AST_BOP_AND;
)

RULE_BOP_LA(E9, E10,
/**/ if (TOK.kind == KS_TOK_LSH) k = KS_AST_BOP_LSH;
else if (TOK.kind == KS_TOK_RSH) k = KS_AST_BOP_RSH;
)
RULE_BOP_LA(E10, E11,
/**/ if (TOK.kind == KS_TOK_ADD) k = KS_AST_BOP_ADD;
else if (TOK.kind == KS_TOK_SUB) k = KS_AST_BOP_SUB;
)
RULE_BOP_LA(E11, E12,
/**/ if (TOK.kind == KS_TOK_MUL) k = KS_AST_BOP_MUL;
else if (TOK.kind == KS_TOK_AT) k = KS_AST_BOP_MATMUL;
else if (TOK.kind == KS_TOK_DIV) k = KS_AST_BOP_DIV;
else if (TOK.kind == KS_TOK_FLOORDIV) k = KS_AST_BOP_FLOORDIV;
else if (TOK.kind == KS_TOK_MOD) k = KS_AST_BOP_MOD;
)

RULE(E12) {
    ks_ast res = SUB(E13);
    if (!res) return NULL;

    int k = 0;
    /**/ if (TOK.kind == KS_TOK_POW) k = KS_AST_BOP_POW;
    if (k != 0) {
        ks_tok t = EAT();
        SKIP_N();
        ks_ast rhs = SUB(E12);
        if (!rhs) {
            KS_DECREF(res);
            return NULL;
        }

        return ks_ast_newn(k, 2, (ks_ast[]){ res, rhs }, NULL, t);
    }

    return res;
}


RULE(E13) {
    int k = 0;
    /* Unary prefix operators go here */
    /**/ if (TOK.kind == KS_TOK_ADDADD) k = KS_AST_UOP_POSPOS;
    else if (TOK.kind == KS_TOK_SUBSUB) k = KS_AST_UOP_NEGNEG;
    else if (TOK.kind == KS_TOK_ADD) k = KS_AST_UOP_POS;
    else if (TOK.kind == KS_TOK_SUB) k = KS_AST_UOP_NEG;
    else if (TOK.kind == KS_TOK_SQIG) k = KS_AST_UOP_SQIG;
    else if (TOK.kind == KS_TOK_NOT) k = KS_AST_UOP_NOT;
    else if (TOK.kind == KS_TOK_QUES) k = KS_AST_UOP_QUES;

    if (k != 0) {
        /* Parse unary operator */
        ks_tok t = EAT();
        SKIP_N();
        ks_ast sub = SUB(E13);
        if (!sub) return NULL;

        return ks_ast_newn(k, 1, (ks_ast[]){ sub }, NULL, t);
    } else {
        /* No prefix operator, so decay */
        return SUB(E14);
    }
}

RULE(E14) {
    /* This one's a bit tricky, we basically start with the left hand side, 
     *   and continually build on it while there is token representing a function call, index operation, or unary postfix operator.
     */
    ks_ast res = NULL;
    ks_tok t, tt;

    /* First, we find the base value. This is either a group '()', function, type, or ATOM */
    if (TOK.kind == KS_TOK_LPAR) {
        /* Either a group, or a tuple. Try some special cases, or try as a tuple and then check afterwards whether there
         *   was a comma given
         */
        t = EAT();
        SKIP_N();

        if (TOK.kind == KS_TOK_COM) {
            EAT();
            SKIP_N();

            if (TOK.kind == KS_TOK_RPAR) {
                res = ks_ast_new(KS_AST_TUPLE, 0, NULL, NULL, ks_tok_combo(t, EAT()));
            } else {
                KS_THROW_SYNTAX(fname, src, TOK, "Expected ')' immediately after ',' in the empty tuple constructor '(,)'");
                return NULL;
            }
        } else if (TOK.kind == KS_TOK_RPAR) {
            res = ks_ast_new(KS_AST_TUPLE, 0, NULL, NULL, ks_tok_combo(t, EAT()));
        } else {
            /* May be expression or tuple */
            ks_ast tup = ks_ast_new(KS_AST_TUPLE, 0, NULL, NULL, t);
            bool had_comma = false;

            while (TOK.kind != KS_TOK_RPAR) {
                tt = TOK;
                bool is_va = TOK.kind == KS_TOK_MUL;
                if (is_va) {
                    EAT();
                    had_comma = true;
                }

                ks_ast sub = SUB(EXPR);
                if (!sub) {
                    KS_DECREF(tup);
                    return NULL;
                }

                if (is_va) sub = ks_ast_newn(KS_AST_UOP_STAR, 1, &sub, NULL, tt);
                ks_ast_pushn(tup, sub);

                SKIP_N();
                if (TOK.kind == KS_TOK_COM) {
                    EAT();                
                    SKIP_N();
                    had_comma = true;
                } else break;
            }

            if (TOK.kind != KS_TOK_RPAR) {
                KS_THROW_SYNTAX(fname, src, TOK, "Expected ')' to end group here");
                return NULL;
            }
            tup->tok = ks_tok_combo(tup->tok, EAT());

            if (tup->args->len == 1 && !had_comma) {
                res = (ks_ast)tup->args->elems[0];
                KS_INCREF(res);
                KS_DECREF(tup);
            } else {
                res = tup;
            }
        }
        res->tok = ks_tok_combo(t, res->tok);

    } else if (TOK.kind == KS_TOK_LBRK) {
        /* List constructor */
        res = ks_ast_new(KS_AST_LIST, 0, NULL, NULL, EAT());

        SKIP_N();
        while (TOK.kind != KS_TOK_RBRK) {
            tt = TOK;
            bool is_va = tt.kind == KS_TOK_MUL;
            if (is_va) EAT();

            ks_ast sub = SUB(EXPR);
            if (!sub) {
                KS_DECREF(res);
                return NULL;
            }

            if (is_va) sub = ks_ast_newn(KS_AST_UOP_STAR, 1, (ks_ast[]){ sub }, NULL, tt);
            ks_ast_pushn(res, sub);

            SKIP_N();
            if (TOK.kind == KS_TOK_COM) {
                EAT();
                SKIP_N();
            } else break;
        }

        if (TOK.kind != KS_TOK_RBRK) {
            int ri = toki - 1;
            while (ri > 0 && toks[ri].kind == KS_TOK_N) ri--;
            KS_THROW_SYNTAX(fname, src, toks[ri], "Expected ']' to end list constructor here");
            KS_DECREF(res);
            return NULL;
        }

        res->tok = ks_tok_combo(res->tok, EAT());

    } else if (TOK.kind == KS_TOK_LBRC) {
        /* Set or dictionary constructor */
        t = EAT();
        SKIP_N();

        if (TOK.kind == KS_TOK_COM) {
            EAT();
            SKIP_N();

            if (TOK.kind == KS_TOK_RBRC) {
                res = ks_ast_new(KS_AST_DICT, 0, NULL, NULL, ks_tok_combo(t, EAT()));
            } else {
                KS_THROW_SYNTAX(fname, src, TOK, "Expected '}' immediately after ',' in the empty dict constructor '{,}'");
                return NULL;
            }
        } else if (TOK.kind == KS_TOK_RBRC) {
            ks_tok tttt = ks_tok_combo(t, EAT());
            res = ks_ast_new(KS_AST_DICT, 0, NULL, NULL, tttt);
        } else {
            /* May be set or dict */
            assert(res == NULL);
            bool is_dict = true;

            while (TOK.kind != KS_TOK_RBRC) {
                tt = TOK;
                bool is_va = tt.kind == KS_TOK_MUL;
                if (is_va) EAT();

                ks_ast sub = SUB(EXPR);
                if (!sub) {
                    if (res) KS_DECREF(res);
                    return NULL;
                }

                if (!res) {
                    /* First time */
                    is_dict = TOK.kind == KS_TOK_COL;
                    res = ks_ast_newn(is_dict ? KS_AST_DICT : KS_AST_SET, 0, NULL, NULL, t);
                } else {
                    if (is_dict && TOK.kind != KS_TOK_COL) {
                        KS_THROW_SYNTAX(fname, src, TOK, "Expected ':' for keyval entry in dict constructor");
                        KS_DECREF(sub);
                        KS_DECREF(res);
                        return NULL;
                    } else if (!is_dict && TOK.kind == KS_TOK_COL) {
                        KS_THROW_SYNTAX(fname, src, TOK, "Unxpected ':' for entry in set constructor (did you mean to make a dict?)");
                        KS_DECREF(sub);
                        KS_DECREF(res);
                        return NULL;
                    }
                }
                if (is_va) sub = ks_ast_newn(KS_AST_UOP_STAR, 1, &sub, NULL, tt);

                if (is_dict) {
                    /* Error should have been handled above */
                    ks_tok ttt = EAT();
                    assert(ttt.kind == KS_TOK_COL);
                    ks_ast val = SUB(EXPR);
                    if (!val) {
                        KS_DECREF(res);
                        KS_DECREF(sub);
                        return NULL;
                    }
                    ks_ast_pushn(res, sub);
                    ks_ast_pushn(res, val);

                } else {
                    ks_ast_pushn(res, sub);
                }

                SKIP_N();
                if (TOK.kind == KS_TOK_COM) {
                    EAT();                
                    SKIP_N();
                } else break;
            }

            if (TOK.kind != KS_TOK_RBRC) {
                KS_THROW_SYNTAX(fname, src, TOK, "Expected '}' to end dict here");
                KS_DECREF(res);
                return NULL;
            }
            res->tok = ks_tok_combo(res->tok, EAT());
        }

    } else if (TOK_EQ(TOK, "func") && (toks[toki+1].kind == KS_TOK_LBRC || toks[toki+1].kind == KS_TOK_LPAR || toks[toki+1].kind == KS_TOK_NAME)) {
        res = ks_ast_new(KS_AST_FUNC, 0, NULL, NULL, EAT());
        ks_str fname = NULL;
        if (TOK.kind == KS_TOK_NAME) {
            fname = ks_tok_str(src, EAT());
        } else {
            fname = ks_str_new(-1, "<anon-func>");
        }
        ksio_StringIO sio = ksio_StringIO_new();
        ks_list lp = ks_list_new(0, NULL);
        ksio_add(sio, "%S(", fname);
        if (TOK.kind == KS_TOK_LPAR) {
            /* Parse parameters */
            ks_ast pars = ks_ast_new(KS_AST_TUPLE, 0, NULL, NULL, EAT());
            int ct = 0;

            while (TOK.kind != KS_TOK_RPAR) {
                tt = TOK;
                bool is_va = tt.kind == KS_TOK_MUL;
                if (is_va) EAT();

                ks_ast sub = SUB(EXPR);
                if (!sub) {
                    KS_DECREF(res);
                    KS_DECREF(pars);
                    KS_DECREF(fname);
                    KS_DECREF(sio);
                    KS_DECREF(lp);
                    return NULL;
                }

                if (sub->kind == KS_AST_NAME) {
                    ks_list_push(lp, sub->val);
                } else if (sub->kind == KS_AST_BOP_ASSIGN) {
                    ks_ast lhs = (ks_ast)sub->args->elems[0];
                    if (lhs->kind == KS_AST_NAME) {
                        ks_list_push(lp, lhs->val);
                    } else {
                        KS_THROW_SYNTAX(fname, src, lhs->tok, "Invalid parameter, expected a name");
                        KS_DECREF(res);
                        KS_DECREF(pars);
                        KS_DECREF(fname);
                        KS_DECREF(sio);
                        KS_DECREF(lp);
                        return NULL;
                    }
                } else {
                    KS_THROW_SYNTAX(fname, src, sub->tok, "Invalid parameter, expected a name");
                    KS_DECREF(res);
                    KS_DECREF(pars);
                    KS_DECREF(fname);
                    KS_DECREF(sio);
                    KS_DECREF(lp);
                    return NULL;
                }

                if (is_va) sub = ks_ast_newn(KS_AST_UOP_STAR, 1, (ks_ast[]){ sub }, NULL, tt);
                ks_ast_pushn(pars, sub);

                if (ct++ > 0) {
                    ksio_add(sio, ", ");
                }
                ks_str tts = ks_tok_str(src, sub->tok);
                ksio_add(sio, "%S", tts);
                KS_DECREF(tts);

                SKIP_N();
                if (TOK.kind == KS_TOK_COM) {
                    EAT();
                    SKIP_N();
                } else break;
            }

            if (TOK.kind != KS_TOK_RPAR) {
                KS_DECREF(res);
                KS_DECREF(pars);
                KS_DECREF(fname);
                KS_DECREF(lp);
                KS_DECREF(sio);
                return NULL;
            }

            pars->tok = ks_tok_combo(pars->tok, EAT());
            ks_ast_pushn(res, pars);

        } else {
            /* No parameters */
            ks_ast_pushn(res, ks_ast_new(KS_AST_TUPLE, 0, NULL, NULL, res->tok));
        }
        ksio_add(sio, ")", fname);
        ks_str sig = ksio_StringIO_getf(sio);

        ks_tuple lpt = ks_tuple_newi((kso)lp);
        KS_DECREF(lp);
        assert(lpt != NULL);

        ks_ast body = SUB(B);
        if (!body) {
            KS_DECREF(res);
            KS_DECREF(fname);
            KS_DECREF(sig);
            return NULL;
        }

        ks_ast_pushn(res, body);

        /* TODO: detect doc */
        ks_str doc = ks_str_new(-1, "");

        res->val = (kso)ks_tuple_newn(4, (kso[]){
            (kso)fname,
            (kso)sig,
            (kso)lpt,
            (kso)doc
        });


    } else if (TOK_EQ(TOK, "type") && (toks[toki+1].kind == KS_TOK_LBRC || toks[toki+1].kind == KS_TOK_NAME)) {
        res = ks_ast_new(KS_AST_TYPE, 0, NULL, NULL, EAT());
        ks_str tname = NULL;
        if (TOK.kind == KS_TOK_NAME) {
            tname = ks_tok_str(src, EAT());
        } else {
            tname = ks_str_new(-1, "<anon-type>");
        }

        /* Parse the base type */
        if (TOK_EQ(TOK, "extends")) {
            EAT();
            ks_ast ext = SUB(EXPR);
            if (!ext) {
                KS_DECREF(res);
                return NULL;
            }

            ks_ast_pushn(res, ext);
            
        } else {
            ks_ast ext = ks_ast_newn(KS_AST_NAME, 0, NULL, (kso)ks_str_new(-1, "object"), res->tok);
            ks_ast_pushn(res, ext);
        }

        ks_ast body = SUB(B);
        if (!body) {
            KS_DECREF(res);
            return NULL;
        }

        ks_ast_pushn(res, body);

        /* TODO: detect doc */
        ks_str doc = ks_str_new(-1, "");

        res->val = (kso)ks_tuple_newn(2, (kso[]){
            (kso)tname,
            (kso)doc
        });


    } else {
        res = SUB(ATOM);
        if (!res) return NULL;
    }


    /* Should have been handled by now */
    assert(res != NULL);

    if (res->kind == KS_AST_NAME || res->kind == KS_AST_TUPLE) {
        if (TOK.kind == KS_TOK_RARW) {
            /* Lambda */
            tt = EAT();

            ks_ast expr = SUB(EXPR);
            if (!expr) {
                KS_DECREF(res);
                return NULL;
            }
            expr = ks_ast_newn(KS_AST_RET, 1, &expr, NULL, expr->tok);
            if (res->kind == KS_AST_NAME) {
                res = ks_ast_newn(KS_AST_TUPLE, 1, &res, NULL, res->tok);
            }
            
            ksio_StringIO sio = ksio_StringIO_new();
            ksio_add(sio, "<lambda>(");

            ks_list lp = ks_list_new(0, NULL);

            int i;
            for (i = 0; i < res->args->len; ++i) {
                if (i > 0) ksio_add(sio, ", ");
                ks_ast s = (ks_ast)res->args->elems[i];

                if (s->kind == KS_AST_NAME) {
                    ks_list_push(lp, s->val);
                } else {
                    ks_list_push(lp, ((ks_ast)s->args->elems[0])->val);
                }
                ks_str ss = ks_tok_str(src, s->tok);
                ksio_add(sio, "%S", ss);
                KS_DECREF(ss);
            }
            ksio_add(sio, ")");

            ks_tuple vt = ks_tuple_newn(4, (kso[]){
                (kso)ks_str_new(-1, "<lambda>"),
                (kso)ksio_StringIO_getf(sio),
                (kso)ks_tuple_new(lp->len, lp->elems),
                (kso)ks_str_new(-1, "")
            });

            KS_DECREF(lp);

            res = ks_ast_newn(KS_AST_FUNC, 2, (ks_ast[]){ res, expr }, (kso)vt, tt);
        }
    }

    while (!DONE) {
        if (TOK.kind == KS_TOK_DOT) {
            /* Attribute reference */
            t = EAT();
            if (TOK.kind == KS_TOK_NAME) {
                t = EAT();
                res = ks_ast_newn(KS_AST_ATTR, 1, (ks_ast[]){ res }, (kso)ks_tok_str(src, t), t);
            } else {
                KS_THROW_SYNTAX(fname, src, TOK, "Expected a valid name after '.' for attribute reference");
                KS_DECREF(res);
                return NULL;
            }
        } else if (TOK.kind == KS_TOK_LPAR) {
            /* Function call */
            res = ks_ast_newn(KS_AST_CALL, 1, (ks_ast[]){ res }, NULL, EAT());

            SKIP_N();
            while (TOK.kind != KS_TOK_RPAR) {
                tt = TOK;
                bool is_va = tt.kind == KS_TOK_MUL;
                if (is_va) EAT();

                ks_ast sub = SUB(EXPR);
                if (!sub) {
                    KS_DECREF(res);
                    return NULL;
                }

                if (is_va) sub = ks_ast_newn(KS_AST_UOP_STAR, 1, (ks_ast[]){ sub }, NULL, tt);
                ks_ast_pushn(res, sub);

                SKIP_N();
                if (TOK.kind == KS_TOK_COM) {
                    EAT();
                    SKIP_N();
                } else break;
            }

            if (TOK.kind != KS_TOK_RPAR) {
                int ri = toki - 1;
                while (ri > 0 && toks[ri].kind == KS_TOK_N) ri--;
                KS_THROW_SYNTAX(fname, src, toks[ri], "Expected ')' to end function call after this");
                KS_DECREF(res);
                return NULL;
            }

            res->tok = ks_tok_combo(res->tok, EAT());

        } else if (TOK.kind == KS_TOK_LBRK) {
            /* Function call */
            res = ks_ast_newn(KS_AST_ELEM, 1, (ks_ast[]){ res }, NULL, EAT());

            SKIP_N();
            while (TOK.kind != KS_TOK_RBRK) {
                tt = TOK;
                bool is_va = tt.kind == KS_TOK_MUL;
                if (is_va) EAT();

                /* Try and parse slice or expression */
                ks_ast sub[3] = { NULL, NULL, NULL };
                bool is_sl = false;
                ks_tok ttt = TOK;
                if (TOK.kind == KS_TOK_COL) {
                    EAT();
                    is_sl = true;
                    KS_INCREF(ast_none);
                    sub[0] = ast_none;
                    if (TOK.kind == KS_TOK_RPAR || TOK.kind == KS_TOK_RBRC || TOK.kind == KS_TOK_RBRK || TOK.kind == KS_TOK_COM) {
                        KS_INCREF(ast_none);
                        sub[1] = ast_none;
                        KS_INCREF(ast_none);
                        sub[2] = ast_none;
                    } else if (TOK.kind == KS_TOK_COL) {
                        EAT();
                        KS_INCREF(ast_none);
                        sub[1] = ast_none;

                        if (TOK.kind == KS_TOK_RPAR || TOK.kind == KS_TOK_RBRC || TOK.kind == KS_TOK_RBRK || TOK.kind == KS_TOK_COM) {
                            /* end: '::' */
                            KS_INCREF(ast_none);
                            sub[2] = ast_none;
                        } else {
                            /* end: '::e' */
                            sub[2] = SUB(EXPR);
                            if (!sub[2]) {
                                KS_DECREF(res);
                                KS_DECREF(sub[0]);
                                KS_DECREF(sub[1]);
                                return NULL;
                            }
                        }
                    } else {
                        sub[1] = SUB(EXPR);
                        if (!sub[1]) {
                            KS_DECREF(res);
                            KS_DECREF(sub[0]);
                            return NULL;
                        }
                        if (TOK.kind == KS_TOK_COL) {
                            EAT();
                            if (TOK.kind == KS_TOK_RPAR || TOK.kind == KS_TOK_RBRC || TOK.kind == KS_TOK_RBRK || TOK.kind == KS_TOK_COM) {
                                /* end: ':e:' */
                                KS_INCREF(ast_none);
                                sub[2] = ast_none;
                            } else {
                                /* end: ':e:e' */
                                sub[2] = SUB(EXPR);
                                if (!sub[2]) {
                                    KS_DECREF(res);
                                    KS_DECREF(sub[0]);
                                    KS_DECREF(sub[1]);
                                    return NULL;
                                }
                            }
                        } else {
                            /* end: '::' */
                            KS_INCREF(ast_none);
                            sub[2] = ast_none;
                        }
                    }
                } else {
                    sub[0] = SUB(EXPR);
                    if (!sub[0]) {
                        KS_DECREF(res);
                        return NULL;
                    }

                    if (TOK.kind == KS_TOK_COL) {
                        EAT();
                        is_sl = true;

                        if (TOK.kind == KS_TOK_COL) {
                            EAT();
                            KS_INCREF(ast_none);
                            sub[1] = ast_none;


                            if (TOK.kind == KS_TOK_RPAR || TOK.kind == KS_TOK_RBRC || TOK.kind == KS_TOK_RBRK || TOK.kind == KS_TOK_COM) {
                                /* end: 'e::' */
                                KS_INCREF(ast_none);
                                sub[2] = ast_none;

                            } else {
                                /* end: 'e::e' */
                                sub[2] = SUB(EXPR);
                                if (!sub[2]) {
                                    KS_DECREF(res);
                                    KS_DECREF(sub[0]);
                                    KS_DECREF(sub[1]);
                                    return NULL;
                                }
                            }
                        } else {
                            if (TOK.kind == KS_TOK_RPAR || TOK.kind == KS_TOK_RBRC || TOK.kind == KS_TOK_RBRK || TOK.kind == KS_TOK_COM) {
                                /* end: 'e:' */
                                KS_INCREF(ast_none);
                                sub[1] = ast_none;
                                KS_INCREF(ast_none);
                                sub[2] = ast_none;

                            } else {
                                sub[1] = SUB(EXPR);
                                if (!sub[1]) {
                                    KS_DECREF(res);
                                    KS_DECREF(sub[0]);
                                    return NULL;
                                }

                                if (TOK.kind == KS_TOK_COL) {
                                    EAT();

                                    if (TOK.kind == KS_TOK_RPAR || TOK.kind == KS_TOK_RBRC || TOK.kind == KS_TOK_RBRK || TOK.kind == KS_TOK_COM) {
                                        /* end: 'e:e:' */
                                        KS_INCREF(ast_none);
                                        sub[2] = ast_none;

                                    } else {
                                        /* end: 'e:e:e' */
                                        sub[2] = SUB(EXPR);
                                        if (!sub[2]) {
                                            KS_DECREF(res);
                                            KS_DECREF(sub[0]);
                                            KS_DECREF(sub[1]);
                                            return NULL;
                                        }
                                    }
                                } else {
                                    /* end: 'e:e:' */
                                    KS_INCREF(ast_none);
                                    sub[2] = ast_none;
                                }
                            }
                        }
                    } else {
                        /* Not a slice */
                    }
                }

                ks_ast rs = NULL;
                if (is_sl) {
                    rs = ks_ast_newn(KS_AST_SLICE, 3, sub, NULL, ttt);
                } else {
                    rs = sub[0];
                }

                if (is_va) rs = ks_ast_newn(KS_AST_UOP_STAR, 1, (ks_ast[]){ rs }, NULL, tt);
                ks_ast_pushn(res, rs);

                SKIP_N();
                if (TOK.kind == KS_TOK_COM) {
                    EAT();
                    SKIP_N();
                } else break;
            }

            if (TOK.kind != KS_TOK_RBRK) {
                int ri = toki - 1;
                while (ri > 0 && toks[ri].kind == KS_TOK_N) ri--;
                KS_THROW_SYNTAX(fname, src, toks[ri], "Expected ']' to end element index after this");
                KS_DECREF(res);
                return NULL;
            }

            res->tok = ks_tok_combo(res->tok, EAT());

        } else if (TOK.kind == KS_TOK_ADDADD) {
            /* ++, so do post-increment */

            res = ks_ast_newn(KS_AST_UOP_POSPOS_POST, 1, &res, NULL, ks_tok_combo(res->tok, EAT()));
        } else if (TOK.kind == KS_TOK_ADDADD) {
            /* --, so do post-decrement */

            res = ks_ast_newn(KS_AST_UOP_NEGNEG_POST, 1, &res, NULL, ks_tok_combo(res->tok, EAT()));

        } else break;
    }

    return res;
}


RULE(ATOM) {
    /* Check for single token expressions */
    if (TOK.kind == KS_TOK_NAME) {
        ks_tok t = EAT();
        ks_str v = ks_tok_str(src, t);
        kso r = ks_dict_get_ih(kwconst, (kso)v, v->v_hash);
        ks_ast res = NULL;
        if (r) {
            KS_DECREF(v);
            res = ks_ast_newn(KS_AST_CONST, 0, NULL, r, t);
        } else {
            res = ks_ast_newn(KS_AST_NAME, 0, NULL, (kso)v, t);
        }
        return res;
    } else if (TOK.kind == KS_TOK_DOTDOTDOT) {

        return ks_ast_newn(KS_AST_CONST, 0, NULL, KSO_DOTDOTDOT, EAT());

    } else if (TOK.kind == KS_TOK_INT) {
        ks_tok t = EAT();
        ks_int v = ks_int_news(t.epos - t.spos, src->data + t.spos, 0);
        if (!v) return NULL;
        return ks_ast_newn(KS_AST_CONST, 0, NULL, (kso)v, t);
    } else if (TOK.kind == KS_TOK_FLOAT) {
        ks_tok t = EAT();
        int ep = t.epos;
        if (src->data[ep - 1] == 'i' || src->data[ep - 1] == 'I') {
            ep--;
        }
        ks_cfloat v;
        if (!ks_cfloat_from_str(src->data + t.spos, ep - t.spos, &v)) return NULL;
        return ks_ast_newn(KS_AST_CONST, 0, NULL, ep == t.epos ? (kso)ks_float_new(v) : (kso)ks_complex_newre(0, v), t);
    } else if (TOK.kind == KS_TOK_REGEX) {
        /* Regex literal */
        ks_tok t = EAT();

        /* Whether its a triple quoted literal */
        bool is3 = (t.epos - t.spos) > 3  && strncmp(src->data + t.spos, "```", 3) == 0;
        ks_str s = ks_str_new(t.epos - t.spos - (is3?6:2), src->data + t.spos + (is3?3:1));

        ks_regex v = ks_regex_new(s);
        if (!v) {
            KS_DECREF(s);
            return NULL;
        }
        KS_DECREF(s);

        return ks_ast_newn(KS_AST_CONST, 0, NULL, (kso)v, t);


    } else if (TOK.kind == KS_TOK_STR) {
        ks_tok t = EAT();
        /* Parse string literal */
        ksio_StringIO sio = ksio_StringIO_new();
        ksio_BaseIO aio = (ksio_BaseIO)sio;

        char c = src->data[t.spos];
        assert(c == '\'' || c == '"');

        /* Whether its a triple quoted literal */
        bool is3 = (t.epos - t.spos) > 3  && strncmp(src->data + t.spos, c == '\'' ? "'''" : "\"\"\"", 3) == 0;

        /* Get section where the contents are */
        int sz = t.epos - t.spos - (is3 ? 6 : 2);
        char* s = src->data + t.spos + (is3 ? 3 : 1);
        int i = 0;

        unsigned char utf8[5];

        while (i < sz) {
            int l = i;
            while (i < sz && s[i] != '\\') {
                i++;
            }
            /* Add literal string */
            ksio_addbuf(aio, i - l, s + l);

            if (i >= sz) break;

            /* Should be an escape code */
            assert(s[i] == '\\');
            i++;
            c = s[i];
            i++;

            /**/ if (c == '\\') ksio_addbuf(aio, 1, "\\");
            else if (c == '\'') ksio_addbuf(aio, 1, "'");
            else if (c == '"') ksio_addbuf(aio, 1, "\"");
            else if (c == 'a') ksio_addbuf(aio, 1, "\a");
            else if (c == 'b') ksio_addbuf(aio, 1, "\b");
            else if (c == 'f') ksio_addbuf(aio, 1, "\f");
            else if (c == 'n') ksio_addbuf(aio, 1, "\n");
            else if (c == 'r') ksio_addbuf(aio, 1, "\r");
            else if (c == 't') ksio_addbuf(aio, 1, "\t");
            else if (c == 'v') ksio_addbuf(aio, 1, "\v");
            else if (c == 'x') {
                /* \xHH, single byte */
                int ct = 0;
                ks_ucp v = 0;
                while (ct < 2) {
                    c = s[i + ct];
                    int d;

                    /**/ if ('0' <= c && c <= '9') d = c - '0';
                    else if ('a' <= c && c <= 'f') d = c - 'a' + 10;
                    else if ('A' <= c && c <= 'F') d = c - 'A' + 10;
                    else {
                        KS_DECREF(sio);
                        KS_THROW_SYNTAX(fname, src, t, "Truncated ecape sequence: '\\xHH'");
                        return NULL;
                    }

                    v = 16 * v + d;
                    ++ct;
                }
                i += ct;

                /* Add byte */
                utf8[0] = v;
                ksio_addbuf(aio, 1, utf8);
            } else if (c == 'u') {
                /* \uHHHH, codepoint */
                int ct = 0;
                ks_ucp v = 0;
                while (ct < 4) {
                    c = s[i + ct];
                    int d;

                    /**/ if ('0' <= c && c <= '9') d = c - '0';
                    else if ('a' <= c && c <= 'f') d = c - 'a' + 10;
                    else if ('A' <= c && c <= 'F') d = c - 'A' + 10;
                    else {
                        KS_DECREF(sio);
                        KS_THROW_SYNTAX(fname, src, t, "Truncated ecape sequence: '\\uHHHH'");
                        return NULL;
                    }

                    v = 16 * v + d;
                    ++ct;
                }
                i += ct;
                /* Decode and add */
                int n = 0;
                KS_UCP_TO_UTF8(utf8, n, v);
                ksio_addbuf(aio, n, utf8);
            } else if (c == 'U') {
                /* \uHHHHHHHH, codepoint */
                int ct = 0;
                ks_ucp v = 0;
                while (ct < 8) {
                    c = s[i + ct];
                    int d;

                    /**/ if ('0' <= c && c <= '9') d = c - '0';
                    else if ('a' <= c && c <= 'f') d = c - 'a' + 10;
                    else if ('A' <= c && c <= 'F') d = c - 'A' + 10;
                    else {
                        KS_DECREF(sio);
                        KS_THROW_SYNTAX(fname, src, t, "Truncated ecape sequence: '\\UHHHHHHHH'");
                        return NULL;
                    }

                    v = 16 * v + d;
                    ++ct;
                }
                i += ct;

                /* Decode and add */
                int n;
                KS_UCP_TO_UTF8(utf8, n, v);
                ksio_addbuf(aio, n, utf8);
            } else if (c == 'N') {
                /* \N[NAME] */
                if (s[i] != '[') {
                    KS_DECREF(sio);
                    KS_THROW_SYNTAX(fname, src, t, "Escape sequence '\\N' expects '[]' enclosing the name", c);
                    return NULL;
                }
                i++;
                
                /* Read until the end */
                int sn = i;
                while (i < sz && s[i] != ']') {
                    i++;
                }
                int en = i;

                /* Check for end */
                if (s[i] != ']') {
                    KS_DECREF(sio);
                    KS_THROW_SYNTAX(fname, src, t, "Escape sequence '\\N' expects '[]' enclosing the name", c);
                    return NULL;
                }
                i++;

                struct ksucd_info info;
                ks_ucp v = ksucd_lookup(&info, en-sn, s+sn);
                if (v < 1) {
                    KS_DECREF(sio);
                    KS_THROW_SYNTAX(fname, src, t, "Unknown unicode character: '%.*s'", en-sn, s+sn);
                    return NULL;
                }
                /* Decode and add */
                int n;
                KS_UCP_TO_UTF8(utf8, n, v);
                ksio_addbuf(aio, n, utf8);
            } else {
                KS_THROW_SYNTAX(fname, src, t, "Unexpected escape sequence '\\%c'", c);
                KS_DECREF(sio);
                return NULL;
            }
        }

        return ks_ast_newn(KS_AST_CONST, 0, NULL, (kso)ksio_StringIO_getf(sio), t);
    } 

    KS_THROW_SYNTAX(fname, src, TOK, "Unexpected token");
    return NULL;
}



/* Exported API */
ks_ast ks_parse_prog(ks_str fname, ks_str src, ks_ssize_t n_toks, ks_tok* toks) {
    int i = 0;
    int *tokip = &i;

    ks_ast res = SUBF(PROG, PF_NONE);
    if (!res) return NULL;

    if (TOK.kind != KS_TOK_EOF) {
        KS_THROW_SYNTAX(fname, src, TOK, "Unexpected token");
        KS_DECREF(res);
        return NULL;
    }

    return res;
}

ks_ast ks_parse_expr(ks_str fname, ks_str src, ks_ssize_t n_toks, ks_tok* toks) {
    int i = 0;
    int *tokip = &i;

    ks_ast res = SUBF(EXPR, PF_NONE);

    return res;
}

void _ksi_parser() {
    ast_none = ks_ast_new(KS_AST_CONST, 0, NULL, KSO_NONE, KS_TOK_MAKE_EMPTY());
    kwconst = ks_dict_new(KS_IKV(
        {"none",         KS_NEWREF(KSO_NONE)},
        {"true",         KS_NEWREF(KSO_TRUE)},
        {"false",        KS_NEWREF(KSO_FALSE)},
        {"inf",         KS_NEWREF(KSO_INF)},
        {"nan",         KS_NEWREF(KSO_NAN)},

    ));
}

