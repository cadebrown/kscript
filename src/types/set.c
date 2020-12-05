/* types/set.c - 'set' type
 *
 * @author: Cade Brown <cade@kscript.org>
 */
#include <ks/impl.h>

#define T_NAME "set"


/* C-API */


/* Export */

static struct ks_type_s tp;
ks_type kst_set = &tp;

void _ksi_set() {
    _ksinit(kst_set, kst_object, T_NAME, sizeof(struct ks_set_s), -1, NULL);
    
}
