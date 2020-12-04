/* types/number.c - 'number' type
 *
 *
 * @author: Cade Brown <cade@kscript.org>
 */
#include <ks/impl.h>

#define T_NAME "number"



/* C-API */



/* Export */

static struct ks_type_s tp;
ks_type kst_number = &tp;

void _ksi_number() {
    _ksinit(kst_number, kst_object, T_NAME, sizeof(struct kso_s), -1, NULL);
}
