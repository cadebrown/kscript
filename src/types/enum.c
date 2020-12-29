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

            ks_type_set(tp, mem->name, (kso)mem);

            eikv++;
        }
    }

    ks_dict_set_c1(tp->attr, "_v2m", (kso)i_v2m);
    ks_dict_set_c1(tp->attr, "_n2m", (kso)i_n2m);

    return tp;
}

/* Adds a member to an enum */
bool ks_enum_addmember(ks_type tp, ks_dict v2m, ks_dict n2m, ks_str name, ks_int val) {

    ks_enum mem = KSO_NEW(ks_enum, tp);
    KS_INCREF(name);
    mem->name = name;
    mpz_init(mem->s_int.val);
    mpz_set(mem->s_int.val, val->val);

    if (!ks_dict_set(v2m, (kso)mem, (kso)mem) || !ks_dict_set(v2m, (kso)mem->name, (kso)mem)) {
        assert(false);
    }

    ks_type_set(tp, mem->name, (kso)mem);

    return true;
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


static KS_TFUNC(T, new) {
    ks_type tp;
    kso of;
    KS_ARGS("tp:* of", &tp, kst_type, &of);

    if (kso_issub(of->type, kst_int)) {
        ks_dict i_v2m = (ks_dict)ks_dict_get_c(tp->attr, "_v2m");
        assert(i_v2m && i_v2m->type == kst_dict);

        ks_enum r = (ks_enum)ks_dict_get(i_v2m, (kso)of);
        KS_DECREF(i_v2m);

        return (kso)r;
    } else {
        ks_dict i_n2m = (ks_dict)ks_dict_get_c(tp->attr, "_n2m");
        assert(i_n2m && i_n2m->type == kst_dict);

        ks_enum r = (ks_enum)ks_dict_get(i_n2m, (kso)of);
        KS_DECREF(i_n2m);

        return (kso)r;
    }
}
static KS_TFUNC(T, make) {
    ks_str name;
    kso members;
    KS_ARGS("name:* members", &name, kst_str, &members);

    ks_type tp = ks_type_new(name->data, kst_enum, sizeof(struct ks_enum_s), -1, "", NULL);
    ks_dict i_v2m = ks_dict_new(NULL), i_n2m = ks_dict_new(NULL);
    if (kso_issub(members->type, kst_list)) {
        ks_list lm = (ks_list)members;
        int i;
        for (i = 0; i < lm->len; ++i) {
            ks_list ol = ks_list_newi(lm->elems[i]);
            if (!ol) return NULL;

            if (ol->len != 2 || (ol->elems[0]->type != kst_str || !(kso_is_int(ol->elems[1]) || ol->elems[1]->type->i__int))) {
                KS_THROW(kst_Error, "Given bad enum initializers, expected elements of '(name, val)' (which must be 'str' and 'int')");
                KS_DECREF(ol);
                KS_DECREF(tp);
                KS_DECREF(i_v2m);
                KS_DECREF(i_n2m);
                break;
            }
            ks_str name = (ks_str)ol->elems[0];
            ks_int val = kso_int(ol->elems[1]);
            if (!val) {
                KS_DECREF(ol);
                KS_DECREF(tp);
                KS_DECREF(i_v2m);
                KS_DECREF(i_n2m);
                return NULL;
            }

            if (!ks_enum_addmember(tp, i_v2m, i_n2m, name, val)) {
                KS_DECREF(val);
                KS_DECREF(ol);
                KS_DECREF(tp);
                KS_DECREF(i_v2m);
                KS_DECREF(i_n2m);
                return NULL;
            }

            KS_DECREF(val);
            KS_DECREF(ol);
        }
    } else {
        KS_DECREF(tp);
        KS_DECREF(i_v2m);
        KS_DECREF(i_n2m);

        KS_THROW(kst_TypeError, "Failed to create 'enum' from members of type '%T' (require a list of (key, val) members)", members);
        return NULL;
    }

    ks_dict_set_c1(tp->attr, "_v2m", (kso)i_v2m);
    ks_dict_set_c1(tp->attr, "_n2m", (kso)i_n2m);
    return (kso)tp;
}

/* Export */

static struct ks_type_s tp;
ks_type kst_enum = &tp;

void _ksi_enum() {
    _ksinit(kst_enum, kst_int, T_NAME, sizeof(struct ks_enum_s), -1, "Enumeration of (integral) values, with associated names and values\n\n    This is a numeric (and integral) type, and is treated this way in expressions\n\n    SEE: https://en.wikipedia.org/wiki/Enumeration", KS_IKV(
        {"__new",                  ksf_wrap(T_new_, T_NAME ".__new(tp, of)", "")},
        {"make",                   ksf_wrap(T_make_, T_NAME ".make(name, members)", "Creates a new enumeration from a dictionary or a list of tuples")},
        {"__getattr",              ksf_wrap(T_getattr_, T_NAME ".__getattr(self, attr)", "")},
    ));
}
