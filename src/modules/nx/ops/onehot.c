/* onehot.c - 'onehot' kernel
 *
 * @author: Cade Brown <cade@kscript.org>
 */
#include <ks/impl.h>
#include <ks/nxi.h>
#include <ks/nxt.h>

#define KERN_FUNC(_name) NXK_PASTE(kern_, _name)

#define NXK_DO_I
#define NXK_DO_F
#define NXK_DO_C
#define NXK_FILE "onehot.kern"
#define K_NAME "onehot"
#include <ks/nxk.h>

bool nx_onehot(nx_t X, nx_t R) {
    if (!nx_zero(R)) {
        return false;
    }

    /* Xpad = X[..., new], so it broadcasts correctly */
    nx_t Xpad = nx_newaxis(X, X.rank);

    if (false) {}
    #define LOOP(TYPE, NAME) else if (R.dtype == nxd_##NAME) { \
        return !nx_apply_Nde(KERN_FUNC(NAME), 2, (nx_t[]){ Xpad, R }, 2, nxd_idx, 0, NULL); \
    }
    NXT_PASTE_IFC(LOOP)
    #undef LOOP

    KS_THROW(kst_TypeError, "Unsupported types for kernel '%s': %R, %R", K_NAME, X.dtype, R.dtype);
    return false;
}
