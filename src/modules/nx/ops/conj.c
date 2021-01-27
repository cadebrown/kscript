/* conj.c - 'conj' kernel
 *
 * @author: Cade Brown <cade@kscript.org>
 */
#include <ks/impl.h>
#include <ks/nxi.h>
#include <ks/nxt.h>

#define KERN_FUNC(_name) NXK_PASTE(kern_, _name)

#define NXK_DO_C
#define NXK_FILE "conj.kern"
#define K_NAME "conj"
#include <ks/nxk.h>

bool nx_conj(nx_t X, nx_t R) {

    if (X.dtype->kind == NX_DTYPE_INT || X.dtype->kind == NX_DTYPE_FLOAT) {
        return nx_cast(X, R);
    }

    if (false) {}
    #define LOOP(TYPE, NAME) else if (R.dtype == nxd_##NAME) { \
        return !nx_apply_elem(KERN_FUNC(NAME), 2, (nx_t[]){ X, R }, R.dtype, NULL); \
    }
    NXT_PASTE_C(LOOP)
    #undef LOOP

    KS_THROW(kst_TypeError, "Unsupported types for kernel '%s': %R, %R", K_NAME, X.dtype, R.dtype);
    return false;
}
