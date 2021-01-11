/* os/path.c - 'os.path' type
 *
 * 
 * @author: Cade Brown <cade@kscript.org>
 */
#include <ks/impl.h>

#define T_NAME "os.path"
#define TW_NAME T_NAME ".walk"


/* Internals */


/* C-API */

ks_str ksos_path_str(kso path) {
    if (kso_issub(path->type, kst_str)) {
        KS_INCREF(path);
        return (ks_str)path;
    } else if (kso_issub(path->type, ksost_path)) {
        return ks_fmt("%S", path);
    } else {
        KS_THROW(kst_TypeError, "'%T' object cannot be treated as path", path);
        return NULL;
    }
}

ksos_path ksos_path_newt(ks_type tp, ks_ssize_t len_b, const char* data, kso root) {
    if (len_b < 0) len_b = strlen(data);
    ksos_path self = KSO_NEW(ksos_path, tp);

    if (len_b > 3 && ('A' <= data[0] && data[0] <= 'Z' && data[1] == ':' && (data[2] == '\\' || data[2] == '/'))) {
        /* Windows-style root */
        if (root != KSO_NONE) {
            KS_DECREF(self);
            KS_THROW(kst_Error, "Failed to create path that began with Windows-style drive root, but given root=%R", root);
            return NULL;
        }

        self->root = (kso)ks_fmt("%.*s%s", 2, data, "/");
        data += 3;
        len_b -= 3;
    } else if (len_b > 0 && (data[0] == '/')) {
        /* Unix-style absolute root */
        if (root != KSO_NONE) {
            KS_DECREF(self);
            KS_THROW(kst_Error, "Failed to create path that began with Unix-style drive root, but given root=%R", root);
            return NULL;
        }
        self->root = (kso)ks_fmt("%.*s", 1, data);

        data += 1;
        len_b -= 1;
    } else {
        /* Use whatever was given */
        KS_INCREF(root);
        self->root = root;
    }

    if (len_b == 0) {
        /* Empty path */
        self->parts = ks_tuple_new(0, NULL);
    } else {
        /* Otherwise, we need to break it up */
        ks_list pl = ks_list_new(0, NULL);
        ks_ssize_t i, j = 0;
        for (i = 0; i < len_b; ++i) {
            if (data[i] == '/' || data[i] == '\\') {
                /* Found seperator to split on */
                if (i > j) {
                    ks_list_pushu(pl, (kso)ks_str_new(i - j, data + j));
                }

                /* Save the next position */
                j = i + 1;
            }
        }

        if (i > j) {
            ks_list_pushu(pl, (kso)ks_str_new(i - j, data + j));
        }

        /* Canonicalize path */
        self->parts = ks_tuple_new(pl->len, pl->elems);
        KS_DECREF(pl);

    }

    return self;
}
ksos_path ksos_path_new(ks_ssize_t len_b, const char* data, kso root) {
    return ksos_path_newt(ksost_path, len_b, data, root);
}

ksos_path ksos_path_new_ot(ks_type tp, kso ob) {
    if (kso_issub(ob->type, tp)) {
        KS_INCREF(ob);
        return (ksos_path)ob;
    } else if (kso_issub(ob->type, kst_str)) {
        return ksos_path_newt(tp, ((ks_str)ob)->len_b, ((ks_str)ob)->data, KSO_NONE);
    } else if (ob == KSO_NONE) {
        return ksos_path_newt(tp, 0, NULL, KSO_NONE);
    }

    KS_THROW_CONV(ob->type, tp);
    return NULL;
}

ksos_path ksos_path_new_o(kso ob) {
    return ksos_path_new_ot(ksost_path, ob);
}

/** Operations on paths **/

ksos_path ksos_path_join(kso* paths, int len) {
    if (len == 0) {
        return ksos_path_new(-1, "", KSO_NONE);
    } else if (len == 1) {
        return ksos_path_new_o(paths[0]);
    }

    ksos_path p0 = ksos_path_new_o(paths[0]);
    if (!p0) return NULL;

    /* Now, join the parts of all paths present */
    ks_list parts = ks_list_new(0, NULL);
    ks_list_pushall(parts, (kso)p0->parts);

    int i;
    for (i = 1; i < len; ++i) {
        ksos_path p = ksos_path_new_o(paths[i]);
        if (!p) {
            KS_DECREF(p0);
            KS_DECREF(parts);
            return NULL;
        }

        if (p->root != KSO_NONE) {
            KS_DECREF(p0);
            KS_DECREF(p);
            KS_DECREF(parts);
            KS_THROW(kst_Error, "Cannot join absolute path; only the first path in 'os.path.join()' may be absolute");
            return NULL;
        }

        ks_list_pushall(parts, (kso)p->parts);
        KS_DECREF(p);
    }


    /* Now, construct result from joined paths */
    ksos_path res = KSO_NEW(ksos_path, ksost_path);

    res->root = KS_NEWREF(p0->root);
    res->parts = ks_tuple_new(parts->len, parts->elems);
    KS_DECREF(parts);
    KS_DECREF(p0);

    return res;
}

ksos_path ksos_path_parent(kso self) {
    ks_str tmp = ks_str_new(2, "..");
    ksos_path res = ksos_path_join((kso[]){ (kso)self, (kso)tmp }, 2);
    KS_DECREF(tmp);
    return res;
}

kso ksos_path_real(kso path) {
#ifdef KS_HAVE_realpath
    ks_str sp = ksos_path_str(path);
    if (!sp) return NULL;

    char* cr = realpath(sp->data, NULL);
    if (cr) {
        if (kso_issub(path->type, kst_str)) {
            /* Construct string */
            ks_str res = ks_str_new(-1, cr);
            free(cr);
            KS_DECREF(sp);
            return (kso)res;
        } else {
            /* Construct path */
            ksos_path res = ksos_path_new(-1, cr, KSO_NONE);
            free(cr);
            KS_DECREF(sp);
            return (kso)res;
        }
    } else {
        KS_DECREF(sp);
        KS_THROW(kst_OSError, "Failed to determine real path for %R: %s", sp, strerror(errno));
        return NULL;
    }
#else
    KS_THROW(kst_OSError, "Failed to determine real path: platform did not provide a 'realpath()' function", sp);
    return NULL;
#endif
    assert(false);
    return NULL;
}


bool ksos_path_exists(kso path, bool* res) {
    struct ksos_cstat st;
    if (!ksos_pstat(&st, path)) {
        kso_catch_ignore();
        *res = false;
        return true;
    } else {
        *res = true;
        return true;
    }
}

bool ksos_path_isfile(kso path, bool* res) {
    struct ksos_cstat st;
    if (!ksos_pstat(&st, path)) {
        return false;
    }

    *res = KSOS_CSTAT_ISFILE(st);
    return true;
}

bool ksos_path_isdir(kso path, bool* res) {
    struct ksos_cstat st;
    if (!ksos_pstat(&st, path)) {
        return false;
    }

    *res = KSOS_CSTAT_ISDIR(st);
    return true;
}

bool ksos_path_islink(kso path, bool* res) {
    struct ksos_cstat st;
    if (!ksos_pstat(&st, path)) {
        return false;
    }

    *res = KSOS_CSTAT_ISLINK(st);
    return true;
}


/* Type Functions */

static KS_TFUNC(T, free) {
    ksos_path self;
    KS_ARGS("self:*", &self, ksost_path);

    if (self->str_) {
        KS_DECREF(self->str_);
    }
    KS_DECREF(self->parts);
    KS_DECREF(self->root);

    KSO_DEL(self);

    return KSO_NONE;
}

static KS_TFUNC(T, new) {
    ks_type tp;
    kso src = KSO_NONE;
    ks_str root = NULL;
    KS_ARGS("tp:* ?src ?root:*", &tp, kst_type, &src, &root, kst_str);

    if (root != NULL) {
        if (kso_issub(src->type, kst_tuple)) {
            ksos_path res = KSO_NEW(ksos_path, tp);
            res->str_ = NULL;
            res->parts = (ks_tuple)KS_NEWREF(src);
            res->root = KS_NEWREF(root);
            return (kso)res;
        }
        if (!kso_issub(src->type, kst_str)) {
            KS_THROW(kst_ArgError, "'src' must be a 'str' object when 'root' is given, but it was a '%T' object", src);
            return NULL;
        }

        return (kso)ksos_path_newt(tp, ((ks_str)src)->len_b, ((ks_str)src)->data, (kso)root);
    } else {
        if (kso_issub(src->type, kst_tuple)) {
            ksos_path res = KSO_NEW(ksos_path, tp);
            res->str_ = NULL;
            res->parts = (ks_tuple)KS_NEWREF(src);
            res->root = KSO_NONE;
            return (kso)res;
        }

        return (kso)ksos_path_new_ot(tp, src);
    }
}

static KS_TFUNC(T, repr) {
    ksos_path self;
    KS_ARGS("self:*", &self, ksost_path);
    if (self->root == KSO_NONE) {
        return (kso)ks_fmt("%T(%R)", self, self->parts);
    } else {
        return (kso)ks_fmt("%T(%R, %R)", self, self->parts, self->root);
    }
}

static KS_TFUNC(T, eq) {
    kso L, R;
    KS_ARGS("L R", &L, &R);

    if ((kso_issub(L->type, kst_str) || kso_issub(L->type, ksost_path)) && (kso_issub(R->type, kst_str) || kso_issub(R->type, ksost_path))) {
        ks_str Ls = ks_fmt("%S", L);
        if (!Ls) return NULL;
        ks_str Rs = ks_fmt("%S", R);
        if (!Rs) {
            KS_DECREF(Ls);
            return NULL;
        }
        bool res;
        if (!kso_eq((kso)Ls, (kso)Rs, &res)) {
            KS_DECREF(Ls);
            KS_DECREF(Rs);
            return NULL;
        }
    
        KS_DECREF(Ls);
        KS_DECREF(Rs);
        return KSO_BOOL(res);
    }
    return KSO_UNDEFINED;
}

static KS_TFUNC(T, div) {
    kso L, R;
    KS_ARGS("L R", &L, &R);

    if ((kso_issub(L->type, kst_str) || kso_issub(L->type, ksost_path)) && (kso_issub(R->type, kst_str) || kso_issub(R->type, ksost_path))) {
        return (kso)ksos_path_join(_args, _nargs);
    }

    return KSO_UNDEFINED;
}

static KS_TFUNC(T, abs) {
    kso self;
    KS_ARGS("self", &self);

    return (kso)ksos_path_real(self);
}

static KS_TFUNC(T, join) {
    return (kso)ksos_path_join(_args, _nargs);
}

static KS_TFUNC(T, parent) {
    kso self;
    KS_ARGS("self", &self);

    return (kso)ksos_path_parent(self);
}

static KS_TFUNC(T, exists) {
    kso self;
    KS_ARGS("self", &self);

    bool res;
    if (!ksos_path_exists(self, &res)) return NULL;

    return KSO_BOOL(res);
}

static KS_TFUNC(T, isfile) {
    kso self;
    KS_ARGS("self", &self);

    bool res;
    if (!ksos_path_isfile(self, &res)) return NULL;

    return KSO_BOOL(res);
}

static KS_TFUNC(T, isdir) {
    kso self;
    KS_ARGS("self", &self);

    bool res;
    if (!ksos_path_isdir(self, &res)) return NULL;

    return KSO_BOOL(res);
}

static KS_TFUNC(T, islink) {
    kso self;
    KS_ARGS("self", &self);

    bool res;
    if (!ksos_path_islink(self, &res)) return NULL;

    return KSO_BOOL(res);
}


/* Export */

static struct ks_type_s tp;
ks_type ksost_path = &tp;


void _ksi_os_path() {

    _ksinit(ksost_path, kst_object, T_NAME, sizeof(struct ksos_path_s), -1, "Path on the filesystem, which can be read, written, or 'stat'd to determine properties\n\n    This is a higher level interface than 'str', which can be used in many of the functions, but makes it difficult to reason about the path. Therefore, this type defines a more object-oriented approach to path treatment", KS_IKV(
        {"__free",                 ksf_wrap(T_free_, T_NAME ".__free(self)", "")},
        {"__new",                  ksf_wrap(T_new_, T_NAME ".__new(tp, src='', root=none)", "")},
        {"__repr",                 ksf_wrap(T_repr_, T_NAME ".__repr(self)", "")},

        {"__eq",                   ksf_wrap(T_eq_, T_NAME ".__eq(L, R)", "")},
        
        {"__abs",                  ksf_wrap(T_abs_, T_NAME ".__abs(self)", "")},
        {"__div",                  ksf_wrap(T_div_, T_NAME ".__div(L, R)", "")},

        {"parent",                 ksf_wrap(T_parent_, T_NAME ".parent(self)", "Computes the parent path")},

        {"join",                   ksf_wrap(T_join_, T_NAME ".join(*args)", "Joins paths together, accepts 'str', 'os.path', and other path-like objects")},
        {"real",                   ksf_wrap(T_abs_, T_NAME ".real(self)", "Computes the real path (i.e. actual location within the filesystem)\n\n    Returns a path with an absolute root")},

        {"exists",                 ksf_wrap(T_exists_, T_NAME ".exists(self)", "Computes whether 'self' exists on the file system")},
        {"isfile",                 ksf_wrap(T_isfile_, T_NAME ".isfile(self)", "Computes whether 'self' is a regular file")},
        {"isdir",                  ksf_wrap(T_isdir_, T_NAME ".isdir(self)", "Computes whether 'self' is a directory")},
        {"islink",                 ksf_wrap(T_islink_, T_NAME ".islink(self)", "Computes whether 'self' is a symbolic link")},

        {"walk",                   KS_NEWREF(ksost_walk)},

    ));
    
}
