/* Token.c - implementation of the token type
 *
 * 
 * @author: Cade Brown <brown.cade@gmail.com>
 */

#include "ks/impl.h"
#include <ks/gram.h>

#define T_NAME "gram.Token"

/* Constants/Definitions/Utilities */


/* C-API Interface */

ksgram_Token ksgram_Token_new(ks_type tp, kso kind, ks_str val, ks_cint sline, ks_cint scol, ks_cint eline, ks_cint ecol, ks_cint pos_b, ks_cint pos_c, ks_cint len_b, ks_cint len_c) {
    ksgram_Token self = KSO_NEW(ksgram_Token, tp);;

    KS_INCREF(kind);
    self->kind = kind;
    KS_INCREF(val);
    self->val = val;

    self->sline = sline;
    self->scol = scol;
    self->eline = eline;
    self->ecol = ecol;
    self->pos_b = pos_b;
    self->pos_c = pos_c;
    self->len_b = len_b;
    self->len_c = len_c;

    return self;
}

/* Type Functions */


static KS_TFUNC(T, new) {
    ks_type tp;
    kso kind;
    ks_str val;
    KS_ARGS("tp:* kind val:*", &tp, kst_type, &kind, &val, kst_str);

    return (kso)ksgram_Token_new(tp, kind, val, 0, 0, 0, 0, 0, 0, 0, 0);
}

static KS_TFUNC(T, free) {
    ksgram_Token self;
    KS_ARGS("self:*", &self, ksgramt_Token);

    KS_DECREF(self->val);

    KSO_DEL(self);

    return KSO_NONE;
}

static KS_TFUNC(T, repr) {
    ksgram_Token self;
    KS_ARGS("self:*", &self, ksgramt_Token);

    return (kso)ks_fmt("%T(%R, %R)", self, self->kind, self->val);
}

static KS_TFUNC(T, str) {
    ksgram_Token self;
    KS_ARGS("self:*", &self, ksgramt_Token);

    return KS_NEWREF(self->val);
}

static KS_TFUNC(T, int) {
    ksgram_Token self;
    KS_ARGS("self:*", &self, ksgramt_Token);

    return (kso)self->kind;
}

static KS_TFUNC(T, getattr) {
    ksgram_Token self;
    ks_str attr;
    KS_ARGS("self:* attr:*", &self, ksgramt_Token, &attr, kst_str);

    if (ks_str_eq_c(attr, "kind", 4)) {
        return KS_NEWREF(self->kind);
    } else if (ks_str_eq_c(attr, "val", 3)) {
        return KS_NEWREF(self->val);
    }

    KS_THROW_ATTR(self, attr);
    return NULL;
}


/* Export */

static struct ks_type_s tp;
ks_type ksgramt_Token = &tp;

void _ksi_gram_Token() {
    _ksinit(ksgramt_Token, kst_object, T_NAME, sizeof(struct ksgram_Token_s), -1, "Token within grammar", KS_IKV(

        {"__new",                ksf_wrap(T_new_, T_NAME ".__new(self, val, kind)", "")},
        {"__free",               ksf_wrap(T_free_, T_NAME ".__free(self)", "")},

        {"__repr",               ksf_wrap(T_repr_, T_NAME ".__repr(self)", "")},
        {"__str",                ksf_wrap(T_str_, T_NAME ".__str(self)", "")},
        {"__int",                ksf_wrap(T_int_, T_NAME ".__int(self)", "")},
        {"__getattr",            ksf_wrap(T_getattr_, T_NAME ".__getattr(self, attr)", "")},

    ));

}

