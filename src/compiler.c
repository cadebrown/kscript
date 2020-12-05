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
    } else if (k == KS_AST_CALL) {
        for (i = 0; i < NSUB; ++i) {
            if (!COMPILE(SUB(i))) return false;
        }
        assert((LEN - ssl) == NSUB);
        EMITI(KSB_CALL, NSUB);
        LEN += 1 - NSUB;
        META(v->tok);

    } else if (k == KS_AST_BLOCK) {
        for (i = 0; i < NSUB; ++i) {
            if (!COMPILE(SUB(i))) return false;
            CLEAR(ssl);
        }
        META(v->tok);
    } else {
        /* unknown */
        KS_THROW_SYNTAX(fname, src, v->tok, "Haven't implemented '%i' AST nodes", v->kind);
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
    ks_code_emito(res, KSB_RET, KSO_NONE);
    return res;
}




