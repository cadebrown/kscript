/* types/code.c - 'code' type
 *
 *
 * @author: Cade Brown <cade@kscript.org>
 */
#include <ks/impl.h>
#include <ks/compiler.h>

#define T_NAME "code"



/* C-API */

ks_code ks_code_new(ks_str fname, ks_str src) {
    ks_code self = KSO_NEW(ks_code, kst_code);

    KS_INCREF(fname);
    self->fname = fname;
    KS_INCREF(src);
    self->src = src;

    self->n_meta = 0;
    self->meta = NULL;

    self->vc = ks_list_new(0, NULL);
    self->vc_map = ks_dict_new(NULL);

    self->bc = ksio_BytesIO_new();

    return self;
}

ks_code ks_code_from(ks_code from) {
    ks_code self = KSO_NEW(ks_code, kst_code);

    KS_INCREF(from->fname);
    self->fname = from->fname;
    KS_INCREF(from->src);
    self->src = from->src;

    self->n_meta = 0;
    self->meta = NULL;

    /* Add a reference to the parent */
    KS_INCREF(from->vc);
    self->vc = from->vc;
    KS_INCREF(from->vc_map);
    self->vc_map = from->vc_map;

    self->bc = ksio_BytesIO_new();

    return self;
}

int ks_code_addconst(ks_code self, kso ob) {
    int i;
    for (i = 0; i < self->vc->len; ++i) {
        kso v = self->vc->elems[i];

        /* Determine whether the objects are equal in both type and value 
         * This is neccessary because we don't want 'true' to map to '1', since
         *   they are technically different
         */
        bool eq = ob == v;
        if (!eq) {
            eq = ob->type == v->type;
            if (eq) {
                if (!kso_eq(v, ob, &eq)) {
                    kso_catch_ignore();
                    eq = false;
                }
            }
        }

        if (eq) {
            /* Found a match */
            return i;
        }
    }


    /* Not found, so push it and return the last index */
    i = self->vc->len;
    ks_list_push(self->vc, ob);

    return i;
}

void ks_code_emit(ks_code self, ksb op) {
    ksio_addbuf((ksio_AnyIO)self->bc, sizeof(op), (const char*)&op);
}
void ks_code_emiti(ks_code self, ksb op, int arg) {
    ksba o;
    o.op = op;
    o.arg = arg;
    ksio_addbuf((ksio_AnyIO)self->bc, sizeof(o), (const char*)&o);
}
void ks_code_emito(ks_code self, ksb op, kso arg) {
    ks_code_emiti(self, op, ks_code_addconst(self, arg));
}

void ks_code_meta(ks_code self, ks_tok tok) {
    if (self->n_meta > 0 && self->meta[self->n_meta - 1].bc_n == self->bc->len_b) {
        return;
    }
    int i = self->n_meta++;
    self->meta = ks_zrealloc(self->meta, sizeof(*self->meta), self->n_meta);

    self->meta[i].bc_n = self->bc->len_b;
    self->meta[i].tok = tok;

    if (i > 0) {
        /* TODO: allow other cases? */
        assert(self->meta[i - 1].bc_n < self->meta[i].bc_n);
    }
}

/* Type Functions */

static KS_TFUNC(T, free) {
    ks_code self;
    KS_ARGS("self:*", &self, kst_code);

    KS_DECREF(self->fname);
    KS_DECREF(self->src);

    KS_DECREF(self->vc);
    KS_DECREF(self->vc_map);

    KS_DECREF(self->bc);

    ks_free(self->meta);

    KSO_DEL(self);

    return KSO_NONE;
}

static KS_TFUNC(T, str) {
    ks_code self;
    KS_ARGS("self:*", &self, kst_code);

    ks_bytes bc = ksio_BytesIO_get(self->bc);
    if (!bc) return NULL;
    ks_str res = ks_fmt("<%R>", bc);
    KS_DECREF(bc);
    return (kso)res;
}


/* Export */

static struct ks_type_s tp;
ks_type kst_code = &tp;

void _ksi_code() {
    _ksinit(kst_code, kst_object, T_NAME, sizeof(struct ks_code_s), -1, KS_IKV(
        {"__free",               ksf_wrap(T_free_, T_NAME ".__free(self)", "")},
        {"__str",                ksf_wrap(T_str_, T_NAME ".__str(self)", "")},
        {"__repr",               ksf_wrap(T_str_, T_NAME ".__repr(self)", "")},
    ));
}
