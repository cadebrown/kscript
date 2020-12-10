/* compiler.c - implements the bytecode compiler for the kscript VM, which transforms ASTs into code objects
 *
 * The implementation is an AST visiter, which visits node in a depth-first traversal, where expressions
 *   yield values on the implicit stack (i.e. when being ran they will be the last items on the thread's program
 *   stack). So, it turns an AST into a postfix machine code which can be efficiently computed
 * 
 * Constant expressions just push their value on the stack, and their parents know that they will be the last on the stack.
 * 
 * When the bytecode for nested expressions executes, it should result in a net gain of a single object being left on the stack,
 *   no matter how complicated. Therefore, binary operators can simply compile their children and then assume the top two items
 *   on the stack are what is left over
 * 
 * @author: Cade Brown <cade@kscript.org>
 */
#include <ks/impl.h>
#include <ks/compiler.h>


/** Internals **/


/* Internal compiler state */
struct compiler {

    /* Length of the stack */
    int len_stk;

};

/* Compile an AST recursively, yielding the success boolean */
#define COMPILE(_node) compile(co, fname, src, code, (_node))

/* Compile a sub-node of the current node */
#define SUB(_idx) ((ks_ast)v->args->elems[(_idx)])

/* Number of sub-nodes of the current node */
#define NSUB ((int)v->args->len)

/* Emit an opcode */
#define EMIT(_op) ks_code_emit(code, (_op))

/* Emit an opcode and argument */
#define EMITI(_op, _ival) ks_code_emiti(code, (_op), (_ival))
#define EMITO(_op, _oval) ks_code_emito(code, (_op), (_oval))

/* Emit a token as meta at the current position */
#define META(_tok) ks_code_meta(code, (_tok))

/* Current number of bytecode */
#define BC_N (code->bc->len_b)

/* Length of the stack */
#define LEN (co->len_stk)

/* Clears the stack to the given length, by popping extra values */
#define CLEAR(_len) do { \
    int _vlen = (_len); \
    assert(_vlen <= LEN); \
    while (LEN > _vlen) { \
        EMIT(KSB_POPU); \
        LEN -= 1; \
    } \
} while (0)

/* Macro to patch a jump in bytecode, takes the location it was added, and the location directly
 *   after the instruction was added, and where it should jump to
 */
#define PATCH(_loc, _from, _to) do { \
    ((ksba*)(code->bc->data + _loc))->arg = _to - _from; \
} while (0)


static bool compile(struct compiler* co, ks_str fname, ks_str src, ks_code code, ks_ast v);


/* Computes assignment, from the TOS (which should alrea)
 */
static bool assign(struct compiler* co, ks_str fname, ks_str src, ks_code code, ks_ast lhs, ks_ast rhs, ks_ast par) {

    if (lhs->kind == KS_AST_NAME) {
        EMITO(KSB_STORE, lhs->val);

    } else if (lhs->kind == KS_AST_ATTR) {
        if (!COMPILE((ks_ast)lhs->args->elems[0])) return false;
        EMITO(KSB_SETATTR, lhs->val);
        LEN--;
        META(par->tok);

    } else {
        KS_THROW_SYNTAX(fname, src, par->tok, "Can't assign to the left hand side");
        return false;
    }

    return true;
}


/* Main internal compilation method 
 */
static bool compile(struct compiler* co, ks_str fname, ks_str src, ks_code code, ks_ast v) {

    int i, j;

    /* Capture information that is frequently reused */
    int k = v->kind;
    int ssl = LEN;


    if (k == KS_AST_CONST) {
        EMITO(KSB_PUSH, v->val);
        META(v->tok);
        LEN += 1;
    } else if (k == KS_AST_NAME) {
        EMITO(KSB_LOAD, v->val);
        META(v->tok);
        LEN += 1;
    } else if (k == KS_AST_ATTR) {
        assert(NSUB == 1);
        if (!COMPILE(SUB(0))) return false;
        EMITO(KSB_GETATTR, v->val);
        META(v->tok);
        LEN += 1 - 1;
    } else if (k == KS_AST_ELEM) {
        assert(NSUB > 0);
        for (i = 0; i < NSUB; ++i) {
            if (!COMPILE(SUB(i))) return false;
        }

        EMITI(KSB_GETELEMS, NSUB);
        META(v->tok);
        LEN += 1 - NSUB;

    } else if (k == KS_AST_CALL) {
        for (i = 0; i < NSUB; ++i) {
            if (!COMPILE(SUB(i))) return false;
        }
        assert((LEN - ssl) == NSUB);
        EMITI(KSB_CALL, NSUB);
        LEN += 1 - NSUB;
        META(v->tok);

    } else if (k == KS_AST_RET) {
        assert(NSUB == 1);
        if (!COMPILE(SUB(0))) return false;
        assert((LEN - ssl) == 1);
        EMIT(KSB_RET);
        LEN -= 1;
        META(v->tok);

    } else if (k == KS_AST_THROW) {
        assert(NSUB == 1);
        if (!COMPILE(SUB(0))) return false;
        assert((LEN - ssl) == 1);
        EMIT(KSB_THROW);
        LEN -= 1;
        META(v->tok);

    } else if (k == KS_AST_IMPORT) {
        EMITO(KSB_IMPORT, v->val);
        LEN += 1;
        META(v->tok);
 
        EMITO(KSB_STORE, v->val);
        EMIT(KSB_POPU);
        LEN -= 1;

    } else if (k == KS_AST_BLOCK) {
        for (i = 0; i < NSUB; ++i) {
            if (!COMPILE(SUB(i))) return false;
            CLEAR(ssl);
        }
        META(v->tok);
    } else if (k == KS_AST_LIST) {
        for (i = 0; i < NSUB; ++i) {
            if (!COMPILE(SUB(i))) return false;
        }
        assert(LEN == ssl + NSUB);
        EMITI(KSB_LIST, NSUB);
        META(v->tok);
        LEN += 1 - NSUB;
    } else if (k == KS_AST_TUPLE) {
        for (i = 0; i < NSUB; ++i) {
            if (!COMPILE(SUB(i))) return false;
        }
        assert(LEN == ssl + NSUB);
        EMITI(KSB_TUPLE, NSUB);
        META(v->tok);
        LEN += 1 - NSUB;


    } else if (k == KS_AST_IF) {
        /* Emit conditional */
        assert((NSUB == 2 || NSUB == 3) && "'if' AST requires either 2 or 3 children");
        if (!COMPILE(SUB(0))) return false;

        /* CFG:
         * 
         * +---+  +---+
         * | B |--| T |--+--
         * +---+  +---+  |
         *   |    +---+  |
         *   +----| F |--+
         *        +---+
         */

        int jb_l = BC_N;
        EMITI(KSB_JMPF, -1);
        int jb_f = BC_N;
        LEN -= 1;

        if (!COMPILE(SUB(1))) return false;
        CLEAR(ssl);

        if (NSUB == 3) {
            /* Have an 'else' clause */
            int jt_l = BC_N;
            EMITI(KSB_JMP, -1);
            int jt_f = BC_N;

            /* False branch should jump to here */
            PATCH(jb_l, jb_f, BC_N);

            if (!COMPILE(SUB(2))) return false;
            CLEAR(ssl);

            /* Patch true branch to jump to after the 'else' */
            PATCH(jt_l, jt_f, BC_N);

        } else {
            /* No 'else' clause */
            PATCH(jb_l, jb_f, BC_N);
        }

    } else if (k == KS_AST_WHILE) {
        /* Emit conditional */
        assert((NSUB == 2 || NSUB == 3) && "'while' AST requires either 2 or 3 children");
        int cond_l = BC_N;
        if (!COMPILE(SUB(0))) return false;

        /* CFG:
         *       +------+
         *       |      |
         * +---+  +---+ |
         * | B |--| T |--+--
         * +---+  +---+  |
         *   |    +---+  |
         *   +----| F |--+
         *        +---+
         * 
         */

        int jc_l = BC_N;
        EMITI(KSB_JMPF, -1);
        int jc_f = BC_N;
        LEN -= 1;

        int body_l = BC_N;

        if (!COMPILE(SUB(1))) return false;
        CLEAR(ssl);

        if (!COMPILE(SUB(0))) return false;

        int jc2_l = BC_N;
        EMITI(KSB_JMPT, -1);
        int jc2_f = BC_N;
        LEN -= 1;

        PATCH(jc2_l, jc2_f, body_l);

        if (NSUB == 3) {

            /* We need to jump past the else clause by default */
            int je_l = BC_N;
            EMITI(KSB_JMP, -1);
            int je_f = BC_N;

            PATCH(jc_l, jc_f, BC_N);

            if (!COMPILE(SUB(2))) return false;
            CLEAR(ssl);

            /* Patch true branch to jump to after the 'else' */
            PATCH(je_l, je_f, BC_N);

        } else {
            /* No 'else' clause, just fall through */
            PATCH(jc_l, jc_f, BC_N);

        }

        LEN = ssl;

    } else if (k == KS_AST_FOR) {
        /* Emit conditional */
        assert((NSUB == 3 || NSUB == 4) && "'for' AST requires either 3 or 4 children");
        int cond_l = BC_N;
        if (!COMPILE(SUB(1))) return false;
        assert(LEN == ssl + 1);
        EMIT(KSB_FOR_START);
        META(SUB(1)->tok);

        ks_tok blame = ks_tok_combo(SUB(0)->tok, SUB(1)->tok);

        /* CFG:
         *       +------+
         *       |      |
         * +---+  +---+ |
         * | B |--| T |--+--
         * +---+  +---+  |
         *   |    +---+  |
         *   +----| F |--+
         *        +---+
         * 
         */

        int jc_l = BC_N;
        EMITI(KSB_FOR_NEXTF, -1);
        META(blame);
        int jc_f = BC_N;
        LEN += 1;

        /* Assign to the for loop capture */
        if (!assign(co, fname, src, code, SUB(0), SUB(1), v)) return NULL;
        CLEAR(ssl+1);

        int body_l = BC_N;

        if (!COMPILE(SUB(2))) return false;
        CLEAR(ssl+1);

        int jc2_l = BC_N;
        EMITI(KSB_FOR_NEXTT, -1);
        META(blame);
        int jc2_f = BC_N;

        PATCH(jc2_l, jc2_f, jc_f);

        if (NSUB == 4) {

            /* We need to jump past the else clause by default */
            int je_l = BC_N;
            EMITI(KSB_JMP, -1);
            int je_f = BC_N;

            PATCH(jc_l, jc_f, BC_N);

            if (!COMPILE(SUB(2))) return false;
            CLEAR(ssl+1);

            /* Patch true branch to jump to after the 'else' */
            PATCH(je_l, je_f, BC_N);

        } else {
            /* No 'else' clause, just fall through */
            PATCH(jc_l, jc_f, BC_N);

        }

        LEN = ssl;
    } else if (k == KS_AST_TRY) {
        /* Try/catch block */

        /* Begin by creating a try block */
        int sj_l = BC_N;
        EMITI(KSB_TRY_START, -1);
        int sj_f = BC_N;

        /* Try to execute the main body */
        if (!COMPILE(SUB(0))) return false;

        /* End the try block */
        int ej_l = BC_N;
        EMITI(KSB_TRY_END, -1);
        int ej_f = BC_N;

        /* Start the error handler */
        PATCH(sj_l, sj_f, BC_N);


        /* Number of catch clauses */
        int n_catches = (NSUB - 1) / 3;
        bool has_finally = NSUB % 3 == 2;

        int *js_l = ks_zmalloc(sizeof(*js_l), n_catches);
        int *js_f = ks_zmalloc(sizeof(*js_f), n_catches);

        /* Iterate over error handlers, and generate the code */
        int lj_l = -1, lj_f = -1;
        for (i = 0; i < n_catches; ++i) {
            ks_ast tp = SUB(3*i+1), to = SUB(3*i+2), body = SUB(3*i+3);

            int cp = BC_N;

            if (!COMPILE(tp)) {
                ks_free(js_l);
                ks_free(js_f);
                return false;
            }

            int tj_l = BC_N;
            EMITI(KSB_TRY_CATCH, -1);
            LEN -= 1;
            int tj_f = BC_N;
            
            
            if (to->kind != KS_AST_CONST) {
                /* Assign TOS to the capture variable */
                if (!assign(co, fname, src, code, to, tp, v)) {
                    ks_free(js_l);
                    ks_free(js_f);
                    return false;
                }
            }

            CLEAR(ssl);

            if (!COMPILE(body)) {
                ks_free(js_l);
                ks_free(js_f);
                return false;
            }

            js_l[i] = BC_N;
            EMITI(KSB_JMP, -1);
            js_f[i] = BC_N;

            /* Have the previous case jump to the case we just generated */
            if (lj_f >= 0) {
                PATCH(lj_l, lj_f, cp);
            }

            /* Now, set the next jumps */
            lj_l = tj_l;
            lj_f = tj_f;
        }

        if (lj_f >= 0) {
            PATCH(lj_l, lj_f, BC_N);
        }

        /* Jump to here for the end */
        PATCH(ej_l, ej_f, BC_N);
        for (i = 0; i < n_catches; ++i) {
            PATCH(js_l[i], js_f[i], BC_N);
        }

        ks_free(js_l);
        ks_free(js_f);

        /* Now, add the 'finally' block */
        if (has_finally) {
            if (!COMPILE(SUB(NSUB - 1))) return false;
        }

        EMIT(KSB_FINALLY_END);

        LEN = ssl;

    /** Handle Special Operators **/

    } else if (k == KS_AST_BOP_QUESQUES) {
        /* Short circuiting OR operator */
        assert(NSUB == 2 && "binary operator requires 2 children");

        /* Begin by creating a try block */
        int sj_l = BC_N;
        EMITI(KSB_TRY_START, -1);
        int sj_f = BC_N;

        /* Try to execute the children */
        if (!COMPILE(SUB(0))) return false;

        /* End the try block */
        int ej_l = BC_N;
        EMITI(KSB_TRY_END, -1);
        int ej_f = BC_N;

        /* If there was an exception, it will jump to here and use the other child */
        int cl_l = BC_N;
        EMITI(KSB_TRY_CATCH_ALL, 0);
        EMIT(KSB_POPU);
        LEN = ssl;
        
        if (!COMPILE(SUB(1))) return false;

        /* Now, the TOS will be the exception */
        LEN = ssl + 1;

        /* Patch the 'try' handle to jump to other child */
        PATCH(sj_l, sj_f, cl_l);
        
        PATCH(ej_l, ej_f, BC_N);
        
    } else if (k == KS_AST_BOP_OROR) {
        /* Short circuiting OR operator */
        assert(NSUB == 2 && "binary operator requires 2 children");
        if (!COMPILE(SUB(0))) return false;

        /* First, duplicate it and jump if its truthy (and jump over execution of the other child) */
        EMIT(KSB_DUP);
        LEN += 1;
        int sj_l = BC_N;
        EMITI(KSB_JMPT, -1);
        int sj_f = BC_N;
        LEN -= 1;

        /* Not truthy, so pop off the last result */
        EMIT(KSB_POPU);
        LEN -= 1;
        if (!COMPILE(SUB(1))) false;

        /* Patch the jump from earlier */
        PATCH(sj_l, sj_f, BC_N);
    } else if (k == KS_AST_BOP_ANDAND) {
        /* Short circuiting AND operator */
        assert(NSUB == 2 && "binary operator requires 2 children");
        if (!COMPILE(SUB(0))) return false;

        /* First, duplicate it and jump if its truthy (and jump over execution of the other child) */
        EMIT(KSB_DUP);
        LEN += 1;
        int sj_l = BC_N;
        EMITI(KSB_JMPF, -1);
        int sj_f = BC_N;
        LEN -= 1;

        /* Not truthy, so pop off the last result */
        EMIT(KSB_POPU);
        LEN -= 1;
        if (!COMPILE(SUB(1))) false;

        /* Patch the jump from earlier */
        PATCH(sj_l, sj_f, BC_N);
    } else if (k == KS_AST_BOP_ASSIGN) {
        assert(NSUB == 2 && "binary operator requires 2 children");
        if (!COMPILE(SUB(1))) return false;

        if (!assign(co, fname, src, code, SUB(0), SUB(1), v)) return false;

    } else if (KS_AST_BOP__FIRST <= k && k <= KS_AST_BOP__LAST) {
        assert(NSUB == 2 && "binary operator requires 2 children");
        /* Binary operator */
        if (!COMPILE(SUB(0)) || !COMPILE(SUB(1))) return false;

        /* The enumerations are set up so that the AST types match directly to the bytecode 
         *   index, so we can just emit it
         */
        EMIT(k);
        META(v->tok);
        LEN += 1 - 2;

    } else if (KS_AST_UOP__FIRST <= k && k <= KS_AST_UOP__LAST) {
        assert(NSUB == 1 && "unary operator requires 1 children");
        /* Binary operator */
        if (!COMPILE(SUB(0))) return false;

        /* The enumerations are set up so that the AST types match directly to the bytecode 
         *   index, so we can just emit it
         */
        EMIT(k);
        META(v->tok);
        LEN += 1 - 1;

    } else {
        /* unknown */
        KS_THROW_SYNTAX(fname, src, v->tok, "Haven't implemented AST nodes of kind '%i'", v->kind);
        return false;
    }

    return true;
}


/* Export */

ks_code ks_compile(ks_str fname, ks_str src, ks_ast prog, ks_code from) {
    ks_code res = from ? ks_code_from(from) : ks_code_new(fname, src);
    if (!res) return NULL;

    struct compiler co;
    co.len_stk = 0;
    if (!compile(&co, fname, src, res, prog)) {
        KS_DECREF(res);
        return NULL;
    }

    /* Default of 'ret none' */
    ks_code_emito(res, KSB_PUSH, KSO_NONE);
    ks_code_emit(res, KSB_RET);
    return res;
}




