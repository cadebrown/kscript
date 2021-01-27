/* prod.c - 'prod' kernel
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
#define NXK_FILE "prod.kern"
#define K_NAME "prod"
#include <ks/nxk.h>

bool nx_prod(nx_t X, nx_t R, int naxes, int* axes) {
    if (R.rank != X.rank - naxes) {
        KS_THROW(kst_SizeError, "Unsupported ranks for kernel '%s': %i, %i (expected R.rank==X.rank-naxes)", K_NAME, X.rank, R.rank);
        return false;
    } else if (naxes == 0) {
        return nx_cast(X, R);
    }

    if (!nx_one(R)) {
        return false;
    }

    /* Pad it with new axes
     */
    nx_t Rpad = nx_newaxes(R, naxes, axes);

    /* Sunken versions, so it applies on the last axes */
    nx_t sX = nx_sinkaxes(X, naxes, axes);
    nx_t sR = nx_sinkaxes(Rpad, naxes, axes);

    if (false) {}
    #define LOOP(TYPE, NAME) else if (Rpad.dtype == nxd_##NAME) { \
        bool res = !nx_apply_eleme(KERN_FUNC(NAME), 2, (nx_t[]){ sX, sR }, Rpad.dtype, 0, NULL); \
        return res; \
    }
    NXT_PASTE_IFC(LOOP)
    #undef LOOP

    KS_THROW(kst_TypeError, "Unsupported types for kernel '%s': %R, %R", K_NAME, X.dtype, R.dtype);
    return false;
}
