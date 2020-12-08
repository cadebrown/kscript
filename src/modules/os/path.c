/* os/path.c - 'os.path' type
 *
 * 
 * @author: Cade Brown <cade@kscript.org>
 */
#include <ks/impl.h>

#define T_NAME "os.path"


/* C-API */

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

        self->root = (kso)ks_fmt("%.*s%s", 2, data, KS_PLATFORM_PATHSEP);
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


/* Type Functions */

static KS_TFUNC(T, free) {
    ksos_path self;
    KS_ARGS("self:*", &self, ksost_path);

    KS_NDECREF(self->str_);
    KS_DECREF(self->parts);
    KS_DECREF(self->root);

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
        return (kso)ks_fmt("%T(%R)", self->parts);
    } else {
        return (kso)ks_fmt("%T(%R, %R)", self, self->parts, self->root);
    }
}
/* Export */

static struct ks_type_s tp;
ks_type ksost_path = &tp;


void _ksi_os_path() {
    _ksinit(ksost_path, kst_object, T_NAME, sizeof(struct ksos_path_s), -1, "Path on the filesystem, which can be read, written, or 'stat'd to determine properties\n\n    This is a higher level interface than 'str', which can be used in many of the functions, but makes it difficult to reason about the path. Therefore, this type defines a more object-oriented approach to path treatment", KS_IKV(
        {"__free",                 ksf_wrap(T_free_, T_NAME ".__free(self)", "")},
        {"__new",                  ksf_wrap(T_new_, T_NAME ".__new(tp, src='', root=none)", "")},
        {"__repr",                 ksf_wrap(T_repr_, T_NAME ".__repr(self)", "")},


    ));
    
}
