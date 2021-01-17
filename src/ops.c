/* ops.c - implements operator overloading
 *
 * 
 * @author: Cade Brown <cade@kscript.org>
 */
#include <ks/impl.h>


/* Template for binary operators */
#define T_BOP(_str, _name, _attr) kso ks_bop_##_name(kso L, kso R) { \
    kso res = NULL; \
    if (L->type->_attr) { \
        res = kso_call(L->type->_attr, 2, (kso[]){ L, R }); \
        if (res == KSO_UNDEFINED) {} \
        else return res; \
    } \
    if (R->type->_attr) { \
        res = kso_call(R->type->_attr, 2, (kso[]){ L, R }); \
        if (res == KSO_UNDEFINED) {} \
        else return res; \
    } \
    KS_THROW(kst_Error, "Binary operator '%s' undefined for '%T' and '%T'", _str, L, R); \
    return NULL; \
}


/* Instantiate */
T_BOP("+", add, i__add)
T_BOP("-", sub, i__sub)
T_BOP("*", mul, i__mul)
T_BOP("@", matmul, i__matmul)
T_BOP("/", div, i__div)
T_BOP("//", floordiv, i__floordiv)
T_BOP("%", mod, i__mod)
T_BOP("**", pow, i__pow)
T_BOP("|", binior, i__binior)
T_BOP("&", binand, i__binand)
T_BOP("^", binxor, i__binxor)
T_BOP("<<", lsh, i__lsh)
T_BOP(">>", rsh, i__rsh)
T_BOP("<", lt, i__lt)
T_BOP(">", gt, i__gt)
T_BOP("<=", le, i__le)
T_BOP(">=", ge, i__ge)

/* Template for unary operators */
#define T_UOP(_str, _name, _attr) kso ks_uop_##_name(kso V) { \
    kso res = NULL; \
    if (V->type->_attr) { \
        res = kso_call(V->type->_attr, 1, (kso[]){ V }); \
        if (res == KSO_UNDEFINED) {} \
        else return res; \
    } \
    KS_THROW(kst_Error, "Unary operator '%s' undefined for '%T'", _str, V); \
    return NULL; \
}

/* Instantiate */
T_UOP("+", pos, i__pos)
T_UOP("-", neg, i__neg)
T_UOP("~", sqig, i__sqig)

kso ks_contains(kso L, kso R) {
    if (L->type->i__contains) {
        return kso_call(L->type->i__contains, 2, (kso[]){ L, R });
    }

    KS_THROW_METH(L, "__contains");
    return NULL; 
}