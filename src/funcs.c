/* funcs.c - kscript standard builtin functions
 *
 * @author: Cade Brown <cade@kscript.org>
 */
#include <ks/impl.h>

ks_func 
    ksf_any,
    ksf_all,
    ksf_min,
    ksf_max,
    ksf_sum,
    ksf_pow,

    ksf_open,
    ksf_close,

    ksf_eval,
    ksf_exec,

    ksf_print,

    ksf_hash,
    ksf_abs,
    ksf_len,
    ksf_repr,
    ksf_id,

    ksf_ord,
    ksf_chr,

    ksf_issub,
    ksf_isinst,
    
    ksf_iter,
    ksf_next
;

static KS_FUNC(open) {
    int n_args;
    kso* args;
    KS_ARGS("*args", &n_args, &args);

    return kso_call((kso)ksiot_FileIO, n_args, args);
}
static KS_FUNC(print) {
    int n_args;
    kso* args;
    KS_ARGS("*args", &n_args, &args);

    /* Where to output to */
    ksio_AnyIO out = (ksio_AnyIO)ksos_stdout;

    int i;
    for (i = 0; i < n_args; ++i) {
        if (i > 0) ksio_addbuf(out, 1, " ");
        if (!ksio_add(out, "%S", args[i])) return NULL;
    }
    ksio_addbuf(out, 1, "\n");

    return KSO_NONE;
}


static KS_FUNC(hash) {
    kso obj;
    KS_ARGS("obj", &obj);
    ks_hash_t res;
    if (!kso_hash(obj, &res)) return NULL;

    return (kso)ks_int_newu(res);
}


static KS_FUNC(len) {
    kso obj;
    KS_ARGS("obj", &obj);
    
    if (obj->type->i__len) {
        return kso_call(obj->type->i__len, 1, &obj);
    }

    KS_THROW_METH(obj, "__len");
    return NULL;
}


static KS_FUNC(repr) {
    kso obj;
    KS_ARGS("obj", &obj);
    return (kso)ks_fmt("%R", obj);
}

static KS_FUNC(pow) {
    kso L, R, M = KSO_NONE;
    KS_ARGS("L R ?M", &L, &R, &M);

    if (M == KSO_NONE) {
        return ks_bop_pow(L, R);
    } else {
        ks_int Li, Ri, Mi;
        if (!(Li = kso_int(L))) return NULL;
        if (!(Ri = kso_int(R))) {
            KS_DECREF(Li);            
            return NULL;
        }
        if (!(Mi = kso_int(M))) {
            KS_DECREF(Li);            
            KS_DECREF(Ri);
            return NULL;
        }

        mpz_t res;
        mpz_init(res);
        mpz_powm(res, Li->val, Ri->val, Mi->val);
        KS_DECREF(Li);
        KS_DECREF(Ri);
        KS_DECREF(Mi);

        return (kso)ks_int_newzn(res);
    }
}


static KS_FUNC(chr) {
    ks_cint ord;
    KS_ARGS("ord:cint", &ord);
    return (kso)ks_str_chr(ord);
}

static KS_FUNC(ord) {
    ks_str chr;
    KS_ARGS("chr:*", &chr, kst_str);
    ks_ucp res = ks_str_ord(chr);
    if (res < 0) return NULL;
    return (kso)ks_int_new(res);
}



static KS_FUNC(iter) {
    kso obj;
    KS_ARGS("obj", &obj);

    return kso_iter(obj);
}

static KS_FUNC(next) {
    kso obj;
    KS_ARGS("obj", &obj);

    return kso_next(obj);
}



void _ksi_funcs() {

    /* Create a function */
    #define F(_name, _sig, _doc) ksf_##_name = (ks_func)ksf_wrap(_name##_, _sig, _doc);

    F(open, "open(src, mode='r')", "Opens a file on disk, and returns an IO object which can be read from (or written to, based on 'mode')");

    F(print, "print(*args)", "Prints out all the arguments to 'os.stdout', seperated by spaces, and followed by a newline")

    F(hash, "hash(obj)", "Computes the hash of an object, which is an integer\n\n    Delegates to 'type(obj).__hash(obj)'");
    F(len, "len(obj)", "Computes the length of an object, which is normally the number of elements in a collection\n\n    Delegates to 'type(obj).__hash(obj)'");
    F(repr, "repr(obj)", "Computes the string representation of an object, which aims to be a string that can either be executed and result in the same value, or give as much information as possible\n\n    Delegates to 'type(obj).__repr(obj)'");

    F(pow, "pow(L, R, M=none)", "Computes exponentiation (i.e. 'L ** R'), or modular exponentiation ('L ** R % m'), if 'M' is given")

    F(chr, "chr(ord)", "Convert an ordinal (i.e. an integral codepoint) into a length-1 string");
    F(ord, "ord(chr)", "Convert a length-1 string into an ordinal (i.e. an integral codepoint)");

    F(iter, "iter(obj)", "Returns an iterator over the contents of 'obj'\n\n    Delegates to 'type(obj).__iter'")
    F(next, "next(obj)", "Returns the next object in an iterator\n\n    Delegates to 'type(obj).__next', or 'next(iter(obj))'")


    #undef F

}


