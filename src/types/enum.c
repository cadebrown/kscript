/* types/enum.c - 'enum' type
 *
 * @author: Cade Brown <cade@kscript.org>
 */
#include <ks/impl.h>

#define T_NAME "enum"


/* C-API */


/* Export */

static struct ks_type_s tp;
ks_type kst_enum = &tp;

void _ksi_enum() {
    _ksinit(kst_enum, kst_int, T_NAME, sizeof(struct ks_enum_s), -1, NULL);
}
