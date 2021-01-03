/* cast.c - 'cast' kernel
 *
 * @author: Cade Brown <cade@kscript.org>
 */
#include <ks/impl.h>
#include <ks/nx.h>
#include <ks/nxt.h>

static int kern(int N, nxar_t* inp, int len, void* _data) {
    assert(N == 2);

    ks_cint i;
    nx_dtype dR = inp[0].dtype, dX = inp[1].dtype;
    ks_uint pR = (ks_uint)inp[0].data, pX = (ks_uint)inp[1].data;
    ks_cint sR = inp[0].strides[0], sX = inp[1].strides[0];


#define LOOP(TYPE) do { \
    for (i = 0; i < len; i++, pR += sR, pX += sX) { \
        *(ATYPE*)pR = *(TYPE*)pX; \
    } \
    return 0; \
} while (0);

#define LOOPC(TYPE) do { \
    for (i = 0; i < len; i++, pR += sR, pX += sX) { \
        *(ATYPE*)pR = ((TYPE*)pX)->re; \
    } \
    return 0; \
} while (0);

#define CASE_NUM(_dt) else if (dR == _dt) { \
    NXT_DO_INTS(dX, LOOP); \
    NXT_DO_FLOATS(dX, LOOP); \
    NXT_DO_COMPLEXS(dX, LOOPC); \
}

    if (false) {}
#define ATYPE nxc_uchar
    CASE_NUM(nxd_uchar)
#undef ATYPE
#define ATYPE nxc_schar
    CASE_NUM(nxd_schar)
#undef ATYPE
#define ATYPE nxc_ushort
    CASE_NUM(nxd_ushort)
#undef ATYPE
#define ATYPE nxc_sshort
    CASE_NUM(nxd_sshort)
#undef ATYPE
#define ATYPE nxc_uint
    CASE_NUM(nxd_uint)
#undef ATYPE
#define ATYPE nxc_sint
    CASE_NUM(nxd_sint)
#undef ATYPE
#define ATYPE nxc_ulong
    CASE_NUM(nxd_ulong)
#undef ATYPE
#define ATYPE nxc_slong
    CASE_NUM(nxd_slong)
#undef ATYPE



#define ATYPE nxc_float
    CASE_NUM(nxd_float)
#undef ATYPE
#define ATYPE nxc_double
    CASE_NUM(nxd_double)
#undef ATYPE
#define ATYPE nxc_longdouble
    CASE_NUM(nxd_longdouble)
#undef ATYPE
#define ATYPE nxc_float128
    CASE_NUM(nxd_float128)
#undef ATYPE


#undef LOOP
#undef LOOPC
#undef CASE_NUM

#define LOOP(TYPE) do { \
    for (i = 0; i < len; i++, pR += sR, pX += sX) { \
        ((ATYPE*)pR)->re = *(TYPE*)pX; \
        ((ATYPE*)pR)->im = 0; \
    } \
    return 0; \
} while (0);

#define LOOPC(TYPE) do { \
    for (i = 0; i < len; i++, pR += sR, pX += sX) { \
        ((ATYPE*)pR)->re = ((TYPE*)pX)->re; \
        ((ATYPE*)pR)->im = ((TYPE*)pX)->im; \
    } \
    return 0; \
} while (0);

#define CASE_NUM(_dt) else if (dR == _dt) { \
    NXT_DO_INTS(dX, LOOP); \
    NXT_DO_FLOATS(dX, LOOP); \
    NXT_DO_COMPLEXS(dX, LOOPC); \
}

#define ATYPE nxc_complexfloat
    CASE_NUM(nxd_complexfloat)
#undef ATYPE
#define ATYPE nxc_complexdouble
    CASE_NUM(nxd_complexdouble)
#undef ATYPE
#define ATYPE nxc_complexlongdouble
    CASE_NUM(nxd_complexlongdouble)
#undef ATYPE
#define ATYPE nxc_complexfloat128
    CASE_NUM(nxd_complexfloat128)
#undef ATYPE


    return 1;
}

bool nx_cast(nxar_t r, nxar_t x) {
    return !nx_apply_elem(kern, 2, (nxar_t[]){ r, x }, NULL);
}
