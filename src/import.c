/* import.c - Module importing in kscript
 *
 * @author: Cade Brown <cade@kscript.org>
 */
#include <ks/impl.h>
#include <ks/compiler.h>


static ks_dict cache = NULL;


static ks_module import_place(kso from, ks_str parname, ks_str name) {
    ks_str fn = ks_fmt("%S/%S", from, name);
    if (!fn) {
        return NULL;
    }

    bool g;
    if (ksos_path_isdir((kso)fn, &g)) {
        /* Boom it exists */
        ksos_path rp = ksos_path_real((kso)fn);
        ks_str rps = NULL;
        if (!rp) {
            kso_catch_ignore();
            KS_INCREF(fn);
            rps = fn;
        } else {
            rps = ks_fmt("%S", rp);
            KS_DECREF(rp);
        }

        ks_str fullname = ks_fmt("%S.%S", parname, name);
        ks_module mod = ks_module_new(fullname->data, rps->data, "", KS_IKV(
        ));

        KS_DECREF(fullname);
        KS_DECREF(rps);

        return mod;

    } else {
        kso_catch_ignore();
    }

    KS_DECREF(fn);
    fn = ks_fmt("%S/%S.ks", from, name);

    if (ksos_path_isfile((kso)fn, &g)) {
        ks_str src = ksio_readall(fn);
        if (!src) {
            KS_DECREF(fn);
        }

        ks_tok* toks = NULL;
        ks_ssize_t n_toks = ks_lex(fn, src, &toks);
        if (n_toks < 0) {
            ks_free(toks);
            return NULL;
        }

        /* Parse the tokens into an AST */
        ks_ast prog = ks_parse_prog(fn, src, n_toks, toks);
        ks_free(toks);
        if (!prog) {
            return NULL;
        }
        /* Compile the AST into a bytecode object which can be executed */
        ks_code code = ks_compile(fn, src, prog, NULL);
        KS_DECREF(prog);
        if (!code) {
            return NULL;
        }
        ksos_path rp = ksos_path_real((kso)fn);
        ks_str rps = NULL;
        if (!rp) {
            kso_catch_ignore();
            KS_INCREF(fn);
            rps = fn;
        } else {
            rps = ks_fmt("%S", rp);
            KS_DECREF(rp);
        }
        ks_str fullname = ks_fmt("%S.%S", parname, name);
        ks_module mod = ks_module_new(fullname->data, rps->data, "", KS_IKV(
        ));
        KS_DECREF(fullname);
        KS_DECREF(rps);

        /* Execute the program, which should return the value */
        kso res = kso_call_ext((kso)code, 0, NULL, mod->attr, NULL);
        KS_DECREF(code);
        if (!res) return NULL;

        KS_DECREF(res);

        return mod;
    } else {
        kso_catch_ignore();
    }
    return NULL;
}

/* Import the base module with a given name */
static ks_module import_base(ks_str name) {

    int i;
    for (i = 0; i < ksg_path->len; ++i) {
        /* Attempt a directory */
        ks_str fn = ks_fmt("%S/%S", ksg_path->elems[i], name);
        if (!fn) {
            return NULL;
        }

        bool g;
        if (ksos_path_isdir((kso)fn, &g)) {
            /* Boom it exists */
            ksos_path rp = ksos_path_real((kso)fn);
            ks_str rps = NULL;
            if (!rp) {
                kso_catch_ignore();
                KS_INCREF(fn);
                rps = fn;
            } else {
                rps = ks_fmt("%S", rp);
                KS_DECREF(rp);
            }
            ks_module mod = ks_module_new(name->data, rps->data, "", KS_IKV(
            ));

            KS_DECREF(rps);


            return mod;

        } else {
            kso_catch_ignore();
        }
    }

    /* No base module found */
    KS_THROW(kst_ImportError, "Failed to import %R", name);
    return NULL;
}

ks_module ks_import_sub(ks_module of, ks_str sub) {
    ks_str k = ks_str_new(-1, "__src");    
    kso src = ks_dict_get(of->attr, (kso)k);
    KS_DECREF(k);
    if (!src) return NULL;

    k = ks_str_new(-1, "__name");    
    ks_str name = (ks_str)ks_dict_get(of->attr, (kso)k);
    KS_DECREF(k);
    if (!name) return NULL;

    ks_module res = import_place(src, name, sub);
    KS_DECREF(src);

    if (res) return res;

    if (ksos_thread_get()->exc) return NULL;

    KS_THROW(kst_ImportError, "Failed to import %R from %R", sub, of);
    return NULL;
}

ks_module ks_import(ks_str name) {
    assert(cache != NULL);

    ks_module res = (ks_module)ks_dict_get_ih(cache, (kso)name, name->v_hash);
    if (res) return res;

    #define _BIMOD(_str) else if (ks_str_eq_c(name, #_str, sizeof(#_str) - 1)) { \
        res = _ksi_##_str(); \
    }

    if (false) {}

    _BIMOD(io)
    _BIMOD(os)
    _BIMOD(m)
    _BIMOD(getarg)
    _BIMOD(time)
    _BIMOD(ffi)
    _BIMOD(ucd)

    if (!res) {
        /* No builtin found, so try dynamically searching */
        ks_list parts = ks_str_split_c(name->data, ".");
        assert(parts);
        assert(parts->len > 0);

        /* First, import the parent module (parts[0]) */

        res = import_base((ks_str)parts->elems[0]);
        if (!res) {
            KS_DECREF(parts);
            return NULL;
        }

        /* Now, ensure all the attributes existed */
        KS_INCREF(res);
        kso t = (kso)res;
        int i;
        for (i = 1; i < parts->len; ++i) {
            kso u = kso_getattr(t, (ks_str)parts->elems[i]);
            if (!u) {
                KS_DECREF(res);
                KS_DECREF(t);
                return NULL;
            }
            KS_DECREF(t);
            t = u;
        }
        KS_DECREF(t);

    }

    if (res) {
        ks_dict_set_h(cache, (kso)name, name->v_hash, (kso)res);
        return res;
    } else {
        KS_THROW(kst_ImportError, "Failed to import %R", name);
        return NULL;
    }
}


void _ksi_import() {
    cache = ks_dict_new(NULL);

}
