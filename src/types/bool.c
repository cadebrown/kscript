/* types/bool.c - 'bool' type
 *
 * @author: Cade Brown <cade@kscript.org>
 */
#include <ks/impl.h>

#define T_NAME "bool"


/* C-API */



/* Type Functions */

static KS_TFUNC(T, free) {
    ks_bool self;
    KS_ARGS("self:*", &self, kst_bool);

    /* Never free, as they are constants */
    self->s_int.refs = KS_REFS_INF;

    return KSO_NONE;
}

/* Export */

static struct ks_type_s tp;
ks_type kst_bool = &tp;


static struct ks_enum_s i_true, i_false;
ks_bool ksg_true = &i_true, ksg_false = &i_false;


void _ksi_bool() {
    ksg_false->s_int.type = kst_bool;
    ksg_true->s_int.type = kst_bool;

    ksg_false->name = ks_str_new(-1, "false");
    ksg_true->name = ks_str_new(-1, "true");

    mpz_init(ksg_false->s_int.val);
    mpz_init(ksg_true->s_int.val);

    mpz_set_si(ksg_false->s_int.val, 0);
    mpz_set_si(ksg_true->s_int.val, 1);

    _ksinit(kst_bool, kst_enum, T_NAME, sizeof(struct ks_enum_s), -1, KS_IKV(
        {"__free",                 ksf_wrap(T_free_, T_NAME ".__free(self)", "")},
        {"false",                  KS_NEWREF(ksg_false)},
        {"true",                   KS_NEWREF(ksg_true)},
    ));

}
