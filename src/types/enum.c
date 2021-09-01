/* types/enum.c - 
 *
 *
 *
 *
 * @author: Cade Brown <cade@kscript.org>
 */

#include <ks/impl.h>

#define T ks_enum
#define T_TYPE kst_enum
#define T_NAME "enum"
#define T_DOC "Immutable numerical quantity representing an integral value, and with an associated name"


/* C-API */

/* Functions */


/* Export */

KS_DECL_TYPE(T_TYPE)

void ksi_enum() {

    ks_init_type(T_TYPE, kst_int, sizeof(*(T)NULL), T_NAME, T_DOC, KS_IKV(

    ));
}
