/* abs.c - 'abs' kernel
 *
 * This kernel is a bit odd, since 'complex' values
 *   should be able to go to real outputs (and should by
 *   default)
 * 
 * @author: Cade Brown <cade@kscript.org>
 */
#include <ks/impl.h>
#include <ks/nx.h>
#include <ks/nxt.h>
#include <ks/nxm.h>

#define K_NAME "abs"


#define LOOPR(TYPE, NAME) static int kern_##NAME(int N, nx_t* args, int len, void* extra) { \
    assert(N == 2); \
    nx_t X = args[0], R = args[1]; \
    assert(X.dtype == R.dtype); \
    ks_uint \
        pX = (ks_uint)X.data, \
        pR = (ks_uint)R.data  \
    ; \
    ks_cint \
        sX = X.strides[0], \
        sR = R.strides[0]  \
    ; \
    ks_cint i; \
    for (i = 0; i < len; i++, pX += sX, pR += sR) { \
        TYPE t = *(TYPE*)pX; \
        *(TYPE*)pR = t < 0 ? -t : t; \
    } \
    return 0; \
}

#define LOOPC(TYPE, NAME) static int kern_##NAME(int N, nx_t* args, int len, void* extra) { \
    assert(N == 2); \
    nx_t X = args[0], R = args[1]; \
    ks_uint \
        pX = (ks_uint)X.data, \
        pR = (ks_uint)R.data  \
    ; \
    ks_cint \
        sX = X.strides[0], \
        sR = R.strides[0]  \
    ; \
    ks_cint i; \
    for (i = 0; i < len; i++, pX += sX, pR += sR) { \
        *(TYPE##r*)pR = TYPE##rhypot(((TYPE*)pX)->re, ((TYPE*)pX)->im); \
    } \
    return 0; \
}

NXT_PASTE_I(LOOPR);
NXT_PASTE_F(LOOPR);

NXT_PASTE_C(LOOPC);

bool nx_abs(nx_t X, nx_t R) {
    if (R.dtype->kind == NX_DTYPE_COMPLEX) {
        KS_THROW(kst_TypeError, "Unsupported types for kernel '%s': %R, %R", K_NAME, X.dtype, R.dtype);
        return false;
    }

    if (X.dtype->kind == NX_DTYPE_COMPLEX) {
        if (!(
            (X.dtype == nxd_cH && R.dtype == nxd_H) ||
            (X.dtype == nxd_cF && R.dtype == nxd_F) ||
            (X.dtype == nxd_cD && R.dtype == nxd_D) ||
            (X.dtype == nxd_cL && R.dtype == nxd_L) ||
            (X.dtype == nxd_cQ && R.dtype == nxd_Q)
        )) {
            KS_THROW(kst_TypeError, "Unsupported types for kernel '%s': %R, %R", K_NAME, X.dtype, R.dtype);
            return false;
        }
    }

    #define LOOP(NAME) do { \
        bool res = !nx_apply_elem(kern_##NAME, 2, (nx_t[]){ X, R }, NULL); \
        return res; \
    } while (0);

    NXT_FOR_ALL(X.dtype, LOOP);
    #undef LOOP

    KS_THROW(kst_TypeError, "Unsupported types for kernel '%s': %R, %R", K_NAME, X.dtype, R.dtype);
    return false;
}
