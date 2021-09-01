/* types/number.c - 
 *
 *
 *
 *
 * @author: Cade Brown <cade@kscript.org>
 */

#include <ks/impl.h>

#define T kso
#define T_TYPE kst_number
#define T_NAME "number"
#define T_DOC "Immutable value that represents a numerical quantitiy. Can include integers, floats, complex numbers, rational numbers, and more"


/* C-API */

/* Functions */


/* Export */

KS_DECL_TYPE(T_TYPE)

void ksi_number() {

    ks_init_type(T_TYPE, kst_object, sizeof(*(T)NULL), T_NAME, T_DOC, KS_IKV(

    ));
}
