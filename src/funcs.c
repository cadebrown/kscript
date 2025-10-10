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

    ksf_bin,
    ksf_oct,
    ksf_hex,

    ksf_input,

    ksf_issub,
    ksf_isinst,
    
    ksf_iter,
    ksf_next,

    ksf_import,
    ksf_wget,
    ksf_js,
    ksf_man
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


static KS_FUNC(bin) {
    kso obj;
    KS_ARGS("obj", &obj);

    ksio_StringIO sio = ksio_StringIO_new();
    if (!ks_fmt2((ksio_BaseIO)sio, "0b%b", 1, &obj)) {
        KS_DECREF(sio);
        return NULL;
    }

    return (kso)ksio_StringIO_getf(sio);
}
static KS_FUNC(oct) {
    kso obj;
    KS_ARGS("obj", &obj);

    ksio_StringIO sio = ksio_StringIO_new();
    if (!ks_fmt2((ksio_BaseIO)sio, "0o%o", 1, &obj)) {
        KS_DECREF(sio);
        return NULL;
    }

    return (kso)ksio_StringIO_getf(sio);
}
static KS_FUNC(hex) {
    kso obj;
    KS_ARGS("obj", &obj);

    ksio_StringIO sio = ksio_StringIO_new();
    if (!ks_fmt2((ksio_BaseIO)sio, "0x%x", 1, &obj)) {
        KS_DECREF(sio);
        return NULL;
    }

    return (kso)ksio_StringIO_getf(sio);
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

static KS_FUNC(import) {
    ks_str name, path = NULL;
    KS_ARGS("name:* ?path:*", &name, kst_str, &path, kst_str);

    return (kso)ks_importx(name, (kso)path);
}

static KS_FUNC(wget) {
    ks_str url, path = NULL;
    KS_ARGS("url:* ?path:*", &url, kst_str, &path, kst_str);

#ifdef EMSCRIPTEN

    if (path == NULL) {
        /* Use the last segment of the URL as the path */
        path = ks_str_new(-1, strrchr(url->data, '/') + 1);
        if (path->len_b == 0) {
            KS_THROW(kst_Error, "Failed to determine filename from URL %R, please provide an explicit 'path' argument", url);
            KS_DECREF(path);
            return NULL;
        }
    } else {
        KS_INCREF(path);
    }

    ks_printf("Downloading URL %R to path %R\n", url, path);
    if (emscripten_wget(url->data, path->data) != 0) {
        KS_THROW(kst_IOError, "Failed to start download of URL %S to path %S", url, path);
        return NULL;
    }

    // NOTE: incref and decref, since we are returning the path
    // convert to a 'correct' path

    ksos_path path_real = ksos_path_new_o((kso)path);
    KS_DECREF(path);
    return (kso)path_real;

#else
    KS_THROW(kst_Error, "'geturl' is only implemented in the web/emscripten build of kscript");
    return NULL;
#endif
}

static KS_FUNC(js) {
    ks_str code;
    KS_ARGS("code:*", &code, kst_str);
#ifdef EMSCRIPTEN
    emscripten_run_script(code->data);
    return KSO_NONE;
#else
    KS_THROW(kst_Error, "'js' is only implemented in the web/emscripten build of kscript");
    return NULL;
#endif
}

static KS_FUNC(man) {
    kso obj;
    KS_ARGS("obj", &obj);

    ks_str res = ks_fmt("");
    ks_str tmp = NULL;
    if (kso_isinst(obj, kst_type)) {
        ks_type tp = (ks_type)obj;
        tmp = ks_fmt("type %S", tp->i__name);
        KS_DECREF(res);
        res = tmp;

        kso s_doc = kso_getattr_c(obj, "__doc");
        if (s_doc == NULL) {
            kso_catch_ignore();
            res = ks_fmt("%S\n\n    (no documentation)\n", res);
        } else {
            // split into lines
            ks_list lines = ks_str_split_c(((ks_str)s_doc)->data, "\n");
            if (!lines) {
                kso_catch_ignore();
            } else {
                tmp = ks_fmt("%S\n\n    %J\n", res, "\n\n    ", (int)lines->len, lines->elems);
                KS_DECREF(res);
                res = tmp;
                KS_DECREF(lines);
            }
            KS_DECREF(s_doc);
        }

        int i;
        for (i = 0; i < tp->attr->len_ents; ++i) {
            struct ks_dict_ent* ent = &tp->attr->ents[i];
            if (ent->key != NULL) {
                if (kso_isinst(ent->val, kst_func)) {
                    kso sub_man = man_(1, (kso[]){ ent->val });
                    if (!sub_man) {
                        kso_catch_ignore();
                        continue;
                    }
                    tmp = ks_fmt("%S\n%S", res, sub_man);
                    KS_DECREF(res);
                    res = tmp;
                    KS_DECREF(sub_man);
                }
            }
        }

    } else if (kso_isinst(obj, kst_func)) {
        ks_func f = (ks_func)obj;

        kso s_sig = kso_getattr_c(obj, "__sig");
        if (s_sig == NULL) {
            kso_catch_ignore();
            tmp = ks_fmt("%R", f);
            KS_DECREF(res);
            res = tmp;
        } else {
            tmp = ks_fmt("func %S", s_sig);
            KS_DECREF(res);
            res = tmp;
            KS_DECREF(s_sig);
        }
        
        kso s_doc = kso_getattr_c(obj, "__doc");
        if (s_doc == NULL || ((ks_str)s_doc)->len_b == 0) {
            kso_catch_ignore();
            tmp = ks_fmt("%S\n\n    (no documentation)\n", res);
            KS_DECREF(res);
            res = tmp;
            if (s_doc) KS_DECREF(s_doc);

        } else {
            if (kso_isinst(s_doc, kst_str)) {
                // replace all newlines with additional indents
                ks_list lines = ks_str_split_c(((ks_str)s_doc)->data, "\n");
                if (!lines) {
                    kso_catch_ignore();
                } else {
                    tmp = ks_fmt("%S\n\n    %J\n", res, "\n\n    ", (int)lines->len, lines->elems);
                    KS_DECREF(res);
                    res = tmp;
                    KS_DECREF(lines);
                }
            }
            KS_DECREF(s_doc);
        }

    } else {
        KS_DECREF(res);
        KS_THROW(kst_TypeError, "Expected a type, func, or module object to generate a manual page for, but got '%T' object", obj);
        return NULL;
    }

    // print it out, and return it
    ks_printf("%S\n", res);
    return (kso)res;

}

void _ksi_funcs() {

    /* Create a function */
    #define F(_name, _sig, _doc) ksf_##_name = (ks_func)ksf_wrap(_name##_, _sig, _doc);

    F(open, "open(src, mode='r')", "Opens a file on disk, and returns an IO object which can be read from (or written to, based on 'mode')");

    F(print, "print(*args)", "Prints out all the arguments to 'os.stdout', seperated by spaces, and followed by a newline")
    F(printf, "printf(fmt, *args)", "Prints a formatted message to 'os.stdout', with no seperators or newline")

    F(hash, "hash(obj)", "Computes the hash of an object, which is an integer\nDelegates to 'type(obj).__hash(obj)'");
    F(abs, "abs(obj)", "Computes absolute value of an object\nDelegates to 'type(obj).__abs(obj)'");
    F(len, "len(obj)", "Computes the length of an object, which is normally the number of elements in a collection\nDelegates to 'type(obj).__hash(obj)'");
    F(repr, "repr(obj)", "Computes the string representation of an object, which aims to be a string that can either be executed and result in the same value, or give as much information as possible\nDelegates to 'type(obj).__repr(obj)'");

    F(exit, "exit(code=0)", "Exit the program with a given exit code")
    F(eval, "eval(src, fname='<expr>', locals=none)", "Evaluate kscript source code, and return the result")

    F(pow, "pow(L, R, M=none)", "Computes exponentiation (i.e. 'L ** R'), or modular exponentiation ('L ** R % m'), if 'M' is given")

    F(issub, "issub(tp, of)", "Computes whether 'tp' was a subtype (or the same type) as 'of', which is either a type, or tuple of types")
    F(isinst, "isinst(obj, of)", "Equivalent to 'issub(type(obj), of)'")

    F(chr, "chr(ord)", "Convert an ordinal (i.e. an integral codepoint) into a length-1 string");
    F(ord, "ord(chr)", "Convert a length-1 string into an ordinal (i.e. an integral codepoint)");

    F(iter, "iter(obj)", "Returns an iterator over the contents of 'obj'\nDelegates to 'type(obj).__iter'")
    F(next, "next(obj)", "Returns the next object in an iterator\nDelegates to 'type(obj).__next', or 'next(iter(obj))'")
    F(input, "input(prompt='')", "Print an (optional) prompt and then return the next line of user input")

    F(bin, "bin(obj)", "Return a string beginning with '0b' and containing the base-2 digits of 'obj' (which should be an integer)");
    F(oct, "oct(obj)", "Return a string beginning with '0o' and containing the base-8 digits of 'obj' (which should be an integer)");
    F(hex, "hex(obj)", "Return a string beginning with '0x' and containing the base-16 digits of 'obj' (which should be an integer)");

    F(id, "id(obj)", "Return the id of an object, which is its memory location");

    F(import, "__import(name, path=none)", "Import a base module by name (i.e. one without any '.' in the name) and return it");
    F(wget, "wget(path, url)", "Download the contents of 'url' to a local file at 'path'\nOnly available in the web/emscripten build of kscript");
    F(js, "js(code)", "Execute a snippet of JavaScript code\nOnly available in the web/emscripten build of kscript");
    F(man, "man(obj)", "Generate a manual page for an object (typically, a type, func, or module)");

    #undef F

}


