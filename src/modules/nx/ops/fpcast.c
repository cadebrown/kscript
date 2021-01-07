/* fpcast.c - 'fpcast' kernel
 *
 * @author: Cade Brown <cade@kscript.org>
 */
#include <ks/impl.h>
#include <ks/nx.h>
#include <ks/nxt.h>

#define K_NAME "fpcast"


static int kern(int N, nxar_t* inp, int len, void* _data) {
    assert(N == 2);

    ks_cint i;
    nx_dtype dR = inp[0].dtype, dX = inp[1].dtype;
    ks_uint pR = (ks_uint)inp[0].data, pX = (ks_uint)inp[1].data;
    ks_cint sR = inp[0].strides[0], sX = inp[1].strides[0];

#define CASE_NUM(_dt) else if (dR == _dt) { \
    NXT_DO_INTS(dX, LOOPI); \
    NXT_DO_FLOATS(dX, LOOPF); \
    NXT_DO_COMPLEXS(dX, LOOPC); \
}

    /* Integer types */
#define LOOPI(TYPE) do { \
    int sa = (int)sizeof(RTYPE) - (int)sizeof(TYPE); \
    for (i = 0; i < len; i++, pR += sR, pX += sX) { \
        *(RTYPE*)pR = sa > 0 ? (*(TYPE*)pX << sa) : (*(TYPE*)pX >> -sa); \
    } \
    return 0; \
} while (0);

#define LOOPF(TYPE) do { \
    bool sgn = RTYPE_MIN < 0; \
    for (i = 0; i < len; i++, pR += sR, pX += sX) { \
        *(RTYPE*)pR = sgn ? (*(TYPE*)pX * ((TYPE)RTYPE_MAX - RTYPE_MIN) + RTYPE_MIN) : (*(TYPE*)pX * RTYPE_MAX); \
    } \
    return 0; \
} while (0);

#define LOOPC(TYPE) do { \
    bool sgn = RTYPE_MIN < 0; \
    for (i = 0; i < len; i++, pR += sR, pX += sX) { \
        TYPE t; \
        *(RTYPE*)pR = sgn ? (((TYPE*)pX)->re * ((t.re = RTYPE_MAX) - RTYPE_MIN) + RTYPE_MIN) : (((TYPE*)pX)->re * RTYPE_MAX); \
    } \
    return 0; \
} while (0);

#define MIN(T) T##_MIN
#define MAX(T) T##_MAX

    if (false) {}
#define RTYPE nxc_uchar
#define RTYPE_MIN MIN(nxc_uchar)
#define RTYPE_MAX MAX(nxc_uchar)
    CASE_NUM(nxd_uchar)
#undef RTYPE
#undef RTYPE_MIN
#undef RTYPE_MAX

#define RTYPE nxc_schar
#define RTYPE_MIN MIN(nxc_schar)
#define RTYPE_MAX MAX(nxc_schar)
    CASE_NUM(nxd_schar)
#undef RTYPE
#undef RTYPE_MIN
#undef RTYPE_MAX
#define RTYPE nxc_ushort
#define RTYPE_MIN MIN(nxc_ushort)
#define RTYPE_MAX MAX(nxc_ushort)
    CASE_NUM(nxd_ushort)
#undef RTYPE
#undef RTYPE_MIN
#undef RTYPE_MAX
#define RTYPE nxc_sshort
#define RTYPE_MIN MIN(nxc_sshort)
#define RTYPE_MAX MAX(nxc_sshort)
    CASE_NUM(nxd_sshort)
#undef RTYPE
#undef RTYPE_MIN
#undef RTYPE_MAX
#define RTYPE nxc_uint
#define RTYPE_MIN MIN(nxc_uint)
#define RTYPE_MAX MAX(nxc_uint)
    CASE_NUM(nxd_uint)
#undef RTYPE
#undef RTYPE_MIN
#undef RTYPE_MAX
#define RTYPE nxc_sint
#define RTYPE_MIN MIN(nxc_sint)
#define RTYPE_MAX MAX(nxc_sint)
    CASE_NUM(nxd_sint)
#undef RTYPE
#undef RTYPE_MIN
#undef RTYPE_MAX
#define RTYPE nxc_ulong
#define RTYPE_MIN MIN(nxc_ulong)
#define RTYPE_MAX MAX(nxc_ulong)
    CASE_NUM(nxd_ulong)
#undef RTYPE
#undef RTYPE_MIN
#undef RTYPE_MAX
#define RTYPE nxc_slong
#define RTYPE_MIN MIN(nxc_slong)
#define RTYPE_MAX MAX(nxc_slong)
    CASE_NUM(nxd_slong)
#undef RTYPE
#undef RTYPE_MIN
#undef RTYPE_MAX


    /* Float types */
#undef LOOPI
#define LOOPI(TYPE) do { \
    RTYPE sa = (RTYPE)MAX(TYPE) - MIN(TYPE), sb = MIN(TYPE); \
    bool sgn = MIN(TYPE) < 0; \
    for (i = 0; i < len; i++, pR += sR, pX += sX) { \
        *(RTYPE*)pR = sgn ? (2 * (RTYPE)(*(TYPE*)pX - sb / 2) / sa) : ((RTYPE)(*(TYPE*)pX - sb) / sa); \
    } \
    return 0; \
} while (0);

#undef LOOPF
#define LOOPF(TYPE) do { \
    for (i = 0; i < len; i++, pR += sR, pX += sX) { \
        *(RTYPE*)pR = *(TYPE*)pX; \
    } \
    return 0; \
} while (0);

#undef LOOPC
#define LOOPC(TYPE) do { \
    for (i = 0; i < len; i++, pR += sR, pX += sX) { \
        *(RTYPE*)pR = ((TYPE*)pX)->re; \
    } \
    return 0; \
} while (0);

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


    KS_THROW(kst_TypeError, "Types not supported for '%s' kernel: %R, %R", K_NAME, dR, dX);
    return 1;
}

bool nx_fpcast(nxar_t r, nxar_t x) {
    return !nx_apply_elem(kern, 2, (nxar_t[]){ r, x }, NULL);
}
