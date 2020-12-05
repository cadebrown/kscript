/* types/names.c - 'names' type
 *
 * @author: Cade Brown <cade@kscript.org>
 */
#include <ks/impl.h>

#define T_NAME "names"


/* C-API */


/* Export */

static struct ks_type_s tp;
ks_type kst_names = &tp;

void _ksi_names() {
    _ksinit(kst_names, kst_object, T_NAME, sizeof(struct ks_names_s), -1, NULL);
    
}
