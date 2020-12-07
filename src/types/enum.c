/* types/enum.c - 'enum' type
 *
 * @author: Cade Brown <cade@kscript.org>
 */
#include <ks/impl.h>

#define T_NAME "enum"


/* C-API */

ks_type ks_enum_make(const char* name, struct ks_eikv* eikv) {
    ks_type tp = ks_type_new(name, kst_enum, sizeof(struct ks_enum_s), -1, "", NULL);

    ks_dict i_v2m = ks_dict_new(NULL), i_n2m = ks_dict_new(NULL);

    if (eikv) {
        while (eikv->key) {
            ks_enum mem = KSO_NEW(ks_enum, tp);

            mem->name = ks_str_new(-1, eikv->key);

            mpz_init(mem->s_int.val);
            mpz_set_si(mem->s_int.val, eikv->val);

            if (!ks_dict_set(i_v2m, (kso)mem, (kso)mem)
             || !ks_dict_set(i_v2m, (kso)mem->name, (kso)mem)) {
                assert(false);
            }

            eikv++;
        }
    }

    ks_dict_set_c1(tp->attr, "_v2m", (kso)i_v2m);
    ks_dict_set_c1(tp->attr, "_n2m", (kso)i_n2m);

    return tp;
}

ks_enum ks_enum_get(ks_type tp, ks_cint val) {
    ks_dict i_v2m = (ks_dict)ks_dict_get_c(tp->attr, "_v2m");
    assert(i_v2m && i_v2m->type == kst_dict);

    ks_int k = ks_int_new(val);
    ks_enum r = (ks_enum)ks_dict_get(i_v2m, (kso)k);
    KS_DECREF(k);
    KS_DECREF(i_v2m);

    return r;
}


/* Type Functions */

static KS_TFUNC(T, getattr) {
    ks_enum self;
    ks_str attr;
    KS_ARGS("self:* attr:*", &self, kst_enum, &attr, kst_str);

    if (ks_str_eq_c(attr, "name", 4)) {
        return KS_NEWREF(self->name);
    } else if (ks_str_eq_c(attr, "val", 3)) {
        return (kso)ks_int_newz(self->s_int.val);
    }

    KS_THROW_ATTR(self, attr);
    return NULL;
}

/* Export */

static struct ks_type_s tp;
ks_type kst_enum = &tp;

void _ksi_enum() {
    _ksinit(kst_enum, kst_int, T_NAME, sizeof(struct ks_enum_s), -1, "Enumeration of (integral) values, with associated names and values\n\n    This is a numeric (and integral) type, and is treated this way in expressions\n\n    SEE: https://en.wikipedia.org/wiki/Enumeration", KS_IKV(
        {"__getattr",              ksf_wrap(T_getattr_, T_NAME ".__getattr(self, attr)", "")},
    ));
}
