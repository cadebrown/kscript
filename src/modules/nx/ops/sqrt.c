/* sqrt.c - 'sqrt' kernel
 *
 * @author: Cade Brown <cade@kscript.org>
 */
#include <ks/impl.h>
#include <ks/nx.h>
#include <ks/nxt.h>
#include <ks/nxm.h>

#define K_NAME "sqrt"


#define LOOPI(TYPE, NAME) static int kern_##NAME(int N, nx_t* args, int len, void* extra) { \
    assert(N == 2); \
    nx_t X = args[0], R = args[1]; \
    KS_THROW(kst_TypeError, "Unsupported types for kernel '%s': %R, %R", K_NAME, X.dtype, R.dtype); \
    return 1; \
}

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
        *(TYPE*)pR = TYPE##sqrt(*(TYPE*)pX); \
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
        sX = X.strides[0], \
        sR = R.strides[0]  \
    ; \
    ks_cint i; \
    for (i = 0; i < len; i++, pX += sX, pR += sR) { \
        TYPE x = *(TYPE*)pX; \
        if (x.re == 0 && x.im == 0) { \
            *(TYPE*)pR = x; \
        } else { \
            TYPE##r ar = TYPE##rfabs(x.re) / 8, ai = TYPE##rfabs(x.im); \
            TYPE##r s = 2 * TYPE##rsqrt(ar + TYPE##rhypot(ar, ai / 8)); \
            TYPE##r d = ai / (2 * s); \
            if (x.re >= 0.0) { \
                ((TYPE*)pR)->re = s; \
                ((TYPE*)pR)->im = TYPE##rcopysign(d, x.im); \
            } else { \
                ((TYPE*)pR)->re = d; \
                ((TYPE*)pR)->im = TYPE##rcopysign(s, x.im); \
            } \
        } \
    } \
    return 0; \
}

NXT_PASTE_I(LOOPI);

NXT_PASTE_F(LOOPR);

NXT_PASTE_C(LOOPC);


bool nx_sqrt(nx_t X, nx_t R) {
    nx_t cX;
    void *fX = NULL;
    if (!nx_getcast(X, R.dtype, &cX, &fX)) {
        return false;
    }

    #define LOOP(NAME) do { \
        bool res = !nx_apply_elem(kern_##NAME, 2, (nx_t[]){ cX, R }, NULL); \
        ks_free(fX); \
        return res; \
    } while (0);

    NXT_FOR_ALL(R.dtype, LOOP);
    #undef LOOP

    ks_free(fX);

    KS_THROW(kst_TypeError, "Unsupported types for kernel '%s': %R, %R", K_NAME, X.dtype, R.dtype);
    return false;
}
