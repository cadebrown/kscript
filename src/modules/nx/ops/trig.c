/* trig.c - trigonometric kernels
 *
 * @author: Cade Brown <cade@kscript.org>
 */
#include <ks/impl.h>
#include <ks/nx.h>
#include <ks/nxt.h>
#include <ks/nxm.h>


#define LOOPC(TYPE, NAME) \
TYPE TYPE##sqrt(TYPE x) { \
    if (x.re == 0 && x.im == 0) { \
        return x; \
    } else { \
        TYPE r; \
        TYPE##r ar = TYPE##rfabs(x.re) / 8, ai = TYPE##rfabs(x.im); \
        TYPE##r s = 2 * TYPE##rsqrt(ar + TYPE##rhypot(ar, ai / 8)); \
        TYPE##r d = ai / (2 * s); \
        if (x.re >= 0.0) { \
            r.re = s; \
            r.im = TYPE##rcopysign(d, x.im); \
        } else { \
            r.re = d; \
            r.im = TYPE##rcopysign(s, x.im); \
        } \
        return r; \
    } \
} \
TYPE TYPE##sinh(TYPE x) { \
    TYPE##r shxr = TYPE##rsinh(x.re), chxr = TYPE##rcosh(x.re); \
    TYPE##r sxi = TYPE##rsin(x.im), cxi = TYPE##rcos(x.im); \
    TYPE r;\
    r.re = cxi * shxr; \
    r.im = sxi * chxr; \
    return r; \
} \
TYPE TYPE##cosh(TYPE x) { \
    TYPE##r shxr = TYPE##rsinh(x.re), chxr = TYPE##rcosh(x.re); \
    TYPE##r sxi = TYPE##rsin(x.im), cxi = TYPE##rcos(x.im); \
    TYPE r;\
    r.re = cxi * chxr; \
    r.im = sxi * shxr; \
    return r; \
} \
TYPE TYPE##tanh(TYPE x) { \
    TYPE##r thxr = TYPE##rtanh(x.re), txi = TYPE##rtan(x.im); \
    TYPE##r cr = 1 / TYPE##rcosh(x.re), m_tht = thxr * txi; \
    TYPE##r den = 1 + m_tht * m_tht; \
    TYPE r;\
    r.re = thxr * (1.0 + txi * txi) / den; \
    r.im = (txi / den) * cr * cr; \
    return r; \
} \
TYPE TYPE##asinh(TYPE x) { \
    TYPE t0, t1; \
    t0.re = 1 + x.im; t0.im = -x.re; \
    t0 = TYPE##sqrt(t0); \
    t1.re = 1 - x.im; t1.im = +x.re; \
    t1 = TYPE##sqrt(t1); \
    TYPE r;\
    r.re = TYPE##rasinh(t0.re*t1.im - t1.re*t0.im); \
    r.im = TYPE##ratan2(x.im, t0.re*t1.re - t0.im*t1.im); \
    return r; \
} \
TYPE TYPE##acosh(TYPE x) { \
    TYPE t0, t1; \
    t0.re = x.re - 1; t0.im = x.im; \
    t0 = TYPE##sqrt(t0); \
    t1.re = x.re + 1; t1.im = x.im; \
    t1 = TYPE##sqrt(t1); \
    TYPE r;\
    r.re = TYPE##rasinh(t0.re*t1.re + t0.im*t1.im); \
    r.im = 2 * TYPE##ratan2(t1.re, t0.re); \
    return r; \
} \
TYPE TYPE##atanh(TYPE x) { \
    if (x.re < 0) { \
        x.re = -x.re; \
        x.im = -x.im; \
        return TYPE##atanh(x); \
    } else { \
        TYPE##r ai = TYPE##rfabs(x.im); \
        TYPE r; \
        if (x.re == 1 && ai < TYPE##rMIN) { \
            if (ai == 0) { \
                r.re = TYPE##rINF; \
                r.im = x.im; \
            } else { \
                r.re = TYPE##rlog(TYPE##rsqrt(ai) / TYPE##rsqrt(TYPE##rhypot(ai, 2))); \
                r.im = TYPE##rcopysign(TYPE##ratan2(2, -ai) / 2, x.im); \
            } \
        } else { \
            r.re = TYPE##rlog1p(4 * x.re / ((1-x.re)*(1-x.re) + ai*ai)) / 4; \
            r.im = -TYPE##ratan2(-2 * x.im, (1-x.re)*(1+x.re) - ai*ai) / 2; \
        } \
        return r; \
    } \
} \
TYPE TYPE##sin(TYPE x) { \
    TYPE t; \
    t.re = -x.im; \
    t.im = x.re; \
    t = TYPE##sinh(t); \
    TYPE r; \
    r.re = t.im; \
    r.im = -t.re; \
    return r; \
} \
TYPE TYPE##cos(TYPE x) { \
    TYPE t; \
    t.re = -x.im; \
    t.im = x.re; \
    return TYPE##cosh(t); \
} \
TYPE TYPE##tan(TYPE x) { \
    TYPE t; \
    t.re = -x.im; \
    t.im = x.re; \
    t = TYPE##tanh(t); \
    TYPE r; \
    r.re = t.im; \
    r.im = -t.re; \
    return r; \
} \
TYPE TYPE##asin(TYPE x) { \
    TYPE t; \
    t.re = -x.im; \
    t.im = x.re; \
    t = TYPE##asinh(t); \
    TYPE r; \
    r.re = t.im; \
    r.im = -t.re; \
    return r; \
} \
TYPE TYPE##acos(TYPE x) { \
    TYPE t0; \
    t0.re = 1 - x.re; \
    t0.im = -x.im; \
    TYPE t1; \
    t1.re = 1 + x.re; \
    t1.im = +x.im; \
    TYPE r; \
    r.re = 2 * TYPE##ratan2(t0.re, t1.re); \
    r.im = TYPE##rasinh(t1.re*t0.im - t1.im*t0.re); \
    return r; \
} \
TYPE TYPE##atan(TYPE x) { \
    TYPE t; \
    t.re = -x.im; \
    t.im = x.re; \
    t = TYPE##atanh(t); \
    TYPE r; \
    r.re = t.im; \
    r.im = -t.re; \
    return r; \
} \

NXT_PASTE_C(LOOPC);
#undef LOOPC


#define K_NAME "sin"

#define LOOPI(TYPE, NAME) static int kern_sin_##NAME(int N, nx_t* args, int len, void* extra) { \
    assert(N == 2); \
    nx_t X = args[0], R = args[1]; \
    KS_THROW(kst_TypeError, "Unsupported types for kernel '%s': %R, %R", K_NAME, X.dtype, R.dtype); \
    return 1; \
}

#define LOOPR(TYPE, NAME) static int kern_sin_##NAME(int N, nx_t* args, int len, void* extra) { \
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
        *(TYPE*)pR = TYPE##sin(*(TYPE*)pX); \
    } \
    return 0; \
}

NXT_PASTE_I(LOOPI);
NXT_PASTE_F(LOOPR);
NXT_PASTE_C(LOOPR);

bool nx_sin(nx_t X, nx_t R) {
    nx_t cX;
    void *fX = NULL;
    if (!nx_getcast(X, R.dtype, &cX, &fX)) {
        return false;
    }

    #define LOOP(NAME) do { \
        bool res = !nx_apply_elem(kern_sin_##NAME, 2, (nx_t[]){ cX, R }, NULL); \
        ks_free(fX); \
        return res; \
    } while (0);

    NXT_FOR_ALL(R.dtype, LOOP);
    #undef LOOP

    ks_free(fX);

    KS_THROW(kst_TypeError, "Unsupported types for kernel '%s': %R, %R", K_NAME, X.dtype, R.dtype);
    return false;
}

#undef K_NAME
#undef LOOPI
#undef LOOPR


#define K_NAME "cos"

#define LOOPI(TYPE, NAME) static int kern_cos_##NAME(int N, nx_t* args, int len, void* extra) { \
    assert(N == 2); \
    nx_t X = args[0], R = args[1]; \
    KS_THROW(kst_TypeError, "Unsupported types for kernel '%s': %R, %R", K_NAME, X.dtype, R.dtype); \
    return 1; \
}

#define LOOPR(TYPE, NAME) static int kern_cos_##NAME(int N, nx_t* args, int len, void* extra) { \
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
        *(TYPE*)pR = TYPE##cos(*(TYPE*)pX); \
    } \
    return 0; \
}

NXT_PASTE_I(LOOPI);
NXT_PASTE_F(LOOPR);
NXT_PASTE_C(LOOPR);

bool nx_cos(nx_t X, nx_t R) {
    nx_t cX;
    void *fX = NULL;
    if (!nx_getcast(X, R.dtype, &cX, &fX)) {
        return false;
    }

    #define LOOP(NAME) do { \
        bool res = !nx_apply_elem(kern_cos_##NAME, 2, (nx_t[]){ cX, R }, NULL); \
        ks_free(fX); \
        return res; \
    } while (0);

    NXT_FOR_ALL(R.dtype, LOOP);
    #undef LOOP

    ks_free(fX);

    KS_THROW(kst_TypeError, "Unsupported types for kernel '%s': %R, %R", K_NAME, X.dtype, R.dtype);
    return false;
}

#undef K_NAME
#undef LOOPI
#undef LOOPR


#define K_NAME "tan"

#define LOOPI(TYPE, NAME) static int kern_tan_##NAME(int N, nx_t* args, int len, void* extra) { \
    assert(N == 2); \
    nx_t X = args[0], R = args[1]; \
    KS_THROW(kst_TypeError, "Unsupported types for kernel '%s': %R, %R", K_NAME, X.dtype, R.dtype); \
    return 1; \
}

#define LOOPR(TYPE, NAME) static int kern_tan_##NAME(int N, nx_t* args, int len, void* extra) { \
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
        *(TYPE*)pR = TYPE##tan(*(TYPE*)pX); \
    } \
    return 0; \
}

NXT_PASTE_I(LOOPI);
NXT_PASTE_F(LOOPR);
NXT_PASTE_C(LOOPR);

bool nx_tan(nx_t X, nx_t R) {
    nx_t cX;
    void *fX = NULL;
    if (!nx_getcast(X, R.dtype, &cX, &fX)) {
        return false;
    }

    #define LOOP(NAME) do { \
        bool res = !nx_apply_elem(kern_tan_##NAME, 2, (nx_t[]){ cX, R }, NULL); \
        ks_free(fX); \
        return res; \
    } while (0);

    NXT_FOR_ALL(R.dtype, LOOP);
    #undef LOOP

    ks_free(fX);

    KS_THROW(kst_TypeError, "Unsupported types for kernel '%s': %R, %R", K_NAME, X.dtype, R.dtype);
    return false;
}

#undef K_NAME
#undef LOOPI
#undef LOOPR



#define K_NAME "asin"

#define LOOPI(TYPE, NAME) static int kern_asin_##NAME(int N, nx_t* args, int len, void* extra) { \
    assert(N == 2); \
    nx_t X = args[0], R = args[1]; \
    KS_THROW(kst_TypeError, "Unsupported types for kernel '%s': %R, %R", K_NAME, X.dtype, R.dtype); \
    return 1; \
}

#define LOOPR(TYPE, NAME) static int kern_asin_##NAME(int N, nx_t* args, int len, void* extra) { \
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
        *(TYPE*)pR = TYPE##asin(*(TYPE*)pX); \
    } \
    return 0; \
}

NXT_PASTE_I(LOOPI);
NXT_PASTE_F(LOOPR);
NXT_PASTE_C(LOOPR);

bool nx_asin(nx_t X, nx_t R) {
    nx_t cX;
    void *fX = NULL;
    if (!nx_getcast(X, R.dtype, &cX, &fX)) {
        return false;
    }

    #define LOOP(NAME) do { \
        bool res = !nx_apply_elem(kern_asin_##NAME, 2, (nx_t[]){ cX, R }, NULL); \
        ks_free(fX); \
        return res; \
    } while (0);

    NXT_FOR_ALL(R.dtype, LOOP);
    #undef LOOP

    ks_free(fX);

    KS_THROW(kst_TypeError, "Unsupported types for kernel '%s': %R, %R", K_NAME, X.dtype, R.dtype);
    return false;
}

#undef K_NAME
#undef LOOPI
#undef LOOPR


#define K_NAME "acos"

#define LOOPI(TYPE, NAME) static int kern_acos_##NAME(int N, nx_t* args, int len, void* extra) { \
    assert(N == 2); \
    nx_t X = args[0], R = args[1]; \
    KS_THROW(kst_TypeError, "Unsupported types for kernel '%s': %R, %R", K_NAME, X.dtype, R.dtype); \
    return 1; \
}

#define LOOPR(TYPE, NAME) static int kern_acos_##NAME(int N, nx_t* args, int len, void* extra) { \
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
        *(TYPE*)pR = TYPE##acos(*(TYPE*)pX); \
    } \
    return 0; \
}

NXT_PASTE_I(LOOPI);
NXT_PASTE_F(LOOPR);
NXT_PASTE_C(LOOPR);

bool nx_acos(nx_t X, nx_t R) {
    nx_t cX;
    void *fX = NULL;
    if (!nx_getcast(X, R.dtype, &cX, &fX)) {
        return false;
    }

    #define LOOP(NAME) do { \
        bool res = !nx_apply_elem(kern_acos_##NAME, 2, (nx_t[]){ cX, R }, NULL); \
        ks_free(fX); \
        return res; \
    } while (0);

    NXT_FOR_ALL(R.dtype, LOOP);
    #undef LOOP

    ks_free(fX);

    KS_THROW(kst_TypeError, "Unsupported types for kernel '%s': %R, %R", K_NAME, X.dtype, R.dtype);
    return false;
}

#undef K_NAME
#undef LOOPI
#undef LOOPR


#define K_NAME "atan"

#define LOOPI(TYPE, NAME) static int kern_atan_##NAME(int N, nx_t* args, int len, void* extra) { \
    assert(N == 2); \
    nx_t X = args[0], R = args[1]; \
    KS_THROW(kst_TypeError, "Unsupported types for kernel '%s': %R, %R", K_NAME, X.dtype, R.dtype); \
    return 1; \
}

#define LOOPR(TYPE, NAME) static int kern_atan_##NAME(int N, nx_t* args, int len, void* extra) { \
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
        *(TYPE*)pR = TYPE##atan(*(TYPE*)pX); \
    } \
    return 0; \
}

NXT_PASTE_I(LOOPI);
NXT_PASTE_F(LOOPR);
NXT_PASTE_C(LOOPR);

bool nx_atan(nx_t X, nx_t R) {
    nx_t cX;
    void *fX = NULL;
    if (!nx_getcast(X, R.dtype, &cX, &fX)) {
        return false;
    }

    #define LOOP(NAME) do { \
        bool res = !nx_apply_elem(kern_atan_##NAME, 2, (nx_t[]){ cX, R }, NULL); \
        ks_free(fX); \
        return res; \
    } while (0);

    NXT_FOR_ALL(R.dtype, LOOP);
    #undef LOOP

    ks_free(fX);

    KS_THROW(kst_TypeError, "Unsupported types for kernel '%s': %R, %R", K_NAME, X.dtype, R.dtype);
    return false;
}

#undef K_NAME
#undef LOOPI
#undef LOOPR




#define K_NAME "sinh"

#define LOOPI(TYPE, NAME) static int kern_sinh_##NAME(int N, nx_t* args, int len, void* extra) { \
    assert(N == 2); \
    nx_t X = args[0], R = args[1]; \
    KS_THROW(kst_TypeError, "Unsupported types for kernel '%s': %R, %R", K_NAME, X.dtype, R.dtype); \
    return 1; \
}

#define LOOPR(TYPE, NAME) static int kern_sinh_##NAME(int N, nx_t* args, int len, void* extra) { \
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
        *(TYPE*)pR = TYPE##sinh(*(TYPE*)pX); \
    } \
    return 0; \
}

NXT_PASTE_I(LOOPI);
NXT_PASTE_F(LOOPR);
NXT_PASTE_C(LOOPR);

bool nx_sinh(nx_t X, nx_t R) {
    nx_t cX;
    void *fX = NULL;
    if (!nx_getcast(X, R.dtype, &cX, &fX)) {
        return false;
    }

    #define LOOP(NAME) do { \
        bool res = !nx_apply_elem(kern_sinh_##NAME, 2, (nx_t[]){ cX, R }, NULL); \
        ks_free(fX); \
        return res; \
    } while (0);

    NXT_FOR_ALL(R.dtype, LOOP);
    #undef LOOP

    ks_free(fX);

    KS_THROW(kst_TypeError, "Unsupported types for kernel '%s': %R, %R", K_NAME, X.dtype, R.dtype);
    return false;
}

#undef K_NAME
#undef LOOPI
#undef LOOPR


#define K_NAME "cosh"

#define LOOPI(TYPE, NAME) static int kern_cosh_##NAME(int N, nx_t* args, int len, void* extra) { \
    assert(N == 2); \
    nx_t X = args[0], R = args[1]; \
    KS_THROW(kst_TypeError, "Unsupported types for kernel '%s': %R, %R", K_NAME, X.dtype, R.dtype); \
    return 1; \
}

#define LOOPR(TYPE, NAME) static int kern_cosh_##NAME(int N, nx_t* args, int len, void* extra) { \
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
        *(TYPE*)pR = TYPE##cosh(*(TYPE*)pX); \
    } \
    return 0; \
}

NXT_PASTE_I(LOOPI);
NXT_PASTE_F(LOOPR);
NXT_PASTE_C(LOOPR);

bool nx_cosh(nx_t X, nx_t R) {
    nx_t cX;
    void *fX = NULL;
    if (!nx_getcast(X, R.dtype, &cX, &fX)) {
        return false;
    }

    #define LOOP(NAME) do { \
        bool res = !nx_apply_elem(kern_cosh_##NAME, 2, (nx_t[]){ cX, R }, NULL); \
        ks_free(fX); \
        return res; \
    } while (0);

    NXT_FOR_ALL(R.dtype, LOOP);
    #undef LOOP

    ks_free(fX);

    KS_THROW(kst_TypeError, "Unsupported types for kernel '%s': %R, %R", K_NAME, X.dtype, R.dtype);
    return false;
}

#undef K_NAME
#undef LOOPI
#undef LOOPR


#define K_NAME "tanh"

#define LOOPI(TYPE, NAME) static int kern_tanh_##NAME(int N, nx_t* args, int len, void* extra) { \
    assert(N == 2); \
    nx_t X = args[0], R = args[1]; \
    KS_THROW(kst_TypeError, "Unsupported types for kernel '%s': %R, %R", K_NAME, X.dtype, R.dtype); \
    return 1; \
}

#define LOOPR(TYPE, NAME) static int kern_tanh_##NAME(int N, nx_t* args, int len, void* extra) { \
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
        *(TYPE*)pR = TYPE##tanh(*(TYPE*)pX); \
    } \
    return 0; \
}

NXT_PASTE_I(LOOPI);
NXT_PASTE_F(LOOPR);
NXT_PASTE_C(LOOPR);

bool nx_tanh(nx_t X, nx_t R) {
    nx_t cX;
    void *fX = NULL;
    if (!nx_getcast(X, R.dtype, &cX, &fX)) {
        return false;
    }

    #define LOOP(NAME) do { \
        bool res = !nx_apply_elem(kern_tanh_##NAME, 2, (nx_t[]){ cX, R }, NULL); \
        ks_free(fX); \
        return res; \
    } while (0);

    NXT_FOR_ALL(R.dtype, LOOP);
    #undef LOOP

    ks_free(fX);

    KS_THROW(kst_TypeError, "Unsupported types for kernel '%s': %R, %R", K_NAME, X.dtype, R.dtype);
    return false;
}

#undef K_NAME
#undef LOOPI
#undef LOOPR



#define K_NAME "asinh"

#define LOOPI(TYPE, NAME) static int kern_asinh_##NAME(int N, nx_t* args, int len, void* extra) { \
    assert(N == 2); \
    nx_t X = args[0], R = args[1]; \
    KS_THROW(kst_TypeError, "Unsupported types for kernel '%s': %R, %R", K_NAME, X.dtype, R.dtype); \
    return 1; \
}

#define LOOPR(TYPE, NAME) static int kern_asinh_##NAME(int N, nx_t* args, int len, void* extra) { \
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
        *(TYPE*)pR = TYPE##asinh(*(TYPE*)pX); \
    } \
    return 0; \
}

NXT_PASTE_I(LOOPI);
NXT_PASTE_F(LOOPR);
NXT_PASTE_C(LOOPR);

bool nx_asinh(nx_t X, nx_t R) {
    nx_t cX;
    void *fX = NULL;
    if (!nx_getcast(X, R.dtype, &cX, &fX)) {
        return false;
    }

    #define LOOP(NAME) do { \
        bool res = !nx_apply_elem(kern_asinh_##NAME, 2, (nx_t[]){ cX, R }, NULL); \
        ks_free(fX); \
        return res; \
    } while (0);

    NXT_FOR_ALL(R.dtype, LOOP);
    #undef LOOP

    ks_free(fX);

    KS_THROW(kst_TypeError, "Unsupported types for kernel '%s': %R, %R", K_NAME, X.dtype, R.dtype);
    return false;
}

#undef K_NAME
#undef LOOPI
#undef LOOPR


#define K_NAME "acosh"

#define LOOPI(TYPE, NAME) static int kern_acosh_##NAME(int N, nx_t* args, int len, void* extra) { \
    assert(N == 2); \
    nx_t X = args[0], R = args[1]; \
    KS_THROW(kst_TypeError, "Unsupported types for kernel '%s': %R, %R", K_NAME, X.dtype, R.dtype); \
    return 1; \
}

#define LOOPR(TYPE, NAME) static int kern_acosh_##NAME(int N, nx_t* args, int len, void* extra) { \
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
        *(TYPE*)pR = TYPE##acosh(*(TYPE*)pX); \
    } \
    return 0; \
}

NXT_PASTE_I(LOOPI);
NXT_PASTE_F(LOOPR);
NXT_PASTE_C(LOOPR);

bool nx_acosh(nx_t X, nx_t R) {
    nx_t cX;
    void *fX = NULL;
    if (!nx_getcast(X, R.dtype, &cX, &fX)) {
        return false;
    }

    #define LOOP(NAME) do { \
        bool res = !nx_apply_elem(kern_acosh_##NAME, 2, (nx_t[]){ cX, R }, NULL); \
        ks_free(fX); \
        return res; \
    } while (0);

    NXT_FOR_ALL(R.dtype, LOOP);
    #undef LOOP

    ks_free(fX);

    KS_THROW(kst_TypeError, "Unsupported types for kernel '%s': %R, %R", K_NAME, X.dtype, R.dtype);
    return false;
}

#undef K_NAME
#undef LOOPI
#undef LOOPR


#define K_NAME "atanh"

#define LOOPI(TYPE, NAME) static int kern_atanh_##NAME(int N, nx_t* args, int len, void* extra) { \
    assert(N == 2); \
    nx_t X = args[0], R = args[1]; \
    KS_THROW(kst_TypeError, "Unsupported types for kernel '%s': %R, %R", K_NAME, X.dtype, R.dtype); \
    return 1; \
}

#define LOOPR(TYPE, NAME) static int kern_atanh_##NAME(int N, nx_t* args, int len, void* extra) { \
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
        *(TYPE*)pR = TYPE##atanh(*(TYPE*)pX); \
    } \
    return 0; \
}

NXT_PASTE_I(LOOPI);
NXT_PASTE_F(LOOPR);
NXT_PASTE_C(LOOPR);

bool nx_atanh(nx_t X, nx_t R) {
    nx_t cX;
    void *fX = NULL;
    if (!nx_getcast(X, R.dtype, &cX, &fX)) {
        return false;
    }

    #define LOOP(NAME) do { \
        bool res = !nx_apply_elem(kern_atanh_##NAME, 2, (nx_t[]){ cX, R }, NULL); \
        ks_free(fX); \
        return res; \
    } while (0);

    NXT_FOR_ALL(R.dtype, LOOP);
    #undef LOOP

    ks_free(fX);

    KS_THROW(kst_TypeError, "Unsupported types for kernel '%s': %R, %R", K_NAME, X.dtype, R.dtype);
    return false;
}

#undef K_NAME
#undef LOOPI
#undef LOOPR

