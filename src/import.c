/* import.c - Module importing in kscript
 *
 * @author: Cade Brown <cade@kscript.org>
 */
#include <ks/impl.h>
#include <ks/compiler.h>



/* Internals */

/* Cache of base modules already imported */
static ks_dict base_cache = NULL;


/* Internal utility to load a module from a path as a C-style DLL */
static ks_module import_path_dll(ks_str p, ks_str name, kso dir) {

    bool g;
    ks_trace("ks", "Attempting %R for %R", p, name);
    if (ksos_path_isfile((kso)p, &g) && g) {
        
        /* Now, check if we can open it, and get the symbol that holds the initializer */
		struct ks_cextinit* sym = NULL;
#ifdef WIN32
		HMODULE handle = LoadLibrary(p->data);
		if (!handle) {
			KS_THROW(kst_Error, "Failed to LoadLibrary %R: [%i]", name, GetLastError());
			return NULL;
		}

		sym = GetProcAddress(handle, _KS_CEXTINIT_SYMBOL_STR);
		if (sym) {
			ks_module res = sym->loadfunc();
			if (!res) {
				FreeLibrary(handle);
				return NULL;
			}
			res->dlhandle = handle;
            ks_dict_merge_ikv(res->attr, KS_IKV(
                {"__src", KS_NEWREF(p)},
            ));
			return res;
		} else {
			FreeLibrary(handle);
			return NULL;
		}
#else
        void* handle = dlopen(p->data, RTLD_LAZY);
        if (!handle) {
            KS_THROW(kst_Error, "Failed to dlopen %R: %s", p, dlerror());
            return NULL;
        }

        sym = dlsym(handle, _KS_CEXTINIT_SYMBOL_STR);
        if (sym) {
            ks_module res = sym->loadfunc();
			if (!res) {
				dlclose(handle);
				return NULL;
			}
            res->dlhandle = handle;
            ks_dict_merge_ikv(res->attr, KS_IKV(
                {"__src", KS_NEWREF(p)},
            ));
            return res;
        } else {
            KS_THROW(kst_ImportError, "Failed to import %R, %R had not symbol '%s'", name, p, _KS_CEXTINIT_SYMBOL_STR);
            dlclose(handle);
            return NULL; 
        }
#endif

    } else {
        kso_catch_ignore();
    }

    return NULL;
}

/* Internal utility to try importing a module from a path */
static ks_module import_path(ks_str p, ks_str name, kso dir) {
    bool g;
    if (ksos_path_isfile((kso)p, &g) && g) {
        ks_trace("ks", "Attempting %R for %R", p, name);
        /* Is a valid file, so read it all */
        ks_str src = ksio_readall(p);
        if (!src) {
            kso_catch_ignore();
            return NULL;
        }

        ks_tok* toks = NULL;
        ks_ssize_t n_toks = ks_lex(p, src, &toks);
        if (n_toks < 0) {
            ks_free(toks);
            return NULL;
        }

        /* Parse the tokens into an AST */
        ks_ast prog = ks_parse_prog(p, src, n_toks, toks);
        ks_free(toks);
        if (!prog) {
            return NULL;
        }
        /* Compile the AST into a bytecode object which can be executed */
        ks_code code = ks_compile(p, src, prog, NULL);
        KS_DECREF(prog);
        if (!code) {
            return NULL;
        }
        ksos_path rp = ksos_path_real((kso)p);
        ks_str rps = NULL;
        if (!rp) {
            kso_catch_ignore();
            KS_INCREF(p);
            rps = p;
        } else {
            rps = ks_fmt("%S", rp);
            KS_DECREF(rp);
        }

        ks_module mod = ks_module_new(name->data, rps->data, "", KS_IKV(
            {"__dir",              KS_NEWREF(dir)},
            {"__file",             KS_NEWREF(p)},
            {"__src",              KS_NEWREF(p)},
        ));
        KS_DECREF(rps);

        /* Execute the program, which should return the value */
        kso res = kso_call_ext((kso)code, 0, NULL, mod->attr, NULL);
        KS_DECREF(code);
        if (!res) {
            KS_DECREF(mod);
            return NULL;
        }

        KS_DECREF(res);
        return mod;
    } else {
        kso_catch_ignore();
    }

    return NULL;
}


/* C-API */

ks_module ks_import(ks_str name) {
    ks_module res = (ks_module)ks_dict_get_ih(base_cache, (kso)name, name->v_hash);
    if (res) return res;

    /* Builtin module */
    #define BIMOD(_str) else if (ks_str_eq_c(name, #_str, sizeof(#_str) - 1)) { \
        res = _ksi_##_str(); \
        if (!res) return NULL; \
    }

    if (false) {}

    BIMOD(io)
    BIMOD(os)
    BIMOD(m)
    BIMOD(getarg)
    BIMOD(time)
    BIMOD(ffi)
    BIMOD(ucd)
    BIMOD(net)
    BIMOD(nx)

    /* Capture thread for exception */
    ksos_thread th = ksos_thread_get();

    /* Search through the paths while it has not been found */
    int i;
    for (i = 0; !res && i < ksg_path->len; ++i) {

        /* Directory in which it was located */
        ks_str dir = NULL;

        /* File being searched */
        ks_str fl = NULL;
        
        /** kscript files **/

        /* Search 'path[i]/' */
        dir = ks_fmt("%S", ksg_path->elems[i]);

        /* path[i]/NAME.ks */
        fl = ks_fmt("%S/%S.ks", dir, name);
        res = import_path(fl, name, (kso)dir);
        KS_DECREF(fl);
        if (res) {
            break;
        } else if (th->exc) {
            KS_DECREF(dir);
            return NULL;
        }

        /* path[i]/ksm_NAME.so  (or different extension for other platforms) */
        fl = ks_fmt("%S/ksm_%S%s", dir, name, KS_PLATFORM_EXTSHARED);
        res = import_path_dll(fl, name, (kso)dir);
        KS_DECREF(fl);
        if (res) {
            break;
        } else if (th->exc) {
            KS_DECREF(dir);
            return NULL;
        }


        KS_DECREF(dir);
        /* Search 'path[i]/NAME' */
        dir = ks_fmt("%S/%S", ksg_path->elems[i], name);

        /* path[i]/NAME/__main.ks */
        fl = ks_fmt("%S/__main.ks", dir, name);
        res = import_path(fl, name, (kso)dir);
        KS_DECREF(fl);
        if (res) {
            break;
        } else if (th->exc) {
            KS_DECREF(dir);
            return NULL;
        }

        /* path[i]/NAME/ksm_NAME.so  (or different extension for other platforms) */
        fl = ks_fmt("%S/ksm_%S%s", dir, name, KS_PLATFORM_EXTSHARED);
        res = import_path_dll(fl, name, (kso)dir);
        KS_DECREF(fl);
        if (res) {
            break;
        } else if (th->exc) {
            KS_DECREF(dir);
            return NULL;
        }

        KS_DECREF(dir);
    }

    if (!res) {
        /* Not found */
        KS_THROW(kst_ImportError, "Failed to import %R", name);
        return NULL;
    }

    /* Found module, so set in the cache and return */
    ks_dict_set_h(base_cache, (kso)name, name->v_hash, (kso)res);
    return res;
}

ks_module ks_import_sub(ks_module of, ks_str sub) {
    ks_module res = (ks_module)ks_dict_get_h(of->attr, (kso)sub, sub->v_hash);
    if (res) return res;

    ks_str k = ks_str_new(-1, "__dir");
    ks_str dir = (ks_str)ks_dict_get(of->attr, (kso)k);
    KS_DECREF(k);
    if (!dir) return NULL;

    k = ks_str_new(-1, "__name");    
    ks_str name = (ks_str)ks_dict_get(of->attr, (kso)k);
    KS_DECREF(k);
    if (!name) return NULL;

    ks_str subname = ks_fmt("%S.%S", name, sub);

    ks_str p = NULL, d = NULL;

    if (!res) {
        p = ks_fmt("%S/%S.ks", dir, sub);
        res = import_path(p, subname, (kso)dir);
        KS_DECREF(p);
    }

    if (!res) {
        p = ks_fmt("%S/%S/__main.ks", dir, sub);
        d = ks_fmt("%S/%S", dir, sub);
        res = import_path(p, subname, (kso)d);
        KS_DECREF(p);
        KS_DECREF(d);
    }

    KS_DECREF(subname);
    if (res) {
        ks_dict_set(of->attr, (kso)sub, (kso)res);
        return res;
    } else {
        KS_THROW(kst_ImportError, "Failed to import %R from %R", sub, name);
        return NULL;        
    }
}


void _ksi_import() {
    base_cache = ks_dict_new(NULL);

}
