/* cumsum.c - 'cumsum' kernel
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
#define NXK_FILE "cumprod.kern"
#define K_NAME "cumprod"
#include <ks/nxk.h>

bool nx_cumprod(nx_t X, nx_t R, int axis) {
    if (R.rank != X.rank) {
        KS_THROW(kst_SizeError, "Unsupported ranks for kernel '%s': %i, %i (expected R.rank==X.rank)", K_NAME, X.rank, R.rank);
        return false;
    }

    if (!nx_one(R)) {
        return false;
    }

    /* Sunken versions, so it applies on the last axes */
    nx_t sX = nx_sinkaxes(X, 1, &axis);
    nx_t sR = nx_sinkaxes(R, 1, &axis);

    if (false) {}
    #define LOOP(TYPE, NAME) else if (sR.dtype == nxd_##NAME) { \
        bool res = !nx_apply_Nd(KERN_FUNC(NAME), 2, (nx_t[]){ sX, sR }, 1, sR.dtype, NULL); \
        return res; \
    }
    NXT_PASTE_IFC(LOOP)
    #undef LOOP

    KS_THROW(kst_TypeError, "Unsupported types for kernel '%s': %R, %R", K_NAME, X.dtype, R.dtype);
    return false;
}
