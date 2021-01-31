/* funcs.c - kscript standard builtin functions
 *
 * @author: Cade Brown <cade@kscript.org>
 */
#include <ks/impl.h>

#if defined(KS_HAVE_readline)

/* Use GNU readline */
#include <readline/readline.h>
#include <readline/history.h>

#endif



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
    
    ksf_exit,

    ksf_print,
    ksf_printf,

    ksf_hash,
    ksf_abs,
    ksf_len,
    ksf_repr,
    ksf_id,

    ksf_ord,
    ksf_chr,

    ksf_input,

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
    ksio_BaseIO out = (ksio_BaseIO)ksos_stdout;

    ks_str toprint = ks_fmt("%J\n", " ", n_args, args);
    if (!toprint) return NULL;
    ksio_addbuf(out, toprint->len_b, toprint->data);
    KS_DECREF(toprint);
    
    fflush(stdout);
    /*
    int i;
    for (i = 0; i < n_args; ++i) {
        if (i > 0) ksio_addbuf(out, 1, " ");
        if (!ksio_add(out, "%S", args[i])) return NULL;
    }
    ksio_addbuf(out, 1, "\n");
    */
    return KSO_NONE;
}
static KS_FUNC(printf) {
    ks_str fmt;
    int n_args;
    kso* args;
    KS_ARGS("fmt:* *args", &fmt, kst_str, &n_args, &args);

    /* Where to output to */
    ksio_StringIO sio = ksio_StringIO_new();

    if (!ks_fmt2((ksio_BaseIO)sio, fmt->data, n_args, args)) {
        KS_DECREF(sio);
        return NULL;
    }

    ksio_BaseIO out = (ksio_BaseIO)ksos_stdout;
    ksio_addbuf(out, sio->len_b, sio->data);
    KS_DECREF(sio);

    return KSO_NONE;
}

static KS_FUNC(hash) {
    kso obj;
    KS_ARGS("obj", &obj);
    ks_hash_t res;
    if (!kso_hash(obj, &res)) return NULL;

    return (kso)ks_int_newu(res);
}

static KS_FUNC(abs) {
    kso obj;
    KS_ARGS("obj", &obj);

    if (obj->type->i__abs) {
        return kso_call(obj->type->i__abs, 1, &obj);
    }

    KS_THROW_METH(obj, "__abs");
    return NULL;
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


static KS_FUNC(exit) {
    ks_cint code = 0;
    KS_ARGS("?code:cint", &code);

    exit(code);

    return KSO_NONE;
}



static KS_FUNC(eval) {
    ks_str src, fname = _ksv_expr;
    kso locals = NULL;
    KS_ARGS("src:* ?fname:* ?locals", &src, kst_str, &fname, kst_str, &locals);

    if (locals == KSO_NONE) {
        locals = NULL;
    } else if (locals && !kso_issub(locals->type, kst_dict)) {
        KS_THROW(kst_TypeError, "Expected 'locals' to be a 'dict', but got '%T' object", locals);
        return NULL;
    }

    return kso_eval(src, fname, (ks_dict)locals);
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

static KS_FUNC(issub) {
    ks_type tp;
    kso of;
    KS_ARGS("tp:* of", &tp, kst_type, &of);

    if (kso_issub(of->type, kst_type)) {
        return KSO_BOOL(kso_issub(tp, (ks_type)of));
    } else if (kso_issub(of->type, kst_tuple)) {
        ks_tuple tup = (ks_tuple)of;
        ks_cint i;
        for (i = 0; i < tup->len; ++i) {
            ks_type ofi = (ks_type)tup->elems[i];
            if (!kso_issub(ofi->type, kst_type)) {
                KS_THROW(kst_TypeError, "Expected 'of' to be either a type or tuple of types, but got '%T' object (which didn't have types in it)", of);
                return NULL;
            }
            if (kso_issub(tp, ofi)) return KSO_TRUE;
        }
        return KSO_FALSE;
    }

    KS_THROW(kst_TypeError, "Expected 'of' to be either a type or tuple of types, but got '%T' object", of);
    return NULL;

}

static KS_FUNC(isinst) {
    kso obj;
    kso of;
    KS_ARGS("obj of", &obj, &of);

    return issub_(2, (kso[]){ (kso)obj->type, of });
}
static KS_FUNC(id) {
    kso obj;
    KS_ARGS("obj", &obj);

    return (kso)ks_int_newu((ks_uint)obj);
}

static KS_FUNC(input) {
    ks_str prompt = NULL;
    KS_ARGS("?prompt:*", &prompt, kst_str);


#ifdef KS_HAVE_readline

    char* line = readline(prompt ? prompt->data : "");
    if (!line) {
        KS_THROW(kst_Error, "readline() returned NULL");
        return NULL;
    }
    ks_str res = ks_str_new(-1, line);
    free(line);

    return (kso)res;

#else

    if (prompt) {
        if (!ks_printf("%S", prompt)) return NULL;
    }

    kso res = kso_next((kso)ksos_stdin);
    if (!res) {
        return NULL;
    }

    return res;
#endif
}

void _ksi_funcs() {

    /* Create a function */
    #define F(_name, _sig, _doc) ksf_##_name = (ks_func)ksf_wrap(_name##_, _sig, _doc);

    F(open, "open(src, mode='r')", "Opens a file on disk, and returns an IO object which can be read from (or written to, based on 'mode')");

    F(print, "print(*args)", "Prints out all the arguments to 'os.stdout', seperated by spaces, and followed by a newline")
    F(printf, "printf(fmt, *args)", "Prints a formatted message to 'os.stdout', with no seperators or newline")

    F(hash, "hash(obj)", "Computes the hash of an object, which is an integer\n\n    Delegates to 'type(obj).__hash(obj)'");
    F(abs, "abs(obj)", "Computes absolute value of an object\n\n    Delegates to 'type(obj).__abs(obj)'");
    F(len, "len(obj)", "Computes the length of an object, which is normally the number of elements in a collection\n\n    Delegates to 'type(obj).__hash(obj)'");
    F(repr, "repr(obj)", "Computes the string representation of an object, which aims to be a string that can either be executed and result in the same value, or give as much information as possible\n\n    Delegates to 'type(obj).__repr(obj)'");

    F(exit, "exit(code=0)", "Exit the program with a given exit code")
    F(eval, "eval(src, fname='<expr>', locals=none)", "Evaluate kscript source code, and return the result")

    F(pow, "pow(L, R, M=none)", "Computes exponentiation (i.e. 'L ** R'), or modular exponentiation ('L ** R % m'), if 'M' is given")

    F(issub, "issub(tp, of)", "Computes whether 'tp' was a subtype (or the same type) as 'of', which is either a type, or tuple of types")
    F(isinst, "isinst(obj, of)", "Equivalent to 'issub(type(obj), of)'")

    F(chr, "chr(ord)", "Convert an ordinal (i.e. an integral codepoint) into a length-1 string");
    F(ord, "ord(chr)", "Convert a length-1 string into an ordinal (i.e. an integral codepoint)");

    F(iter, "iter(obj)", "Returns an iterator over the contents of 'obj'\n\n    Delegates to 'type(obj).__iter'")
    F(next, "next(obj)", "Returns the next object in an iterator\n\n    Delegates to 'type(obj).__next', or 'next(iter(obj))'")
    F(input, "input(prompt='')", "Print an (optional) prompt and then return the next line of user input")

    F(id, "id(obj)", "Return the id of an object, which is its memory location");

    #undef F

}


