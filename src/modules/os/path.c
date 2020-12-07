/* os/path.c - 'os.path' type
 *
 * 
 * @author: Cade Brown <cade@kscript.org>
 */
#include <ks/impl.h>

#define T_NAME "os.path"


/* C-API */

ksos_path ksos_path_new(ks_ssize_t len_b, const char* data, kso root) {
    if (len_b < 0) len_b = strlen(data);
    
    ksos_path self = KSO_NEW(ksos_path, ksost_path);

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
                    ks_str ss = ks_str_new(i - j, data + i);
                    ks_list_push(pl, (kso)ss);
                    KS_DECREF(ss);
                }

                /* Save the next position */
                j = i + 1;
            }
        }

        if (i > j) {
            ks_str ss = ks_str_new(i - j, data + i);
            ks_list_push(pl, (kso)ss);
            KS_DECREF(ss);
        }

        /* Canonicalize path */
        self->parts = ks_tuple_new(pl->len, pl->elems);
        KS_DECREF(pl);
    }

    return self;
}

ksos_path ksos_path_new_o(kso ob) {
    if (kso_issub(ob->type, ksost_path)) {
        KS_INCREF(ob);
        return (ksos_path)ob;
    } else if (kso_issub(ob->type, kst_str)) {
        return ksos_path_new(((ks_str)ob)->len_b, ((ks_str)ob)->data, KSO_NONE);
    }

    KS_THROW_CONV(ob->type, ksost_path);
    return NULL;
}


/* Type Functions */


/* Export */

static struct ks_type_s tp;
ks_type ksost_path = &tp;


void _ksi_os_path() {
    _ksinit(ksost_path, kst_object, T_NAME, sizeof(struct ksos_path_s), -1, "Path on the filesystem, which can be read, written, or 'stat'd to determine properties\n\n    This is a higher level interface than 'str', which can be used in many of the functions, but makes it difficult to reason about the path. Therefore, this type defines a more object-oriented approach to path treatment", KS_IKV(

    ));
    
}
