/* cast.c - 'cast' kernel
 *
 * @author: Cade Brown <cade@kscript.org>
 */
#include <ks/impl.h>
#include <ks/nxt.h>

#define K_NAME "cast"



#define LOOPR(TYPE, NAME) static int KN(NAME)(int N, nx_t* args, int len, void* extra) { \
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
#define KN(_X) kern_##_X##_bl
  NXT_PASTE_I(LOOPR)
  NXT_PASTE_F(LOOPR)
  NXT_PASTE_C(LOOPC)
#undef RNAME
#undef RTYPE
#undef KN

#undef LOOPR
#undef LOOPC


#define LOOPR(TYPE, NAME) static int KN(NAME)(int N, nx_t* args, int len, void* extra) { \
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
        *(RTYPE*)pR = *(TYPE*)pX; \
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
        *(RTYPE*)pR = ((TYPE*)pX)->re; \
    } \
    return 0; \
}

#define RNAME u8
#define RTYPE nx_u8
#define KN(_X) kern_##_X##_u8
  NXT_PASTE_I(LOOPR)
  NXT_PASTE_F(LOOPR)
  NXT_PASTE_C(LOOPC)
#undef RNAME
#undef RTYPE
#undef KN

#define RNAME s8
#define RTYPE nx_s8
#define KN(_X) kern_##_X##_s8
  NXT_PASTE_I(LOOPR)
  NXT_PASTE_F(LOOPR)
  NXT_PASTE_C(LOOPC)
#undef RNAME
#undef RTYPE
#undef KN

#define RNAME u16
#define RTYPE nx_u16
#define KN(_X) kern_##_X##_u16
  NXT_PASTE_I(LOOPR)
  NXT_PASTE_F(LOOPR)
  NXT_PASTE_C(LOOPC)
#undef RNAME
#undef RTYPE
#undef KN

#define RNAME s16
#define RTYPE nx_s16
#define KN(_X) kern_##_X##_s16
  NXT_PASTE_I(LOOPR)
  NXT_PASTE_F(LOOPR)
  NXT_PASTE_C(LOOPC)
#undef RNAME
#undef RTYPE
#undef KN

#define RNAME u32
#define RTYPE nx_u32
#define KN(_X) kern_##_X##_u32
  NXT_PASTE_I(LOOPR)
  NXT_PASTE_F(LOOPR)
  NXT_PASTE_C(LOOPC)
#undef RNAME
#undef RTYPE
#undef KN

#define RNAME s32
#define RTYPE nx_s32
#define KN(_X) kern_##_X##_s32
  NXT_PASTE_I(LOOPR)
  NXT_PASTE_F(LOOPR)
  NXT_PASTE_C(LOOPC)
#undef RNAME
#undef RTYPE
#undef KN

#define RNAME u64
#define RTYPE nx_u64
#define KN(_X) kern_##_X##_u64
  NXT_PASTE_I(LOOPR)
  NXT_PASTE_F(LOOPR)
  NXT_PASTE_C(LOOPC)
#undef RNAME
#undef RTYPE
#undef KN

#define RNAME s64
#define RTYPE nx_s64
#define KN(_X) kern_##_X##_s64
  NXT_PASTE_I(LOOPR)
  NXT_PASTE_F(LOOPR)
  NXT_PASTE_C(LOOPC)
#undef RNAME
#undef RTYPE
#undef KN


#define RNAME H
#define RTYPE nx_H
#define KN(_X) kern_##_X##_H
  NXT_PASTE_I(LOOPR)
  NXT_PASTE_F(LOOPR)
  NXT_PASTE_C(LOOPC)
#undef RNAME
#undef RTYPE
#undef KN

#define RNAME F
#define RTYPE nx_F
#define KN(_X) kern_##_X##_F
  NXT_PASTE_I(LOOPR)
  NXT_PASTE_F(LOOPR)
  NXT_PASTE_C(LOOPC)
#undef RNAME
#undef RTYPE
#undef KN

#define RNAME D
#define RTYPE nx_D
#define KN(_X) kern_##_X##_D
  NXT_PASTE_I(LOOPR)
  NXT_PASTE_F(LOOPR)
  NXT_PASTE_C(LOOPC)
#undef RNAME
#undef RTYPE
#undef KN


#define RNAME L
#define RTYPE nx_L
#define KN(_X) kern_##_X##_L
  NXT_PASTE_I(LOOPR)
  NXT_PASTE_F(LOOPR)
  NXT_PASTE_C(LOOPC)
#undef RNAME
#undef RTYPE
#undef KN

#define RNAME Q
#define RTYPE nx_Q
#define KN(_X) kern_##_X##_Q
  NXT_PASTE_I(LOOPR)
  NXT_PASTE_F(LOOPR)
  NXT_PASTE_C(LOOPC)
#undef RNAME
#undef RTYPE
#undef KN

#undef LOOPR
#undef LOOPC


#define LOOPR(TYPE, NAME) static int KN(NAME)(int N, nx_t* args, int len, void* extra) { \
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
    ks_cint i; \
    for (i = 0; i < len; i++, pX += sX, pR += sR) { \
        ((RTYPE*)pR)->re = ((TYPE*)pX)->re; \
        ((RTYPE*)pR)->im = ((TYPE*)pX)->im; \
    } \
    return 0; \
}

#define RNAME cH
#define RTYPE nx_cH
#define KN(_X) kern_##_X##_cH
  NXT_PASTE_I(LOOPR)
  NXT_PASTE_F(LOOPR)
  NXT_PASTE_C(LOOPC)
#undef RNAME
#undef RTYPE
#undef KN

#define RNAME cF
#define RTYPE nx_cF
#define KN(_X) kern_##_X##_cF
  NXT_PASTE_I(LOOPR)
  NXT_PASTE_F(LOOPR)
  NXT_PASTE_C(LOOPC)
#undef RNAME
#undef RTYPE
#undef KN

#define RNAME cD
#define RTYPE nx_cD
#define KN(_X) kern_##_X##_cD
  NXT_PASTE_I(LOOPR)
  NXT_PASTE_F(LOOPR)
  NXT_PASTE_C(LOOPC)
#undef RNAME
#undef RTYPE
#undef KN

#define RNAME cL
#define RTYPE nx_cL
#define KN(_X) kern_##_X##_cL
  NXT_PASTE_I(LOOPR)
  NXT_PASTE_F(LOOPR)
  NXT_PASTE_C(LOOPC)
#undef RNAME
#undef RTYPE
#undef KN

#define RNAME cQ
#define RTYPE nx_cQ
#define KN(_X) kern_##_X##_cQ
  NXT_PASTE_I(LOOPR)
  NXT_PASTE_F(LOOPR)
  NXT_PASTE_C(LOOPC)
#undef RNAME
#undef RTYPE
#undef KN

#undef LOOPR
#undef LOOPC

bool nx_cast(nx_t X, nx_t R) {
    #define LOOP(_X, _R) do { \
        return !nx_apply_elem(kern_##_X##_##_R, 2, (nx_t[]){ X, R }, NULL); \
    } while (0);

    NXT_FOR_ALL2(X.dtype, R.dtype, LOOP);
    #undef LOOP

    KS_THROW(kst_TypeError, "Unsupported types for kernel '%s': %R, %R", K_NAME, X.dtype, R.dtype);
    return false;
}
