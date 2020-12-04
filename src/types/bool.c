/* types/bool.c - 'bool' type
 *
 * @author: Cade Brown <cade@kscript.org>
 */
#include <ks/impl.h>

#define T_NAME "bool"


/* C-API */


/* Export */

static struct ks_type_s tp;
ks_type kst_bool = &tp;


static struct ks_enum_s i_true, i_false;
ks_bool ksg_true = &i_true, ksg_false = &i_false;


void _ksi_bool() {
    _ksinit(kst_bool, kst_enum, T_NAME, sizeof(struct ks_enum_s), -1, NULL);
}
