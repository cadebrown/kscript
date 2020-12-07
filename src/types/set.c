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
    _ksinit(kst_set, kst_object, T_NAME, sizeof(struct ks_set_s), -1, "A set of (unique) objects, which can be modified, ordered by first insertion order, resetting with deletion\n\n    Internally, it is a hash-set, which means only one object that hashes a certain way and compares equal with other keys may be contained. Therefore, you cannot store things like 'true' and '1' in the same hashset -- they will become the same item", KS_IKV(

    ));
    
}
