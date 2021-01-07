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
        *(RTYPE*)pR = *(TYPE*)pX; \
    } \
    return 0; \
} while (0);

#define LOOPC(TYPE) do { \
    for (i = 0; i < len; i++, pR += sR, pX += sX) { \
        *(RTYPE*)pR = ((TYPE*)pX)->re; \
    } \
    return 0; \
} while (0);

#define CASE_NUM(_dt) else if (dR == _dt) { \
    NXT_DO_INTS(dX, LOOP); \
    NXT_DO_FLOATS(dX, LOOP); \
    NXT_DO_COMPLEXS(dX, LOOPC); \
}

    if (false) {}
#define RTYPE nxc_uchar
    CASE_NUM(nxd_uchar)
#undef RTYPE
#define RTYPE nxc_schar
    CASE_NUM(nxd_schar)
#undef RTYPE
#define RTYPE nxc_ushort
    CASE_NUM(nxd_ushort)
#undef RTYPE
#define RTYPE nxc_sshort
    CASE_NUM(nxd_sshort)
#undef RTYPE
#define RTYPE nxc_uint
    CASE_NUM(nxd_uint)
#undef RTYPE
#define RTYPE nxc_sint
    CASE_NUM(nxd_sint)
#undef RTYPE
#define RTYPE nxc_ulong
    CASE_NUM(nxd_ulong)
#undef RTYPE
#define RTYPE nxc_slong
    CASE_NUM(nxd_slong)
#undef RTYPE


#define RTYPE nxc_float
    CASE_NUM(nxd_float)
#undef RTYPE
#define RTYPE nxc_double
    CASE_NUM(nxd_double)
#undef RTYPE
#define RTYPE nxc_longdouble
    CASE_NUM(nxd_longdouble)
#undef RTYPE
#define RTYPE nxc_float128
    CASE_NUM(nxd_float128)
#undef RTYPE


#undef LOOP
#undef LOOPC
#undef CASE_NUM

#define LOOP(TYPE) do { \
    for (i = 0; i < len; i++, pR += sR, pX += sX) { \
        ((RTYPE*)pR)->re = *(TYPE*)pX; \
        ((RTYPE*)pR)->im = 0; \
    } \
    return 0; \
} while (0);

#define LOOPC(TYPE) do { \
    for (i = 0; i < len; i++, pR += sR, pX += sX) { \
        ((RTYPE*)pR)->re = ((TYPE*)pX)->re; \
        ((RTYPE*)pR)->im = ((TYPE*)pX)->im; \
    } \
    return 0; \
} while (0);

#define CASE_NUM(_dt) else if (dR == _dt) { \
    NXT_DO_INTS(dX, LOOP); \
    NXT_DO_FLOATS(dX, LOOP); \
    NXT_DO_COMPLEXS(dX, LOOPC); \
}

#define RTYPE nxc_complexfloat
    CASE_NUM(nxd_complexfloat)
#undef RTYPE
#define RTYPE nxc_complexdouble
    CASE_NUM(nxd_complexdouble)
#undef RTYPE
#define RTYPE nxc_complexlongdouble
    CASE_NUM(nxd_complexlongdouble)
#undef RTYPE
#define RTYPE nxc_complexfloat128
    CASE_NUM(nxd_complexfloat128)
#undef RTYPE


    return 1;
}

bool nx_cast(nxar_t r, nxar_t x) {
    return !nx_apply_elem(kern, 2, (nxar_t[]){ r, x }, NULL);
}
