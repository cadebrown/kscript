/* vm.c - kscript virtual machine
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
 * 
 * 
 * 
 * @author: Cade Brown <cade@kscript.org>
 */
#include <ks/impl.h>
#include <ks/compiler.h>

/* VM - Virtual Machine
 *
 * A Virtual Machine (or VM for short) is an abstracted machine which can understand computations
 *   described in some language (in our case, bytecode, or BC/bc).
 * 
 * The full specification of the kscript bytecode format is defined in `compiler.h`, with the operations
 *   being `KSB_*` enumeration values. Some take just the op code (1 byte), and other take the op code
 *   and a signed integer value (5 bytes total, 1 byte opcode + 4 byte argument)
 * 
 * For actually implementing it, we have bytes in an array that are tightly packed. Then, we start at
 *   position 0, and iterate through, moving along and 'consuming' the bytecode as we go. However, some
 *   instructions may cause a jump backwards or forwards, so it is not always linear in that regard
 *   (and a bytecode with 100 bytes may execute thousands of isntructions). But the code is stored linear in memory,
 *   which is more efficient than an AST traversal, for example
 * 
 * The method used in the control loop is either a switch/case (default), or a computed goto (^0). The former
 *   is less error prone, allows for (some) error checking for malformed bytecode, but is not as (theoretically)
 *   fast as using computed goto. I intend to (once this code is a bit more mature) benchmark the results of
 *   each method and see which is faster
 * 
 * 
 * Possible optimizations:
 *   - Include list operations in this file, so to inline and optimize for specific cases
 * 
 * 
 * References:
 *   ^0: https://eli.thegreenplace.net/2012/07/12/computed-goto-for-efficient-dispatch-tables
 * 
 * @author: Cade Brown <cade@kscript.org>
 */


/** Utilities **/

/* Temporary unlock and relock the GIL so other threads can use it 
 * TODO: implement smarter switching
 */
#define VM_ALLOW_GIL() do { \
    KS_GIL_UNLOCK(); \
    KS_GIL_LOCK(); \
} while(0)

/* Dispatch/Execution (VMD==Virtual Machine Dispatch) */

/* Starts the VMD */
#define VMD_START while (true) switch (*pc)

/* Catches unknown instruction */
#define VMD_CATCH_REST default: fprintf(stderr, "[VM]: Unknown instruction encountered in <code @ %p>: %i (offset: %i)\n", bc, *pc, (int)(pc - bc->bc->data)); assert(false); break;

/* Consume the next instruction 
 * TODO: switch based on instructions or time
 */
#define VMD_NEXT() VM_ALLOW_GIL(); goto disp;

/* Declare code for a given operator */
#define VMD_OP(_op) case _op: pc += sizeof(ksb);

/* Declare code for a given operator, which takes an argument */
#define VMD_OPA(_op) case _op: arg = ((ksba*)pc)->arg; pc += sizeof(ksba);

/* End the section for an operator */
#define VMD_OP_END  VMD_NEXT(); break;


/* Check whether a type fits typeinfo */
static bool is_typeinfo(ks_type tp, kso info, bool* out) {

    if (kso_issub(info->type, kst_type)) {
        *out = kso_issub(tp, (ks_type)info);
        return true;
    } else if (kso_issub(info->type, kst_tuple)) {
        int i;
        ks_tuple tps = (ks_tuple)info;
        for (i = 0; i < tps->len; ++i) {
            if (!is_typeinfo(tp, tps->elems[i], out)) {
                return false;
            }
            if (*out) return true;
        }
        *out = false;
        return true;
    }

    KS_THROW(kst_TypeError, "Unexpected typeinfo: %R, should either be a type, or tuple of types");
    return false;
}


/* Execute on the current thread and return the result returned, or NULL if
 *   an exception was thrown.
 * 
 * This method does not add anything to the thread's stack frames (that should be
 *   done in the caller, for example in 'kso_call_ext()')
 */
kso _ks_exec(ks_code bc, ks_type _in) {
    /* Thread we are executing on */
    ksos_thread th = ksos_thread_get();
    assert(th && th->frames->len > 0);
    assert(ksos_thread_get() == ksg_main_thread);

    /* Frame being executed on */
    ksos_frame frame = (ksos_frame)th->frames->elems[th->frames->len - 1];

    /* Program counter (instruction pointer) */
    #define pc (frame->pc)
    pc = bc->bc->data;

    /* Program stack (value stack) */
    ks_list stk = th->stk;
    int ssl = stk->len;

    /* Number of handlers */
    int snh = th->n_handlers;

    /* Get a value from the value cache/constant array */
    #define VC(_idx) (bc->vc->elems[_idx])

    /* Temporaries */
    ks_str name;
    kso L, R, V;
    bool truthy;
    int i, j;
    ksos_frame fit;


    /* Argument, if the instruction gave one */
    int arg;

    /* Return result */
    kso res = NULL;

    /* Arguments for functions (they need local storage in case something modifies the main stack) */
    int n_args = 0;
    kso* args = NULL;

    /* Allocate and ensure arguments */
    #define ENSURE_ARGS(_n_args) do { \
        n_args = (_n_args); \
        args = ks_zrealloc(args, sizeof(*args), n_args); \
    } while (0)

    /* Pop off the last '_n' arguments from the stack */
    #define ARGS_FROM_STK(_num) do { \
        int _n = (_num); \
        assert(stk->len >= _n); \
        ENSURE_ARGS(_n); \
        int _i; \
        stk->len -= _n; /* Shift off, and copy arguments from end (absorbing references) */\
        for (_i = 0; _i < _n; ++_i) { \
            args[_i] = stk->elems[stk->len + _i]; \
        } \
    } while (0)


    /* 'DECREF' arguments */
    #define DECREF_ARGS(_num) do { \
        int _i, _n = (_num); \
        for (_i = 0; _i < _n; ++_i) { \
            KS_DECREF(args[_i]); \
        } \
    } while (0)

    /* Store a local value */
    #define STORE(_name, _obj) do { \
        if (_in != NULL) { \
            if (!ks_type_set(_in, _name, (kso)_obj)) { \
                goto thrown; \
            } \
        } else { \
            if (!ks_dict_set_h(frame->locals, (kso)_name, _name->v_hash, (kso)_obj)) { \
                goto thrown; \
            } \
        } \
    } while (0)

    /* Dispatch */
    disp:;
    VMD_START {
        VMD_OP(KSB_NOOP)
        VMD_OP_END

        VMD_OPA(KSB_PUSH)
            ks_list_push(stk, VC(arg));
        VMD_OP_END
        
        VMD_OP(KSB_POPU)
            KS_DECREF(stk->elems[--stk->len]);
        VMD_OP_END
        
        VMD_OP(KSB_DUP)
            ks_list_push(stk, stk->elems[stk->len - 1]);
        VMD_OP_END

        VMD_OPA(KSB_DUPI)
            assert(arg < 0);
            ks_list_push(stk, stk->elems[stk->len + arg]);
        VMD_OP_END

        VMD_OPA(KSB_LOAD)
            name = (ks_str)VC(arg);
            assert(name->type == kst_str);

            /* Check frame (and closures) */
            fit = frame;
            do {
                if (fit->locals) {
                    V = ks_dict_get_ih(fit->locals, (kso)name, name->v_hash);
                    if (V) {
                        /* Found in this scope, so push it and execute the next */
                        ks_list_pushu(stk, V);
                        VMD_NEXT();
                    }
                }
 
                fit = fit->closure;
            } while (fit != NULL);

            /* Now, check globals */
            V = ks_dict_get_ih(ksg_globals, (kso)name, name->v_hash);

            if (!V) {
                KS_THROW(kst_NameError, "Unknown name: %R", name);
                goto thrown;
            }
            ks_list_pushu(stk, V);
        VMD_OP_END
        
        VMD_OPA(KSB_STORE)
            name = (ks_str)VC(arg);
            assert(name->type == kst_str);
            V = stk->elems[stk->len - 1];
            STORE(name, V);
        VMD_OP_END

        VMD_OPA(KSB_GETATTR)
            V = stk->elems[--stk->len];
            name = (ks_str)VC(arg);
            R = kso_getattr(V, name);
            KS_DECREF(V);
            if (!R) goto thrown;
            ks_list_pushu(stk, R);
        VMD_OP_END

        VMD_OPA(KSB_SETATTR)
            L = stk->elems[stk->len - 1];
            R = stk->elems[stk->len - 2];
            if (!kso_setattr(L, (ks_str)VC(arg), R)) {
                goto thrown;
            }
            ks_list_popu(stk);
        VMD_OP_END

        VMD_OPA(KSB_GETELEMS)
            ARGS_FROM_STK(arg);
            V = kso_getelems(arg, args);
            DECREF_ARGS(arg);
            if (!V) goto thrown;

            ks_list_pushu(stk, V);
        VMD_OP_END

        VMD_OPA(KSB_SETELEMS)
            ARGS_FROM_STK(arg);
            if (!kso_setelems(arg, args)) {
                DECREF_ARGS(arg);
                goto thrown;
            }

            ks_list_push(stk, args[arg - 1]);
            DECREF_ARGS(arg);
        VMD_OP_END

        VMD_OPA(KSB_CALL)
            assert(arg >= 1);
            ARGS_FROM_STK(arg);
            V = kso_call(args[0], n_args - 1, args + 1);
            DECREF_ARGS(arg);
            if (!V) goto thrown;
            ks_list_pushu(stk, V);

        VMD_OP_END

        /** Constructors **/

        VMD_OPA(KSB_LIST)
            stk->len -= arg;
            ks_list_pushu(stk, (kso)ks_list_newn(arg, stk->elems + stk->len));
        VMD_OP_END

        VMD_OPA(KSB_TUPLE)
            stk->len -= arg;
            ks_list_pushu(stk, (kso)ks_tuple_newn(arg, stk->elems + stk->len));
        VMD_OP_END

        VMD_OPA(KSB_FUNC)
            /* (name, sig, (*pars), doc, va_idx) */
            ks_tuple finfo = (ks_tuple)VC(arg);
            assert(finfo->type == kst_tuple && finfo->len == 5);

            ks_cint va_idx;
            if (!kso_get_ci(finfo->elems[4], &va_idx)) {
                assert(false);
            }

            kso fbc = ks_list_pop(stk);
            assert(fbc && fbc->type == kst_code);
            ks_func fnew = ks_func_new_k(fbc, (ks_tuple)finfo->elems[2], 0, NULL, va_idx, (ks_str)finfo->elems[1], (ks_str)finfo->elems[3]);
            KS_INCREF((kso)frame);
            fnew->bfunc.closure = (kso)frame;
            KS_DECREF(fbc);

            ks_list_pushu(stk, (kso)fnew);

        VMD_OP_END

        VMD_OPA(KSB_FUNC_DEFA)
            ARGS_FROM_STK(arg);
            ks_func f = (ks_func)stk->elems[stk->len - 1];
            assert(f->type == kst_func && !f->is_cfunc);
            ks_func_setdefa(f, n_args, args);
            DECREF_ARGS(arg);
        VMD_OP_END

        VMD_OPA(KSB_TYPE)
            /* (name, doc) */
            ks_tuple tinfo = (ks_tuple)VC(arg);
            assert(tinfo->type == kst_tuple && tinfo->len == 2);

            ks_type tbase = (ks_type)ks_list_pop(stk);
            assert(tbase && kso_issub(tbase->type, kst_type));
            kso tbc = ks_list_pop(stk);
            assert(tbc && tbc->type == kst_code);

            int tsz = tbase->ob_sz, tattr = tbase->ob_attr;

            if (tattr < 0) {
                tattr = tsz;
                tsz += sizeof(ks_dict);
            }
            ks_type tnew = ks_type_new(((ks_str)tinfo->elems[0])->data, tbase, tsz, tattr, ((ks_str)tinfo->elems[1])->data, NULL);
            KS_DECREF(tbase);

            /* Execute the body */
            V = kso_call_ext(tbc, 1, (kso[]){ (kso)tnew }, tnew->attr, frame);
            KS_DECREF(tbc);
            if (!V) {
                KS_DECREF(tnew);
                KS_DECREF(tbc);
                goto thrown;
            }

            ks_list_pushu(stk, (kso)tnew);

        VMD_OP_END


        /** Control Flow **/

        VMD_OPA(KSB_JMP)
            pc += arg;
        VMD_OP_END

        VMD_OPA(KSB_JMPT)
            V = stk->elems[--stk->len];
            if (!kso_truthy(V, &truthy)) {
                KS_DECREF(V);
                goto thrown;
            }
            KS_DECREF(V);
            if (truthy) {
                pc += arg;
            }
        VMD_OP_END

        VMD_OPA(KSB_JMPF)
            V = stk->elems[--stk->len];
            if (!kso_truthy(V, &truthy)) {
                KS_DECREF(V);
                goto thrown;
            }
            KS_DECREF(V);
            if (!truthy) {
                pc += arg;
            }
        VMD_OP_END

        VMD_OP(KSB_RET)
            res = ks_list_pop(stk);
            goto done;
        VMD_OP_END

        VMD_OP(KSB_THROW)
            res = ks_list_pop(stk);
            kso_throw((ks_Exception)res);
            goto thrown;
        VMD_OP_END

        VMD_OPA(KSB_ASSERT)
            res = ks_list_pop(stk);
            if (!kso_truthy(res, &truthy)) {
                KS_DECREF(res);
                goto thrown;
            }

            KS_DECREF(res);
            if (!truthy) {
                KS_THROW(kst_AssertError, "Assertion failed: '%S'", VC(arg));
                goto thrown;
            }
        VMD_OP_END


        VMD_OP(KSB_FINALLY_END)
            if (th->exc) goto thrown;
        VMD_OP_END


        VMD_OP(KSB_FOR_START)
            res = ks_list_pop(stk);
            V = kso_iter(res);
            KS_DECREF(res);
            if (!V) goto thrown;
            ks_list_pushu(stk, V);
        VMD_OP_END

        VMD_OPA(KSB_FOR_NEXTT)
            V = kso_next(stk->elems[stk->len - 1]);
            if (!V) {
                if (th->exc->type == kst_OutOfIterException) {
                    kso_catch_ignore();
                    ks_list_popu(stk);
                } else {
                    goto thrown;
                }
            } else {
                pc += arg;
                ks_list_pushu(stk, V);
            }
        VMD_OP_END

        VMD_OPA(KSB_FOR_NEXTF)
            V = kso_next(stk->elems[stk->len - 1]);
            if (!V) {
                if (th->exc->type == kst_OutOfIterException) {
                    kso_catch_ignore();
                    ks_list_popu(stk);
                    pc += arg;
                } else {
                    goto thrown;
                }
            } else {
                ks_list_pushu(stk, V);
            }
        VMD_OP_END

        VMD_OPA(KSB_TRY_START)
            i = th->n_handlers++;
            th->handlers = ks_zrealloc(th->handlers, sizeof(*th->handlers), th->n_handlers);
            th->handlers[i].topc = pc + arg;
            th->handlers[i].stklen = stk->len;
        VMD_OP_END

        VMD_OPA(KSB_TRY_CATCH)
            assert(th->exc);
            assert(stk->len >= 1);
            V = ks_list_pop(stk);
            if (!is_typeinfo(th->exc->type, V, &truthy)) {
                KS_DECREF(V);
                goto thrown;
            }

            KS_DECREF(V);
            if (truthy) {
                ks_list_pushu(stk, (kso)kso_catch());
            } else {
                pc += arg;
            }

        VMD_OP_END

        VMD_OPA(KSB_TRY_CATCH_ALL)
            ks_list_pushu(stk, (kso)kso_catch());
            pc += arg;
        VMD_OP_END

        VMD_OPA(KSB_TRY_END)
            th->n_handlers--;
            pc += arg;
        VMD_OP_END


        VMD_OPA(KSB_IMPORT)
            name = (ks_str)VC(arg);
            assert(name->type == kst_str);

            V = (kso)ks_import(name);
            if (!V) goto thrown;
            ks_list_pushu(stk, V);

        VMD_OP_END

        VMD_OP(KSB_BOP_EEQ)
            R = ks_list_pop(stk);
            L = ks_list_pop(stk);
            truthy = L == R;
            KS_DECREF(L);
            KS_DECREF(R);
            ks_list_push(stk, KSO_BOOL(truthy));
        VMD_OP_END

        VMD_OP(KSB_BOP_EQ)
            R = ks_list_pop(stk);
            L = ks_list_pop(stk);
            if (!kso_eq(L, R, &truthy)) {
                KS_DECREF(L);
                KS_DECREF(R);
                goto thrown;
            }

            KS_DECREF(L);
            KS_DECREF(R);
            ks_list_push(stk, KSO_BOOL(truthy));
        VMD_OP_END

        VMD_OP(KSB_BOP_NE)
            R = ks_list_pop(stk);
            L = ks_list_pop(stk);
            if (!kso_eq(L, R, &truthy)) {
                KS_DECREF(L);
                KS_DECREF(R);
                goto thrown;
            }

            KS_DECREF(L);
            KS_DECREF(R);
            ks_list_push(stk , KSO_BOOL(!truthy));
        VMD_OP_END

        /* Template for binary operators */
        #define T_BOP(_b, _name) VMD_OP(_b) \
            R = ks_list_pop(stk); \
            L = ks_list_pop(stk); \
            V = ks_bop_##_name(L, R); \
            KS_DECREF(L); KS_DECREF(R); \
            if (!V) goto thrown; \
            ks_list_pushu(stk, V); \
        VMD_OP_END
        
        /* Binary operators */
        T_BOP(KSB_BOP_ADD, add)
        T_BOP(KSB_BOP_SUB, sub)
        T_BOP(KSB_BOP_MUL, mul)
        T_BOP(KSB_BOP_DIV, div)
        T_BOP(KSB_BOP_FLOORDIV, floordiv)
        T_BOP(KSB_BOP_MOD, mod)
        T_BOP(KSB_BOP_POW, pow)
        T_BOP(KSB_BOP_IOR, binior)
        T_BOP(KSB_BOP_AND, binand)
        T_BOP(KSB_BOP_XOR, binxor)
        T_BOP(KSB_BOP_LSH, lsh)
        T_BOP(KSB_BOP_RSH, rsh)
        T_BOP(KSB_BOP_LT, lt)
        T_BOP(KSB_BOP_LE, le)
        T_BOP(KSB_BOP_GT, gt)
        T_BOP(KSB_BOP_GE, ge)

        /* Template for unary operators */
        #define T_UOP(_b, _name) VMD_OP(_b) \
            L = ks_list_pop(stk); \
            V = ks_uop_##_name(L); \
            KS_DECREF(L); \
            if (!V) goto thrown; \
            ks_list_pushu(stk, V); \
        VMD_OP_END

        T_UOP(KSB_UOP_POS, pos)
        T_UOP(KSB_UOP_NEG, neg)
        T_UOP(KSB_UOP_SQIG, sqig)


        VMD_OP(KSB_UOP_NOT)
            L = ks_list_pop(stk);
            if (!kso_truthy(L, &truthy)) {
                KS_DECREF(L);
                goto thrown;
            }
            KS_DECREF(L);
            ks_list_push(stk, KSO_BOOL(!truthy));
        VMD_OP_END


        VMD_OP(KSB_BOP_IN)
            R = ks_list_pop(stk);
            L = ks_list_pop(stk);
            V = ks_contains(R, L);
            if (!V) {
                KS_DECREF(L);
                KS_DECREF(R);
                goto thrown;
            }

            KS_DECREF(L);
            KS_DECREF(R);
            ks_list_pushu(stk, V);
        VMD_OP_END

        /* Error on unknown */
        VMD_CATCH_REST
    }


    thrown:;
    /* Exception was thrown */
    if (th->n_handlers > snh) {
        /* Execute handler */
        while (stk->len > th->handlers[th->n_handlers - 1].stklen) {
            ks_list_popu(stk);
        }
        pc = th->handlers[--th->n_handlers].topc;
        VMD_NEXT();
    }

    /* Ensure we return NULL */
    res = NULL;

    done:;
    /* Clean up and return */

    /* Remove extre values */
    while (stk->len > ssl) {
        KS_DECREF(stk->elems[--stk->len]);
    }

    /* Free temporary arguments */
    ks_free(args);


    return res;

    #undef stk
}



