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

    /* Number of break-able loops present (while,for) */
    int loop_n;

    /* Array of loops */
    struct compiler_loop {

        /* Required stack length */
        int stklen;
        
        /* Number of control-flow-actions (CFAs) */
        int cfa_n;

        /* List of actions */
        struct compiler_loop_action {

            /* Location at which the instruction is */
            int loc;

            /* Location from which the instruction is jumping from
            * (always 'loc+5')
            */
            int from;

            /* Whether it should continue (else break) */
            bool is_cont;

        }* cfa;

    }* loop;
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
#define EMITO(_op, _oval) ks_code_emito(code, (_op), (kso)(_oval))

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


/* Computes assignment, to the TOS
 *
 * If 'aug' is positive, then it should be augmented assignment with that binary operator (i.e. give KS_AST_BOP_ADD for augmented
 *   assignment)
 * If 'aug' is zero, then it is normal assignment
 * If 'aug' is negative, then RHS has already been emitted and is at the TOS
 * 
 */
static bool assign(struct compiler* co, ks_str fname, ks_str src, ks_code code, ks_ast lhs, ks_ast rhs, ks_ast par, int aug) {
    int i, ssl = LEN;
    if (lhs->kind == KS_AST_NAME) {
        if (aug < 0) {
            /* Already emitted */            
            EMITO(KSB_STORE, lhs->val);
        } else if (aug > 0) {
            /* Augmented assignment */
            EMITO(KSB_LOAD, lhs->val);
            LEN += 1;

            /* Perform operation */
            if (!COMPILE(rhs)) return false;
            EMIT(aug);
            LEN += 1 - 2;

            /* Store back */
            EMITO(KSB_STORE, lhs->val);

        } else {
            /* Just store in name */
            if (!COMPILE(rhs)) return false;
            EMITO(KSB_STORE, lhs->val);
        }

    } else if (lhs->kind == KS_AST_ATTR) {
        if (aug < 0) {
            /* Already emitted */      

            /* Store as attribute */      
            if (!COMPILE((ks_ast)lhs->args->elems[0])) return false;
            EMITO(KSB_SETATTR, lhs->val);
            LEN--;
            META(par->tok);
        } else if (aug > 0) {
            /* First, get the attribute */

            if (!COMPILE(lhs)) return false;
            if (!COMPILE(rhs)) return false;
            EMIT(aug);
            LEN += 1 - 2;

            if (!COMPILE((ks_ast)lhs->args->elems[0])) return false;
            EMITO(KSB_SETATTR, lhs->val);
            LEN--;
            META(par->tok);

        } else {
            if (!COMPILE(rhs)) return false;
            if (!COMPILE((ks_ast)lhs->args->elems[0])) return false;
            EMITO(KSB_SETATTR, lhs->val);
            LEN--;
            META(par->tok);
        }

    } else if (lhs->kind == KS_AST_ELEM) {
        if (aug < 0) {
            /* Already emitted */

            /* Store as element (awkward since it is already emitted) */
            for (i = 0; i < lhs->args->len; ++i) {
                if (!COMPILE((ks_ast)lhs->args->elems[i])) return false;
            }

            EMITI(KSB_DUPI, -lhs->args->len-1);
            EMITI(KSB_SETELEMS, 1 + lhs->args->len);
            EMIT(KSB_POPU);
            LEN += 1 - (1 + lhs->args->len);
            META(par->tok);

        } else if (aug > 0) {
            /* Augmented assignment */
            /* Emit all arguments */
            for (i = 0; i < lhs->args->len; ++i) {
                if (!COMPILE((ks_ast)lhs->args->elems[i])) return false;
            }
            assert(LEN == ssl + lhs->args->len);
            EMITI(KSB_DUPN, lhs->args->len);
            LEN += lhs->args->len;
            EMITI(KSB_GETELEMS, lhs->args->len);
            LEN += 1 - lhs->args->len;
            META(lhs->tok);
            assert(LEN == ssl + 1 + lhs->args->len);
            if (!COMPILE(rhs)) return false;

            /* Do operation */
            EMIT(aug);
            LEN += 1 - 2;

            assert(LEN == ssl + 1 + lhs->args->len);

            /* Now, store element */
            EMITI(KSB_SETELEMS, 1 + lhs->args->len);
            LEN += - lhs->args->len;
            META(par->tok);

            assert(LEN == ssl + 1);

        } else {
            
            /* Emit all arguments */
            for (i = 0; i < lhs->args->len; ++i) {
                if (!COMPILE((ks_ast)lhs->args->elems[i])) return false;
            }
            if (!COMPILE(rhs)) return false;
            EMITI(KSB_SETELEMS, 1 + lhs->args->len);
            LEN += - lhs->args->len;
            META(par->tok);
            assert(LEN == ssl + 1);
        }
    } else if (lhs->kind == KS_AST_TUPLE) {
        /* Multi-assignment */
        if (aug < 0) {
            /* Already emitted */
        } else if (aug > 0) {
            KS_THROW_SYNTAX(fname, src, par->tok, "Can't do augmented assignment to tuple");
            return false;
        } else {
            if (!COMPILE(rhs)) return false;
        }

        /* Find arguments */
        int vararg_idx = -1;
        for (i = 0; i < lhs->args->len; ++i) {
            ks_ast child = (ks_ast)lhs->args->elems[i];
            if (child->kind == KS_AST_UOP_STAR) {
                if (vararg_idx >= 0) {
                    /* Already found a '*' target */
                    KS_THROW_SYNTAX(fname, src, par->tok, "May only have one '*' target per assignment");
                    return false;
                }
                vararg_idx = i;
            }
        }

        /* Now, given that there was 1 item on the stack that we must assign to the tuple,
        *   we need to break it up into the correct length to process
        */
        EMITI(KSB_ASSV, vararg_idx);
        EMITI(KSB_ASSM, lhs->args->len);
        META(rhs->tok);
        
        for (i = 0; i < lhs->args->len; ++i) {
            ks_ast child = (ks_ast)lhs->args->elems[i];
            if (child->kind == KS_AST_UOP_STAR) {
                /* Replace with its child, since that has already been handled */
                child = (ks_ast)child->args->elems[0];
            }
            if (!assign(co, fname, src, code, child, rhs, par, -1)) return false;
            META(child->tok);
            EMIT(KSB_POPU);
        }
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
            if (SUB(i)->kind == KS_AST_UOP_STAR) {
                break;
            }
            if (!COMPILE(SUB(i))) return false;
        }
        //assert((LEN - ssl) == i);

        if (i == NSUB) {
            /* Normal call */
            EMITI(KSB_CALL, i);
            META(v->tok);
            LEN += 1 - i;
        } else {
            EMITI(KSB_LIST, i);
            LEN += 1 - i;
            for (j = i+1; i < NSUB; ++i) {
                if (SUB(i)->kind == KS_AST_UOP_STAR) {
                    if (i > j) {
                        EMITI(KSB_LIST_PUSHN, i - j);
                        LEN -= (i - j);
                        j = i+1;
                    }

                    if (!COMPILE((ks_ast)SUB(i)->args->elems[0])) return false;
                    EMIT(KSB_LIST_PUSHI);
                    LEN -= 1;
                } else {
                    if (!COMPILE(SUB(i))) return false;
                }
            }
            //i = NSUB - 1;
            if (i > j) {
                EMITI(KSB_LIST_PUSHN, i - j);
                LEN -= (i - j);
                j = i+1;
            }

            EMIT(KSB_CALLV);
            LEN += 1 - 1;
            META(v->tok);
        }

    } else if (k == KS_AST_BREAK) {
        assert(NSUB == 0);

        if (co->loop_n <= 0) {
            KS_THROW_SYNTAX(fname, src, v->tok, "Unexpected 'break' outside of a loop");
            return false;
        }

        struct compiler_loop* loop = &co->loop[co->loop_n - 1];

        i = loop->cfa_n++;
        loop->cfa = ks_zrealloc(loop->cfa, sizeof(*loop->cfa), loop->cfa_n);

        /* Clear to the stack length */
        CLEAR(loop->stklen);

        LEN = ssl;

        /* Dummy values now */
        int l = BC_N;
        EMITI(KSB_JMP, -1);
        int f = BC_N;

        loop->cfa[i] = (struct compiler_loop_action) {
            l,
            f,
            false
        };

    } else if (k == KS_AST_CONT) {
        assert(NSUB == 0);

        if (co->loop_n <= 0) {
            KS_THROW_SYNTAX(fname, src, v->tok, "Unexpected 'cont' outside of a loop");
            return false;
        }

        struct compiler_loop* loop = &co->loop[co->loop_n - 1];

        i = loop->cfa_n++;
        loop->cfa = ks_zrealloc(loop->cfa, sizeof(*loop->cfa), loop->cfa_n);

        /* Clear to the stack length */
        CLEAR(loop->stklen);

        LEN = ssl;

        /* Dummy values now */
        int l = BC_N;
        EMITI(KSB_JMP, -1);
        int f = BC_N;

        loop->cfa[i] = (struct compiler_loop_action) {
            l,
            f,
            true
        };
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

    } else if (k == KS_AST_ASSERT) {
        assert(NSUB == 1);
        if (!COMPILE(SUB(0))) return false;
        assert((LEN - ssl) == 1);
        ks_str msg = ks_tok_str(src, SUB(0)->tok);
        EMITO(KSB_ASSERT, msg);
        KS_DECREF(msg);
        LEN -= 1;
        META(v->tok);
    } else if (k == KS_AST_IMPORT) {
        EMITO(KSB_IMPORT, v->val);
        LEN += 1;
        META(v->tok);

        ks_str name = (ks_str)v->val;
        int ip = 0;
        while (ip < name->len_b && name->data[ip] != '.') {
            ip++;
        }
        ks_str toname = ks_str_new(ip, name->data);

        EMITO(KSB_STORE, toname);
        KS_DECREF(toname);
        EMIT(KSB_POPU);
        LEN -= 1;

    } else if (k == KS_AST_BLOCK) {
        for (i = 0; i < NSUB; ++i) {
            if (!COMPILE(SUB(i))) return false;
            CLEAR(ssl);
        }
        META(v->tok);
    } else if (k == KS_AST_SLICE) {
        assert(NSUB == 3);
        if (!COMPILE(SUB(0))) return false;
        if (!COMPILE(SUB(1))) return false;
        if (!COMPILE(SUB(2))) return false;

        EMIT(KSB_SLICE);
        META(v->tok);
        LEN += 1 - 3;

    } else if (k == KS_AST_LIST) {
        for (i = 0; i < NSUB; ++i) {
            if (SUB(i)->kind == KS_AST_UOP_STAR) {
                break;
            }
            if (!COMPILE(SUB(i))) return false;
        }
        EMITI(KSB_LIST, i);
        LEN += 1 - i;

        META(v->tok);
        for (j = i+1; i < NSUB; ++i) {
            if (SUB(i)->kind == KS_AST_UOP_STAR) {
                if (i > j) {
                    EMITI(KSB_LIST_PUSHN, i - j);
                    LEN -= (i - j);
                    j = i+1;
                }
                if (!COMPILE((ks_ast)SUB(i)->args->elems[0])) return false;
                EMIT(KSB_LIST_PUSHI);
                LEN -= 1;
            } else {
                if (!COMPILE(SUB(i))) return false;
            }
        }
        if (i > j) {
            EMITI(KSB_LIST_PUSHN, i - j);
            LEN -= (i - j);
            j = i+1;
        }

    } else if (k == KS_AST_TUPLE) {
        for (i = 0; i < NSUB; ++i) {
            if (SUB(i)->kind == KS_AST_UOP_STAR) {
                break;
            }
            if (!COMPILE(SUB(i))) return false;
        }
        if (i == NSUB) {
            /* Just create tuple */
            EMITI(KSB_TUPLE, i);
            LEN += 1 - i;
            META(v->tok);
        } else {
            /* Built tuple */
            EMITI(KSB_TUPLE, i);
            LEN += 1 - i;
            META(v->tok);
            for (j = i+1; i < NSUB; ++i) {
                if (SUB(i)->kind == KS_AST_UOP_STAR) {
                    if (i > j) {
                        EMITI(KSB_TUPLE_PUSHN, i - j);
                        LEN -= (i - j);
                        j = i+1;
                    }

                    if (!COMPILE((ks_ast)SUB(i)->args->elems[0])) return false;
                    EMIT(KSB_TUPLE_PUSHI);
                    LEN -= 1;
                } else {
                    if (!COMPILE(SUB(i))) return false;

                }
            }
            if (i > j) {
                EMITI(KSB_TUPLE_PUSHN, i - j);
                LEN -= (i - j);
                j = i+1;
            }
        }
    } else if (k == KS_AST_SET) {
        for (i = 0; i < NSUB; ++i) {
            if (SUB(i)->kind == KS_AST_UOP_STAR) {
                break;
            }
            if (!COMPILE(SUB(i))) return false;
        }
        EMITI(KSB_SET, i);
        LEN += 1 - i;

        META(v->tok);
        for (j = i+1; i < NSUB; ++i) {
            if (SUB(i)->kind == KS_AST_UOP_STAR) {
                if (i > j) {
                    EMITI(KSB_SET_PUSHN, i - j);
                    LEN -= (i - j);
                    j = i+1;
                }

                if (!COMPILE((ks_ast)SUB(i)->args->elems[0])) return false;
                EMIT(KSB_SET_PUSHI);
                LEN -= 1;
            } else {
                if (!COMPILE(SUB(i))) return false;

            }
        }
        i = NSUB - 1;

        if (i > j) {
            EMITI(KSB_SET_PUSHN, i - j);
            LEN -= (i - j);
            j = i+1;
        }
    } else if (k == KS_AST_DICT) {
        assert(NSUB % 2 == 0);
        for (i = 0; i < NSUB; ++i) {
            if (SUB(i)->kind == KS_AST_UOP_STAR) {
                break;
            }
            if (!COMPILE(SUB(i))) return false;
        }
        EMITI(KSB_DICT, i);
        LEN += 1 - i;

        META(v->tok);
        for (j = i+1; i < NSUB; ++i) {
            if (SUB(i)->kind == KS_AST_UOP_STAR) {
                if (i > j) {
                    EMITI(KSB_DICT_PUSHN, i - j);
                    LEN -= (i - j);
                    j = i+1;
                }

                if (!COMPILE((ks_ast)SUB(i)->args->elems[0])) return false;
                EMIT(KSB_SET_PUSHI);
                LEN -= 1;
            } else {
                if (!COMPILE(SUB(i))) return false;

            }
        }
        i = NSUB - 1;

        if (i > j) {
            EMITI(KSB_DICT_PUSHN, i - j);
            LEN -= (i - j);
            j = i+1;
        }

    } else if (k == KS_AST_FUNC) {
        assert(NSUB == 2);
        /* info = (name, sig, names, doc) */
        ks_tuple info = (ks_tuple)v->val;
        assert(info && kso_issub(info->type, kst_tuple) && info->len == 4);
        ks_ast params = SUB(0), body = SUB(1);
        assert(params->kind == KS_AST_TUPLE);

        /* Index of the vararg, and the start of the default arguments */
        int vararg_idx = -1, defa_idx = -1;
        for (i = 0; i < params->args->len; ++i) {
            ks_ast par = (ks_ast)params->args->elems[i];
            if (par->kind == KS_AST_UOP_STAR) {
                /* Vararg (*NAME) */
                if (vararg_idx >= 0) {
                    KS_THROW_SYNTAX(fname, src, par->tok, "Given multiple '*' parameters; there may only be one in a 'func' definition");
                    return false;
                }
                vararg_idx = i;
                assert(par->args->len == 1);
                par = (ks_ast)par->args->elems[0];
                assert(par->kind == KS_AST_NAME);
            } else if (par->kind == KS_AST_BOP_ASSIGN) {
                /* Default parameter (NAME=VAL) */
                if (defa_idx < 0) defa_idx = i;
                assert(par->args->len == 2);
                par = (ks_ast)par->args->elems[0];

            } else if (par->kind == KS_AST_NAME) {
                /* Just normal name (NAME) */
                if (defa_idx >= 0) {
                    KS_THROW_SYNTAX(fname, src, par->tok, "Given normal parameter after default parameter");
                    return false;
                }

            } else {
                KS_THROW_SYNTAX(fname, src, par->tok, "Invalid parameter to %s", v->kind == KS_AST_FUNC ? "function" : "lambda");
                return false;
            }
        }

        if (vararg_idx >= 0 && defa_idx >= 0) {
            KS_THROW_SYNTAX(fname, src, params->tok, "Given '*' parameters and default parameters (may not have both)");
            return false;
        }

        ks_int t = ks_int_new((ks_cint)vararg_idx);
        ks_tuple newinfo = ks_tuple_new(5, (kso[]){
            info->elems[0],
            info->elems[1],
            info->elems[2],
            info->elems[3],
            (kso)t
        });
        KS_DECREF(t);

        ks_code body_bc = ks_compile((ks_str)info->elems[1], src, body, NULL);
        if (!body_bc) {
            KS_DECREF(newinfo);
            return NULL;
        }
        body_bc->tok = ks_tok_combo(params->tok, params->tok);

        EMITO(KSB_PUSH, body_bc);
        KS_DECREF(body_bc);
        LEN += 1;

        EMITO(KSB_FUNC, newinfo);
        LEN += 1 - 1;
        KS_DECREF(newinfo);

        if (((ks_str)info->elems[0])->data[0] != '<') {
            EMITO(KSB_STORE, info->elems[0]);
        }

        /* Now, emit the defaults */
        int n_defa = 0;
        if (defa_idx >= 0) {
            n_defa = params->args->len - defa_idx;
            for (i = defa_idx; i < params->args->len; ++i) {
                ks_ast par = (ks_ast)params->args->elems[i];
                assert(par->kind == KS_AST_BOP_ASSIGN && par->args->len == 2);
                par = (ks_ast)par->args->elems[1];
                if (!COMPILE(par)) return false;
            }
            EMITI(KSB_FUNC_DEFA, n_defa);
            LEN -= n_defa;
        }
    } else if (k == KS_AST_TYPE) {
        assert(NSUB == 2);
        ks_tuple info = (ks_tuple)v->val;
        assert(info && kso_issub(info->type, kst_tuple) && info->len == 2);
        ks_ast ext = SUB(0), body = SUB(1);

        ks_code body_bc = ks_compile((ks_str)info->elems[0], src, body, code);
        if (!body_bc) return false;

        body_bc->tok = v->tok;

        EMITO(KSB_PUSH, body_bc);
        KS_DECREF(body_bc);
        LEN += 1;

        if (!COMPILE(ext)) return false;

        EMITO(KSB_TYPE, info);
        LEN += 1 - 2;

        /* Store as a name */
        if (((ks_str)info->elems[0])->data[0] != '<') {
            EMITO(KSB_STORE, info->elems[0]);
        }
    } else if (k == KS_AST_ENUM) {
        assert(NSUB == 1);
        ks_tuple info = (ks_tuple)v->val;
        assert(info && kso_issub(info->type, kst_tuple) && info->len == 2);
        ks_ast body = SUB(0);

        ks_code body_bc = ks_compile((ks_str)info->elems[0], src, body, code);
        if (!body_bc) return false;

        body_bc->tok = v->tok;

        /* Create ENUM */
        assert(false && "TODO: allow enum syntax to compile");

        /* Store as a name */
        if (((ks_str)info->elems[0])->data[0] != '<') {
            EMITO(KSB_STORE, info->elems[0]);
        }
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


        /* Add loop */
        int st_i = co->loop_n++;
        co->loop = ks_zrealloc(co->loop, sizeof(*co->loop), co->loop_n);

        co->loop[st_i].cfa_n = 0;
        co->loop[st_i].cfa = NULL;
        co->loop[st_i].stklen = ssl;

        int body_l = BC_N;
        if (!COMPILE(SUB(1))) return false;
        CLEAR(ssl);

        if (!COMPILE(SUB(0))) return false;

        int jc2_l = BC_N;
        EMITI(KSB_JMPT, -1);
        int jc2_f = BC_N;
        LEN -= 1;

        PATCH(jc2_l, jc2_f, body_l);

        /* Pop off the loops */
        int cfa_n = co->loop[st_i].cfa_n;
        struct compiler_loop_action* cfa = co->loop[st_i].cfa;
        co->loop_n--;

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

        /* Fill in break/cont */
        int i;
        for (i = 0; i < cfa_n; ++i) {
            struct compiler_loop_action* a = &cfa[i];

            PATCH(a->loc, a->from, a->is_cont ? cond_l : BC_N);
        }
        ks_free(cfa);

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
        if (!assign(co, fname, src, code, SUB(0), SUB(1), v, -1)) return NULL;
        CLEAR(ssl+1);

        int body_l = BC_N;

        /* Add loop */
        int st_i = co->loop_n++;
        co->loop = ks_zrealloc(co->loop, sizeof(*co->loop), co->loop_n);

        co->loop[st_i].cfa_n = 0;
        co->loop[st_i].cfa = NULL;
        co->loop[st_i].stklen = ssl;

        if (!COMPILE(SUB(2))) return false;
        CLEAR(ssl+1);

        int jc2_l = BC_N;
        EMITI(KSB_FOR_NEXTT, -1);
        META(blame);
        int jc2_f = BC_N;

        PATCH(jc2_l, jc2_f, jc_f);


        /* Pop off the loops */
        int cfa_n = co->loop[st_i].cfa_n;
        struct compiler_loop_action* cfa = co->loop[st_i].cfa;
        co->loop_n--;

        if (NSUB == 4) {

            /* We need to jump past the else clause by default */
            int je_l = BC_N;
            EMITI(KSB_JMP, -1);
            int je_f = BC_N;

            PATCH(jc_l, jc_f, BC_N);

            if (!COMPILE(SUB(3))) return false;
            CLEAR(ssl+1);

            /* Patch true branch to jump to after the 'else' */
            PATCH(je_l, je_f, BC_N);

        } else {
            /* No 'else' clause, just fall through */
            PATCH(jc_l, jc_f, BC_N);

        }
        /* Fill in break/cont */
        int i;
        for (i = 0; i < cfa_n; ++i) {
            struct compiler_loop_action* a = &cfa[i];

            PATCH(a->loc, a->from, a->is_cont ? body_l : BC_N);
        }
        ks_free(cfa);

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
                if (!assign(co, fname, src, code, to, tp, v, -1)) {
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
    } else if (k == KS_AST_COND) {
        assert(NSUB == 3 && "ternary operator requires 3 children");

        /* First, compile the actual expression */
        if (!COMPILE(SUB(0))) return false;

        int sj_l = BC_N;
        EMITI(KSB_JMPF, -1);
        LEN -= 1;
        int sj_f = BC_N;

        if (!COMPILE(SUB(1))) return false;

        int tj_l = BC_N;
        EMITI(KSB_JMP, -1);
        int tj_f = BC_N;

        /* If false, jump directly to here */
        PATCH(sj_l, sj_f, BC_N);
        LEN -= 1;

        if (!COMPILE(SUB(2))) return false;

        /* If true, skip over the falsey */
        PATCH(tj_l, tj_f, BC_N);

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
        if (!assign(co, fname, src, code, SUB(0), SUB(1), v, 0)) return false;

    } else if (KS_AST_BOP__AFIRST <= k && k <= KS_AST_BOP__ALAST) {
        assert(NSUB == 2 && "binary operator requires 2 children");

        int vk;
        if (k == KS_AST_BOP_AADD) vk = KS_AST_BOP_ADD;
        else if (k == KS_AST_BOP_ASUB) vk = KS_AST_BOP_SUB;
        else if (k == KS_AST_BOP_AMUL) vk = KS_AST_BOP_MUL;
        else if (k == KS_AST_BOP_AMATMUL) vk = KS_AST_BOP_MATMUL;
        else if (k == KS_AST_BOP_ADIV) vk = KS_AST_BOP_DIV;
        else if (k == KS_AST_BOP_AFLOORDIV) vk = KS_AST_BOP_FLOORDIV;
        else if (k == KS_AST_BOP_AMOD) vk = KS_AST_BOP_MOD;
        else if (k == KS_AST_BOP_APOW) vk = KS_AST_BOP_POW;
        else if (k == KS_AST_BOP_ALSH) vk = KS_AST_BOP_LSH;
        else if (k == KS_AST_BOP_ARSH) vk = KS_AST_BOP_RSH;
        else if (k == KS_AST_BOP_AIOR) vk = KS_AST_BOP_IOR;
        else if (k == KS_AST_BOP_AXOR) vk = KS_AST_BOP_XOR;
        else if (k == KS_AST_BOP_AAND) vk = KS_AST_BOP_AND;
        else {
            assert(false);
        }

        if (!assign(co, fname, src, code, SUB(0), SUB(1), v, vk)) return false;

    } else if (k == KS_AST_RICHCMP) {
        /* Rich comparison chaining */
        ks_tuple cmps = (ks_tuple)v->val;
        assert(cmps && cmps->type == kst_tuple && cmps->len >= 1);
        assert (NSUB == cmps->len + 1);

        /* Always emit first child */
        if (!COMPILE(SUB(0))) return false;

        /* False-jumps from, locs */
        int fjs_n = NSUB - 1;
        int* fjs_f = ks_zmalloc(sizeof(*fjs_f), fjs_n), * fjs_l = ks_zmalloc(sizeof(*fjs_f), fjs_n);

        for (i = 1; i < NSUB; ++i) {
            /* On every iteration, we will have:
             * | c[i-1]
             * To start with
             */

            /* Emit current child so we have:
             * | c[i-1] c[i]
             */
            if (!COMPILE(SUB(i))) {
                ks_free(fjs_f);
                ks_free(fjs_l);
                return false;
            }
            EMIT(KSB_RCR);
            LEN += 1;

            /* Now, compare via the current child
             */
            ks_cint cmp_ast_kind;
            if (!kso_get_ci(cmps->elems[i - 1], &cmp_ast_kind)) {
                ks_free(fjs_f);
                ks_free(fjs_l);
                return false;
            }

            /* Perform comparison */
            EMIT(cmp_ast_kind);
            LEN += 1 - 2;

            /* Short circuit if given false */
            fjs_l[i - 1] = BC_N;
            EMITI(KSB_JMPF, -1);
            fjs_f[i - 1] = BC_N;
            LEN -= 1;

        }

        /* At the very end, we should put on a 'true' and then reach sync point */
        EMIT(KSB_POPU);
        EMITO(KSB_PUSH, KSO_TRUE);
        int tj_l = BC_N;
        EMITI(KSB_JMP, -1);
        int tj_f = BC_N;

        /* Fill in jumps */
        for (i = 0; i < fjs_n; ++i) {
            PATCH(fjs_l[i], fjs_f[i], BC_N);
        }

        EMIT(KSB_POPU);
        EMITO(KSB_PUSH, KSO_FALSE);

        PATCH(tj_l, tj_f, BC_N);

        ks_free(fjs_f);
        ks_free(fjs_l);

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

    } else if (k == KS_AST_UOP_POSPOS) {
        assert(NSUB == 1 && "unary operator requires 1 children");
        /* ++x */

        /* equivalent to 'x += 1' */
        ks_ast rhs = ks_ast_newn(KS_AST_CONST, 0, NULL, (kso)ks_int_new(1), v->tok);
        if (!assign(co, fname, src, code, SUB(0), rhs, v, KS_AST_BOP_ADD)) {
            KS_DECREF(rhs);
            return false;
        }
        KS_DECREF(rhs);

    } else if (k == KS_AST_UOP_NEGNEG) {
        assert(NSUB == 1 && "unary operator requires 1 children");
        /* --x */

        /* equivalent to 'x -= 1' */
        ks_ast rhs = ks_ast_newn(KS_AST_CONST, 0, NULL, (kso)ks_int_new(1), v->tok);
        if (!assign(co, fname, src, code, SUB(0), rhs, v, KS_AST_BOP_SUB)) {
            KS_DECREF(rhs);
            return false;
        }
        KS_DECREF(rhs);

    } else if (k == KS_AST_UOP_POSPOS_POST) {
        assert(NSUB == 1 && "unary operator requires 1 children");
        /* x++ */
        assert(false);
    } else if (k == KS_AST_UOP_NEGNEG_POST) {
        assert(NSUB == 1 && "unary operator requires 1 children");
        /* x-- */
        assert(false);

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
    //ks_code res = from ? ks_code_from(from) : ks_code_new(fname, src);
    ks_code res = ks_code_new(fname, src);
    if (!res) return NULL;

    struct compiler co;
    co.len_stk = 0;
    co.loop_n = 0;
    co.loop = NULL;
    if (!compile(&co, fname, src, res, prog)) {
        ks_free(co.loop);
        KS_DECREF(res);
        return NULL;
    }

    ks_free(co.loop);

    /* Default of 'ret none' */
    ks_code_emito(res, KSB_PUSH, KSO_NONE);
    ks_code_emit(res, KSB_RET);
    return res;
}




