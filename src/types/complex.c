/* types/complex.c - 
 *
 *
 *
 *
 * @author: Cade Brown <cade@kscript.org>
 */

#include <ks/impl.h>

#define T ks_complex
#define T_TYPE kst_complex
#define T_NAME "complex"
#define T_DOC "Immutable numerical quantity representing a complex floating point value"


/* C-API */

/* Methods */

/* Export */

KS_DECL_TYPE(T_TYPE)

void ksi_complex() {

    ks_init_type(T_TYPE, kst_number, sizeof(*(T)NULL), T_NAME, T_DOC, KS_IKV(

    ));
}
