/* types/int.c - 
 *
 *
 *
 *
 * @author: Cade Brown <cade@kscript.org>
 */

#include <ks/impl.h>

#define T ks_bool
#define T_TYPE kst_bool
#define T_NAME "bool"
#define T_DOC "Immutable number type representing either 'true' (1) or 'false' (0)"


/* C-API */

/* Functions */

/* Export */

KS_DECL_TYPE(T_TYPE)

void ksi_bool() {

    ks_init_type(T_TYPE, kst_enum, sizeof(*(T)NULL), T_NAME, T_DOC, KS_IKV(

    ));
}
