/* min.c - 'min' kernel
 *
 * 
 * @author: Cade Brown <cade@kscript.org>
 */
#include <ks/impl.h>
#include <ks/nx.h>
#include <ks/nxt.h>
#include <ks/nxm.h>

#define K_NAME "min"


struct extra_data {

    /* Whether the result has been initialized */
    bool init;

};

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
        sR = R.strides[0] \
    ; \
    ks_cint i; \
    bool init = ((struct extra_data*)extra)->init; \
    if (!init) { \
        *(TYPE*)pR = *(TYPE*)pX; \
        ((struct extra_data*)extra)->init = true; \
    } \
    TYPE x; \
    for (i = 0; i < len; i++, pX += sX, pR += sR) { \
        x = *(TYPE*)pX; \
        if (x < *(TYPE*)pR) *(TYPE*)pR = x; \
    } \
    return 0; \
}

#define LOOPC(TYPE, NAME) static int kern_##NAME(int N, nx_t* args, int len, void* extra) { \
    assert(N == 2); \
    nx_t X = args[0], R = args[1]; \
    assert(X.dtype == R.dtype); \
    ks_uint \
        pX = (ks_uint)X.data, \
        pR = (ks_uint)R.data  \
    ; \
    ks_cint \
        sX = X.strides[0] \
    ; \
    bool init = ((struct extra_data*)extra)->init; \
    if (!init) { \
        *(TYPE*)pR = *(TYPE*)pX; \
        ((struct extra_data*)extra)->init = true; \
    } \
    ks_cint i; \
    TYPE x; \
    TYPE##r a2 = TYPE##rhypot(((TYPE*)pR)->re, ((TYPE*)pR)->im), xa2; \
    for (i = 0; i < len; i++, pX += sX) { \
        x = *(TYPE*)pX; \
        xa2 = TYPE##rhypot(x.re, x.im); \
        if (xa2 < a2) { \
            *(TYPE*)pR = x; \
            a2 = xa2; \
        } \
    } \
    return 0; \
}

NXT_PASTE_I(LOOPR);
NXT_PASTE_F(LOOPR);

NXT_PASTE_C(LOOPC);

bool nx_min(nx_t X, nx_t R, int naxes, int* axes) {
    if (X.rank == R.rank) {
        return nx_cast(X, R);
    } else if (X.rank < R.rank) {
        KS_THROW(kst_SizeError, "Unsupported ranks for kernel '%s': X.rank=%i, R.rank=%i (expected R.rank<=X.rank)", K_NAME, X.rank, R.rank);
        return false;
    }

    if (R.rank != X.rank - naxes) {
        KS_THROW(kst_SizeError, "Unsupported ranks for kernel '%s': X.rank=%i, R.rank=%i, naxes=%i (expected R.rank==X.rank-naxes)", K_NAME, X.rank, R.rank, naxes);
        return false;
    }

    if (!nx_zero(R)) return false;

    struct extra_data ed;
    ed.init = false;

    nx_t cX;
    void* fX = NULL;
    if (!nx_getcast(X, R.dtype, &cX, &fX)) {
        return false;
    }

    /* Insert axes */
    R = nx_with_axes(R, naxes, axes);

    #define LOOP(NAME) do { \
        bool res = !nx_apply_elem(kern_##NAME, 2, (nx_t[]){ X, R }, &ed); \
        ks_free(fX); \
        return res; \
    } while (0);

    NXT_FOR_ALL(R.dtype, LOOP);
    #undef LOOP

    ks_free(fX);
    KS_THROW(kst_TypeError, "Unsupported types for kernel '%s': %R, %R", K_NAME, X.dtype, R.dtype);
    return false;
}
