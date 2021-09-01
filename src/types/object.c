/* types/object.c - 
 *
 *
 *
 *
 * @author: Cade Brown <cade@kscript.org>
 */

#include <ks/impl.h>

#define T kso
#define T_TYPE kst_object
#define T_NAME "object"
#define T_DOC "Base type of all other types in kscript (everything is an object), which is used as a building block for other types"


/* C-API */


/* Functions */

static KS_TFUNC(T, hash) {
    T self;
    KS_ARGS("self", &self);

    /* hash(self) = address of self */
    return (kso)ks_int_newu((ks_uint)self);
}


/* Export */

KS_DECL_TYPE(T_TYPE)

void ksi_object() {

    ks_init_type(T_TYPE, NULL, sizeof(*(T)NULL), T_NAME, T_DOC, KS_IKV(
        {"__hash",                 ksf_wrap(T_hash_, T_NAME ".__hash(self)", "")},

    ));

}
