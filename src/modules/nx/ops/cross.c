/* cross.c - 'cross' kernel
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
#define NXK_FILE "cross.kern"
#define K_NAME "cross"
#include <ks/nxk.h>

bool nx_cross(nx_t X, nx_t Y, nx_t R) {

    if (X.rank < 1 || Y.rank < 1 || R.rank < 1) {
        KS_THROW(kst_SizeError, "Unsupported ranks for kernel '%s': %i, %i, %i (must > 0)", K_NAME, X.rank, Y.rank, R.rank);
        return false;
    }
    if (X.shape[X.rank - 1] != 3 || Y.shape[Y.rank - 1] != 3 || R.shape[R.rank - 1] != 3) {
        KS_THROW(kst_SizeError, "Unsupported shapes for kernel '%s': All must end with '3'", K_NAME);
        return false;
    }

    if (false) {}
    #define LOOP(TYPE, NAME) else if (R.dtype == nxd_##NAME) { \
        return !nx_apply_Nd(KERN_FUNC(NAME), 3, (nx_t[]){ X, Y, R }, 1, R.dtype, NULL); \
    }
    NXT_PASTE_IFC(LOOP)
    #undef LOOP

    KS_THROW(kst_TypeError, "Unsupported types for kernel '%s': %R, %R, %R", K_NAME, X.dtype, Y.dtype, R.dtype);
    return false;
}
