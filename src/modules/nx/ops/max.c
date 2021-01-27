/* max.c - 'max' kernel
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
#define NXK_FILE "max.kern"
#define K_NAME "max"
#include <ks/nxk.h>

bool nx_max(nx_t X, nx_t R, int naxes, int* axes) {
    if (R.rank != X.rank - naxes) {
        KS_THROW(kst_SizeError, "Unsupported ranks for kernel '%s': %i, %i (expected R.rank==X.rank-naxes)", K_NAME, X.rank, R.rank);
        return false;
    } else if (naxes == 0) {
        return nx_cast(X, R);
    }

    void* dptr = ks_malloc(szprod(R.rank, R.shape) * nxd_bl->size);
    if (!dptr) {
        KS_THROW(kst_InternalError, "Failed: 'malloc()'");
        return -1;
    }
    /* Boolean of whether a given member has been initialized
     */
    nx_t Rinit = nx_make(dptr, nxd_bl, R.rank, R.shape, NULL);
    if (!nx_zero(Rinit)) {
        ks_free(dptr);
        return -1;
    }

    /* Pad it with new axes
     */
    nx_t Rpad = nx_newaxes(R, naxes, axes);
    nx_t Rinitpad = nx_newaxes(Rinit, naxes, axes);

    /* Sunken versions, so it applies on the last axes */
    nx_t sX = nx_sinkaxes(X, naxes, axes);
    nx_t sR = nx_sinkaxes(Rpad, naxes, axes);
    nx_t sRinit = nx_sinkaxes(Rinitpad, naxes, axes);

    if (false) {}
    #define LOOP(TYPE, NAME) else if (Rpad.dtype == nxd_##NAME) { \
        bool res = !nx_apply_eleme(KERN_FUNC(NAME), 3, (nx_t[]){ sX, sR, sRinit }, Rpad.dtype, 0, NULL); \
        ks_free(dptr); \
        return res; \
    }
    NXT_PASTE_IFC(LOOP)
    #undef LOOP

    KS_THROW(kst_TypeError, "Unsupported types for kernel '%s': %R, %R", K_NAME, X.dtype, R.dtype);
    ks_free(dptr);
    return false;
}
