/* fpcast.c - 'fpcast' kernel
 *
 * @author: Cade Brown <cade@kscript.org>
 */
#include <ks/impl.h>
#include <ks/nxt.h>

#define K_NAME "fpcast"


#define LOOPI(TYPE, NAME) static int KN(NAME)(int N, nx_t* args, int len, void* extra) { \
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
        *(RTYPE*)pR = nx_blv(*(TYPE*)pX); \
    } \
    return 0; \
}

#define LOOPF(TYPE, NAME) static int KN(NAME)(int N, nx_t* args, int len, void* extra) { \
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
        *(RTYPE*)pR = nx_blv(*(TYPE*)pX); \
    } \
    return 0; \
}

#define LOOPC(TYPE, NAME) static int KN(NAME)(int N, nx_t* args, int len, void* extra) { \
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
        *(RTYPE*)pR = nx_blv(((TYPE*)pX)->re) && nx_blv(((TYPE*)pX)->im); \
    } \
    return 0; \
}

#define RNAME bl
#define RTYPE nx_bl
#define RTYPE_MIN nx_blMIN
#define RTYPE_MAX nx_blMAX
#define KN(_X) kern_##_X##_bl
  NXT_PASTE_I(LOOPI)
  NXT_PASTE_F(LOOPF)
  NXT_PASTE_C(LOOPC)
#undef RNAME
#undef RTYPE
#undef RTYPE_MIN
#undef RTYPE_MAX
#undef KN

#undef LOOPI
#undef LOOPF
#undef LOOPC

#define LOOPI(TYPE, NAME) static int KN(NAME)(int N, nx_t* args, int len, void* extra) { \
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
    int sa = (int)sizeof(RTYPE) - (int)sizeof(TYPE); \
    ks_cint i; \
    for (i = 0; i < len; i++, pX += sX, pR += sR) { \
        *(RTYPE*)pR = sa > 0 ? (*(TYPE*)pX << sa) : (*(TYPE*)pX >> -sa); \
    } \
    return 0; \
}

#define LOOPF(TYPE, NAME) static int KN(NAME)(int N, nx_t* args, int len, void* extra) { \
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
    bool sgn = RTYPE_MIN < 0; \
    ks_cint i; \
    for (i = 0; i < len; i++, pX += sX, pR += sR) { \
        *(RTYPE*)pR = sgn ? \
          (TYPE)RTYPE_MAX * *(TYPE*)pX \
          : (*(TYPE*)pX * (TYPE)RTYPE_MAX); \
    } \
    return 0; \
}

#define LOOPC(TYPE, NAME) static int KN(NAME)(int N, nx_t* args, int len, void* extra) { \
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
    bool sgn = RTYPE_MIN < 0; \
    ks_cint i; \
    for (i = 0; i < len; i++, pX += sX, pR += sR) { \
        *(RTYPE*)pR = sgn ? \
          (((TYPE##r)RTYPE_MAX - (TYPE##r)RTYPE_MIN) * (((TYPE*)pX)->re + 1) / 2 + (TYPE##r)RTYPE_MIN) \
          : (((TYPE*)pX)->re * (TYPE##r)RTYPE_MAX); \
    } \
    return 0; \
}

#define RNAME u8
#define RTYPE nx_u8
#define RTYPE_MIN nx_u8MIN
#define RTYPE_MAX nx_u8MAX
#define KN(_X) kern_##_X##_u8
  NXT_PASTE_I(LOOPI)
  NXT_PASTE_F(LOOPF)
  NXT_PASTE_C(LOOPC)
#undef RNAME
#undef RTYPE
#undef RTYPE_MIN
#undef RTYPE_MAX
#undef KN

#define RNAME s8
#define RTYPE nx_s8
#define RTYPE_MIN nx_s8MIN
#define RTYPE_MAX nx_s8MAX
#define KN(_X) kern_##_X##_s8
  NXT_PASTE_I(LOOPI)
  NXT_PASTE_F(LOOPF)
  NXT_PASTE_C(LOOPC)
#undef RNAME
#undef RTYPE
#undef RTYPE_MIN
#undef RTYPE_MAX
#undef KN

#define RNAME u16
#define RTYPE nx_u16
#define RTYPE_MIN nx_u16MIN
#define RTYPE_MAX nx_u16MAX
#define KN(_X) kern_##_X##_u16
  NXT_PASTE_I(LOOPI)
  NXT_PASTE_F(LOOPF)
  NXT_PASTE_C(LOOPC)
#undef RNAME
#undef RTYPE
#undef RTYPE_MIN
#undef RTYPE_MAX
#undef KN

#define RNAME s16
#define RTYPE nx_s16
#define RTYPE_MIN nx_s16MIN
#define RTYPE_MAX nx_s16MAX
#define KN(_X) kern_##_X##_s16
  NXT_PASTE_I(LOOPI)
  NXT_PASTE_F(LOOPF)
  NXT_PASTE_C(LOOPC)
#undef RNAME
#undef RTYPE
#undef RTYPE_MIN
#undef RTYPE_MAX
#undef KN

#define RNAME u32
#define RTYPE nx_u32
#define RTYPE_MIN nx_u32MIN
#define RTYPE_MAX nx_u32MAX
#define KN(_X) kern_##_X##_u32
  NXT_PASTE_I(LOOPI)
  NXT_PASTE_F(LOOPF)
  NXT_PASTE_C(LOOPC)
#undef RNAME
#undef RTYPE
#undef RTYPE_MIN
#undef RTYPE_MAX
#undef KN

#define RNAME s32
#define RTYPE nx_s32
#define RTYPE_MIN nx_s32MIN
#define RTYPE_MAX nx_s32MAX
#define KN(_X) kern_##_X##_s32
  NXT_PASTE_I(LOOPI)
  NXT_PASTE_F(LOOPF)
  NXT_PASTE_C(LOOPC)
#undef RNAME
#undef RTYPE
#undef RTYPE_MIN
#undef RTYPE_MAX
#undef KN

#define RNAME u64
#define RTYPE nx_u64
#define RTYPE_MIN nx_u64MIN
#define RTYPE_MAX nx_u64MAX
#define KN(_X) kern_##_X##_u64
  NXT_PASTE_I(LOOPI)
  NXT_PASTE_F(LOOPF)
  NXT_PASTE_C(LOOPC)
#undef RNAME
#undef RTYPE
#undef RTYPE_MIN
#undef RTYPE_MAX
#undef KN

#define RNAME s64
#define RTYPE nx_s64
#define RTYPE_MIN nx_s64MIN
#define RTYPE_MAX nx_s64MAX
#define KN(_X) kern_##_X##_s64
  NXT_PASTE_I(LOOPI)
  NXT_PASTE_F(LOOPF)
  NXT_PASTE_C(LOOPC)
#undef RNAME
#undef RTYPE
#undef RTYPE_MIN
#undef RTYPE_MAX
#undef KN

#undef LOOPI
#undef LOOPF
#undef LOOPC


/* Float Types */

#define LOOPI(TYPE, NAME) static int KN(NAME)(int N, nx_t* args, int len, void* extra) { \
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
    bool sgn = TYPE##MIN < 0; \
    ks_cint i; \
    for (i = 0; i < len; i++, pX += sX, pR += sR) { \
        *(RTYPE*)pR = sgn ? \
          (*(TYPE*)pX + (RTYPE)TYPE##MAX) / ((RTYPE)TYPE##MAX) - 1 \
          : (*(TYPE*)pX / (RTYPE)TYPE##MAX); \
    } \
    return 0; \
}


#define LOOPF(TYPE, NAME) static int KN(NAME)(int N, nx_t* args, int len, void* extra) { \
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
    bool sgn = RTYPE_MIN < 0; \
    ks_cint i; \
    for (i = 0; i < len; i++, pX += sX, pR += sR) { \
        *(RTYPE*)pR = *(RTYPE*)pX; \
    } \
    return 0; \
}

#define LOOPC(TYPE, NAME) static int KN(NAME)(int N, nx_t* args, int len, void* extra) { \
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
    bool sgn = RTYPE_MIN < 0; \
    ks_cint i; \
    for (i = 0; i < len; i++, pX += sX, pR += sR) { \
        *(RTYPE*)pR = *(RTYPE*)pX; \
    } \
    return 0; \
}

#define RNAME H
#define RTYPE nx_H
#define RTYPE_MIN nx_HMIN
#define RTYPE_MAX nx_HMAX
#define KN(_X) kern_##_X##_H
  NXT_PASTE_I(LOOPI)
  NXT_PASTE_F(LOOPF)
  NXT_PASTE_C(LOOPC)
#undef RNAME
#undef RTYPE
#undef RTYPE_MIN
#undef RTYPE_MAX
#undef KN

#define RNAME F
#define RTYPE nx_F
#define RTYPE_MIN nx_FMIN
#define RTYPE_MAX nx_FMAX
#define KN(_X) kern_##_X##_F
  NXT_PASTE_I(LOOPI)
  NXT_PASTE_F(LOOPF)
  NXT_PASTE_C(LOOPC)
#undef RNAME
#undef RTYPE
#undef RTYPE_MIN
#undef RTYPE_MAX
#undef KN

#define RNAME D
#define RTYPE nx_D
#define RTYPE_MIN nx_DMIN
#define RTYPE_MAX nx_DMAX
#define KN(_X) kern_##_X##_D
  NXT_PASTE_I(LOOPI)
  NXT_PASTE_F(LOOPF)
  NXT_PASTE_C(LOOPC)
#undef RNAME
#undef RTYPE
#undef RTYPE_MIN
#undef RTYPE_MAX
#undef KN

#define RNAME L
#define RTYPE nx_L
#define RTYPE_MIN nx_LMIN
#define RTYPE_MAX nx_LMAX
#define KN(_X) kern_##_X##_L
  NXT_PASTE_I(LOOPI)
  NXT_PASTE_F(LOOPF)
  NXT_PASTE_C(LOOPC)
#undef RNAME
#undef RTYPE
#undef RTYPE_MIN
#undef RTYPE_MAX
#undef KN

#define RNAME E
#define RTYPE nx_E
#define RTYPE_MIN nx_EMIN
#define RTYPE_MAX nx_EMAX
#define KN(_X) kern_##_X##_E
  NXT_PASTE_I(LOOPI)
  NXT_PASTE_F(LOOPF)
  NXT_PASTE_C(LOOPC)
#undef RNAME
#undef RTYPE
#undef RTYPE_MIN
#undef RTYPE_MAX
#undef KN

#undef LOOPI
#undef LOOPF
#undef LOOPC


/* Complex Types */


#define LOOPI(TYPE, NAME) static int KN(NAME)(int N, nx_t* args, int len, void* extra) { \
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
    bool sgn = TYPE##MIN < 0; \
    RTYPE##r sa = (RTYPE##r)TYPE##MAX - TYPE##MIN, sb = TYPE##MIN; \
    ks_cint i; \
    for (i = 0; i < len; i++, pX += sX, pR += sR) { \
        ((RTYPE*)pR)->re = sgn ? (2 * (RTYPE##r)(*(TYPE*)pX - sb / 2) / sa) : ((RTYPE##r)(*(TYPE*)pX - sb) / sa); \
        ((RTYPE*)pR)->im = 0; \
    } \
    return 0; \
}

#define LOOPF(TYPE, NAME) static int KN(NAME)(int N, nx_t* args, int len, void* extra) { \
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
    bool sgn = RTYPE_MIN < 0; \
    ks_cint i; \
    for (i = 0; i < len; i++, pX += sX, pR += sR) { \
        ((RTYPE*)pR)->re = *(TYPE*)pX; \
        ((RTYPE*)pR)->im = 0; \
    } \
    return 0; \
}

#define LOOPC(TYPE, NAME) static int KN(NAME)(int N, nx_t* args, int len, void* extra) { \
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
    bool sgn = RTYPE_MIN < 0; \
    ks_cint i; \
    for (i = 0; i < len; i++, pX += sX, pR += sR) { \
        *(RTYPE*)pR = *(RTYPE*)pX; \
    } \
    return 0; \
}

#define RNAME cH
#define RTYPE nx_cH
#define RTYPEr nx_H
#define RTYPE_MIN nx_cHrMIN
#define RTYPE_MAX nx_cHrMAX
#define KN(_X) kern_##_X##_cH
  NXT_PASTE_I(LOOPI)
  NXT_PASTE_F(LOOPF)
  NXT_PASTE_C(LOOPC)
#undef RNAME
#undef RTYPE
#undef RTYPEr
#undef RTYPE_MIN
#undef RTYPE_MAX
#undef KN

#define RNAME cF
#define RTYPE nx_cF
#define RTYPEr nx_F
#define RTYPE_MIN nx_cFrMIN
#define RTYPE_MAX nx_cFrMAX
#define KN(_X) kern_##_X##_cF
  NXT_PASTE_I(LOOPI)
  NXT_PASTE_F(LOOPF)
  NXT_PASTE_C(LOOPC)
#undef RNAME
#undef RTYPE
#undef RTYPEr
#undef RTYPE_MIN
#undef RTYPE_MAX
#undef KN

#define RNAME cD
#define RTYPE nx_cD
#define RTYPEr nx_D
#define RTYPE_MIN nx_cDrMIN
#define RTYPE_MAX nx_cDrMAX
#define KN(_X) kern_##_X##_cD
  NXT_PASTE_I(LOOPI)
  NXT_PASTE_F(LOOPF)
  NXT_PASTE_C(LOOPC)
#undef RNAME
#undef RTYPE
#undef RTYPEr
#undef RTYPE_MIN
#undef RTYPE_MAX
#undef KN

#define RNAME cL
#define RTYPE nx_cL
#define RTYPEr nx_L
#define RTYPE_MIN nx_cLrMIN
#define RTYPE_MAX nx_cLrMAX
#define KN(_X) kern_##_X##_cL
  NXT_PASTE_I(LOOPI)
  NXT_PASTE_F(LOOPF)
  NXT_PASTE_C(LOOPC)
#undef RNAME
#undef RTYPE
#undef RTYPEr
#undef RTYPE_MIN
#undef RTYPE_MAX
#undef KN

#define RNAME cE
#define RTYPE nx_cE
#define RTYPEr nx_E
#define RTYPE_MIN nx_cErMIN
#define RTYPE_MAX nx_cErMAX
#define KN(_X) kern_##_X##_cE
  NXT_PASTE_I(LOOPI)
  NXT_PASTE_F(LOOPF)
  NXT_PASTE_C(LOOPC)
#undef RNAME
#undef RTYPE
#undef RTYPEr
#undef RTYPE_MIN
#undef RTYPE_MAX
#undef KN

#undef LOOPI
#undef LOOPF
#undef LOOPC

bool nx_fpcast(nx_t X, nx_t R) {
    #define LOOP(_X, _R) do { \
        return !nx_apply_elem(kern_##_X##_##_R, 2, (nx_t[]){ X, R }, NULL); \
    } while (0);

    NXT_PASTE_ALL2(X.dtype, R.dtype, LOOP);
    #undef LOOP

    KS_THROW(kst_TypeError, "Unsupported types for kernel '%s': %R, %R", K_NAME, X.dtype, R.dtype);
    return false;
}
