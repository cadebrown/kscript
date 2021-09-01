/* types/float.c - 
 *
 *
 *
 *
 * @author: Cade Brown <cade@kscript.org>
 */

#include <ks/impl.h>

#define T ks_float
#define T_TYPE kst_float
#define T_NAME "float"
#define T_DOC "Immutable numerical quantity representing a floating point value"


/* C-API */

/* Methods */

/* Export */

KS_DECL_TYPE(T_TYPE)

void ksi_float() {

    ks_init_type(T_TYPE, kst_number, sizeof(*(T)NULL), T_NAME, T_DOC, KS_IKV(

    ));
}
