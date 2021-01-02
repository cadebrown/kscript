/* ks/nxt.h - NumeriX template header library
 * 
 * @author:    Cade Brown <cade@kscript.org>
 */

#pragma once
#ifndef KSNXT_H__
#define KSNXT_H__

#include <ks/nx.h>


/* Perform '_loop' for all integer types */
#define NXT_DO_INTS(_dtype, _loop) do { \
    nx_dtype dt = _dtype; \
    if (dt == nxd_schar) { _loop(nxc_schar) } \
    else if (dt == nxd_uchar) { _loop(nxc_uchar) } \
    else if (dt == nxd_sshort) { _loop(nxc_sshort) } \
    else if (dt == nxd_ushort) { _loop(nxc_ushort) } \
    else if (dt == nxd_sint) { _loop(nxc_sint) } \
    else if (dt == nxd_uint) { _loop(nxc_uint) } \
    else if (dt == nxd_slong) { _loop(nxc_slong) } \
    else if (dt == nxd_ulong) { _loop(nxc_ulong) } \
} while (0)

/* Perform '_loop' for all floating point types */
#define NXT_DO_FLOATS(_dtype, _loop) do { \
    nx_dtype dt = _dtype; \
    if (dt == nxd_float) { _loop(nxc_float) } \
    else if (dt == nxd_double) { _loop(nxc_double) } \
    else if (dt == nxd_longdouble) { _loop(nxc_longdouble) } \
    else if (dt == nxd_float128) { _loop(nxc_float128) } \
} while (0)

/* Perform '_loop' for all complex floating point types */
#define NXT_DO_COMPLEXS(_dtype, _loop) do { \
    nx_dtype dt = _dtype; \
    if (dt == nxd_complexfloat) { _loop(nxc_complexfloat) } \
    else if (dt == nxd_complexdouble) { _loop(nxc_complexdouble) } \
    else if (dt == nxd_complexlongdouble) { _loop(nxc_complexlongdouble) } \
    else if (dt == nxd_complexfloat128) { _loop(nxc_complexfloat128) } \
} while (0)


#endif /* KSNXT_H__ */
