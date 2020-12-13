/* types/dotdotdot.c - '...' type
 *
 * @author: Cade Brown <cade@kscript.org>
 */
#include <ks/impl.h>

#define T_NAME "dotdotdot.__type"





/* Type Functions */

static KS_TFUNC(T, free) {
    kso self;
    KS_ARGS("self:*", &self, kst_dotdotdot);

    /* We shouldn't free '...', since it is a global singleton */
    self->refs = KS_REFS_INF;

    return KSO_NONE;
}

/* Export */

static struct ks_type_s tp;
ks_type kst_dotdotdot = &tp;

static struct kso_s i_dotdotdot;
kso ksg_dotdotdot = &i_dotdotdot;

void _ksi_dotdotdot() {
    
    _ksinit(kst_dotdotdot, kst_object, T_NAME, 0, -1, "'...' mean continuation, or otherwise extra values are given, but can be used in different contexts", KS_IKV(
        {"__free",               ksf_wrap(T_free_, T_NAME ".__free(self)", "")},
    ));
    
    KS_INCREF(kst_dotdotdot);
    ksg_dotdotdot->type = kst_dotdotdot;
    ksg_dotdotdot->refs = 1;

}
