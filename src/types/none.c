/* types/none.c - 'none' type
 *
 * @author: Cade Brown <cade@kscript.org>
 */
#include <ks/impl.h>

#define T_NAME "none.__type"



/* Export */

static struct ks_type_s tp;
ks_type kst_none = &tp;

static struct kso_s i_none;
kso ksg_none = &i_none;

void _ksi_none() {
    
    _ksinit(kst_none, kst_object, T_NAME, 0, -1, KS_IKV(

    ));
    
    KS_INCREF(kst_none);
    ksg_none->type = kst_none;
    ksg_none->refs = 1;

}
