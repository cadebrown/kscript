/* pow.c - 'pow' kernel
 *
 * @author: Cade Brown <cade@kscript.org>
 */
#include <ks/impl.h>
#include <ks/nx.h>
#include <ks/nxt.h>
#include <ks/nxm.h>

#define K_NAME "pow"


#define LOOPI(TYPE, NAME) static int kern_##NAME(int N, nx_t* args, int len, void* extra) { \
    assert(N == 3); \
    nx_t X = args[0], Y = args[1], R = args[2]; \
    KS_THROW(kst_TypeError, "Unsupported types for kernel '%s': %R, %R, %R", K_NAME, X.dtype, Y.dtype, R.dtype); \
    return 1; \
}

#define LOOPR(TYPE, NAME) static int kern_##NAME(int N, nx_t* args, int len, void* extra) { \
    assert(N == 3); \
    nx_t X = args[0], Y = args[1], R = args[2]; \
    assert(X.dtype == R.dtype); \
    ks_uint \
        pX = (ks_uint)X.data, \
        pY = (ks_uint)Y.data, \
        pR = (ks_uint)R.data  \
    ; \
    ks_cint \
        sX = X.strides[0], \
        sY = Y.strides[0], \
        sR = R.strides[0]  \
    ; \
    ks_cint i; \
    for (i = 0; i < len; i++, pX += sX, pY += sY, pR += sR) { \
        *(TYPE*)pR = TYPE##pow(*(TYPE*)pX, *(TYPE*)pY); \
    } \
    return 0; \
}

#define LOOPC(TYPE, NAME) static int kern_##NAME(int N, nx_t* args, int len, void* extra) { \
    assert(N == 3); \
    nx_t X = args[0], Y = args[1], R = args[2]; \
    assert(X.dtype == R.dtype); \
    ks_uint \
        pX = (ks_uint)X.data, \
        pY = (ks_uint)Y.data, \
        pR = (ks_uint)R.data  \
    ; \
    ks_cint \
        sX = X.strides[0], \
        sY = Y.strides[0], \
        sR = R.strides[0]  \
    ; \
    ks_cint i; \
    for (i = 0; i < len; i++, pX += sX, pR += sR) { \
        TYPE x = *(TYPE*)pX, y = *(TYPE*)pY, r; \
        if (y.re == 0 && y.im == 0) { \
            ((TYPE*)pR)->re = 1; \
            ((TYPE*)pR)->im = 0; \
        } else if (x.re == 0 && x.im == 0) { \
            if (y.re <= 0) { \
                ((TYPE*)pR)->re = TYPE##rNAN; \
                ((TYPE*)pR)->im = TYPE##rNAN; \
            } else { \
                ((TYPE*)pR)->re = 0; \
                ((TYPE*)pR)->im = 0; \
            } \
        } else if (x.re == 0 && y.im == 0 && (y.re == y.re && y.re < TYPE##rINF && y.re > -TYPE##rINF) && ((int)y.re) == y.re && y.re > 0) { \
            int b = (int)y.re; \
            int bm4 = b % 4; \
            TYPE##r v = TYPE##rpow(x.im, b); \
            if (bm4 == 0) { \
                ((TYPE*)pR)->re = v; \
                ((TYPE*)pR)->im = 0; \
            } else if (bm4 == 1) { \
                ((TYPE*)pR)->re = 0; \
                ((TYPE*)pR)->im = v; \
            } else if (bm4 == 2) { \
                ((TYPE*)pR)->re = -v; \
                ((TYPE*)pR)->im = 0; \
            } else { \
                ((TYPE*)pR)->re = 0; \
                ((TYPE*)pR)->im = -v; \
            } \
        } else { \
            TYPE##r a_x = TYPE##rhypot(x.re, x.im), a_y = TYPE##rhypot(y.re, y.im); \
            TYPE##r p_x = TYPE##ratan2(x.im, x.re); \
            TYPE##r a_v = TYPE##rpow(a_x, y.re), p_v = p_x * y.re; \
            if (y.im != 0) { \
                a_v *= TYPE##rexp(-p_x * y.im); \
                p_v += y.im * TYPE##rlog(a_x); \
            } \
            ((TYPE*)pR)->re = a_v * TYPE##rcos(p_v); \
            ((TYPE*)pR)->im = a_v * TYPE##rsin(p_v); \
        } \
    } \
}

NXT_PASTE_I(LOOPI);

NXT_PASTE_F(LOOPR);

NXT_PASTE_C(LOOPC);


bool nx_pow(nx_t X, nx_t Y, nx_t R) {
    nx_t cX, cY;
    void *fX = NULL, *fY = NULL;
    if (!nx_getcast(X, R.dtype, &cX, &fX)) {
        return false;
    }
    if (!nx_getcast(Y, R.dtype, &cY, &fY)) {
        ks_free(fX);
        return false;
    }

    #define LOOP(NAME) do { \
        bool res = !nx_apply_elem(kern_##NAME, 3, (nx_t[]){ cX, cY, R }, NULL); \
        ks_free(fX); \
        ks_free(fY); \
        return res; \
    } while (0);

    NXT_PASTE_ALL(R.dtype, LOOP);
    #undef LOOP

    ks_free(fX);
    ks_free(fY);

    KS_THROW(kst_TypeError, "Unsupported types for kernel '%s': %R, %R, %R", K_NAME, X.dtype, Y.dtype, R.dtype);
    return false;
}
