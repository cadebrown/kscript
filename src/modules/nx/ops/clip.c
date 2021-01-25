/* clip.c - 'clip' kernel
 *
 * @author: Cade Brown <cade@kscript.org>
 */
#include <ks/impl.h>
#include <ks/nx.h>
#include <ks/nxt.h>
#include <ks/nxm.h>

#define K_NAME "clip"


#define LOOPR(TYPE, NAME) static int kern_##NAME(int N, nx_t* args, int len, void* extra) { \
    assert(N == 4); \
    nx_t X = args[0], Y = args[1], Z = args[2], R = args[3]; \
    assert(X.dtype == Y.dtype && Y.dtype == Z.dtype && Z.dtype == R.dtype); \
    ks_uint \
        pX = (ks_uint)X.data, \
        pY = (ks_uint)Y.data, \
        pZ = (ks_uint)Z.data, \
        pR = (ks_uint)R.data  \
    ; \
    ks_cint \
        sX = X.strides[0], \
        sY = Y.strides[0], \
        sZ = Z.strides[0], \
        sR = R.strides[0]  \
    ; \
    ks_cint i; \
    for (i = 0; i < len; i++, pX += sX, pY += sY, pZ += sZ, pR += sR) { \
        TYPE x = *(TYPE*)pX, y = *(TYPE*)pY, z = *(TYPE*)pZ; \
        *(TYPE*)pR = x < y ? y : (x > z ? z : x); \
    } \
    return 0; \
}
#define LOOPC(TYPE, NAME) static int kern_##NAME(int N, nx_t* args, int len, void* extra) { \
    assert(N == 4); \
    nx_t X = args[0], Y = args[1], Z = args[2], R = args[3]; \
    KS_THROW(kst_TypeError, "Unsupported types for kernel '%s': %R, %R, %R, %R", K_NAME, X.dtype, Y.dtype, Z.dtype, R.dtype); \
    return 1; \
}

NXT_PASTE_I(LOOPR);
NXT_PASTE_F(LOOPR);
NXT_PASTE_C(LOOPC);

bool nx_clip(nx_t X, nx_t Y, nx_t Z, nx_t R) {
    nx_t cX, cY, cZ;
    void *fX = NULL, *fY = NULL, *fZ;
    if (!nx_getcast(X, R.dtype, &cX, &fX)) {
        return false;
    }
    if (!nx_getcast(Y, R.dtype, &cY, &fY)) {
        ks_free(fX);
        return false;
    }
    if (!nx_getcast(Z, R.dtype, &cZ, &fZ)) {
        ks_free(fX);
        ks_free(fY);
        return false;
    }

    #define LOOP(NAME) do { \
        bool res = !nx_apply_elem(kern_##NAME, 4, (nx_t[]){ cX, cY, cZ, R }, NULL); \
        ks_free(fX); \
        ks_free(fY); \
        ks_free(fZ); \
        return res; \
    } while (0);

    NXT_FOR_ALL(R.dtype, LOOP);
    #undef LOOP

    ks_free(fX);
    ks_free(fY);
    ks_free(fZ);

    KS_THROW(kst_TypeError, "Unsupported types for kernel '%s': %R, %R, %R, %R", K_NAME, X.dtype, Y.dtype, Z.dtype, R.dtype);
    return false;
}
