/* types/ast.c - 
 *
 *
 *
 *
 * @author: Cade Brown <cade@kscript.org>
 */

#include <ks/impl.h>

#define T ks_ast
#define T_TYPE kst_ast
#define T_NAME "AST"
#define T_DOC "Base type of abstract syntax tree types, which represent code/operations"


/* C-API */


/* Methods */

static KS_TFUNC(T, hash) {
    T self;
    KS_ARGS("self", &self);

    /* hash(self) = address of self */
    return (kso)ks_int_newu(NULL, (ks_uint)self);
}


static KS_TFUNC(T, free) {
    T self;
    KS_ARGS("self:*", &self, T_TYPE);

    KS_DECREF(self->sub);

    KS_DEL(self);

    /* hash(self) = address of self */
    return KS_NONE;
}



/* Export */

KS_DECL_TYPE(T_TYPE)

void ksi_object() {

    ks_init_type(T_TYPE, NULL, sizeof(*(T)NULL), T_NAME, T_DOC, KS_IKV(
        {"__hash",                 ksf_wrap(T_hash_, T_NAME ".__hash(self)", "")},
    ));

}
