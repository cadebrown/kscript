/* ks/nxe.h - elementwise function definitions
 * 
 * 
 * @author:    Cade Brown <cade@kscript.org>
 */

#pragma once
#ifndef NXE_H__
#define NXE_H__

#include <ks/nxt.h>
#include <ks/nxm.h>

/* To include static functions, write:
 * #define NXE_STATIC
 * Before including this file
 */
#ifndef NXE_STATIC

#define LOOPC(TYPE, NAME) \
KS_API TYPE TYPE##sqrt(TYPE x); \
KS_API TYPE TYPE##sinh(TYPE x); \
KS_API TYPE TYPE##cosh(TYPE x); \
KS_API TYPE TYPE##tanh(TYPE x); \
KS_API TYPE TYPE##asinh(TYPE x); \
KS_API TYPE TYPE##acosh(TYPE x); \
KS_API TYPE TYPE##atanh(TYPE x); \
KS_API TYPE TYPE##sin(TYPE x); \
KS_API TYPE TYPE##cos(TYPE x); \
KS_API TYPE TYPE##tan(TYPE x); \
KS_API TYPE TYPE##asin(TYPE x); \
KS_API TYPE TYPE##acos(TYPE x); \
KS_API TYPE TYPE##atan(TYPE x);

NXT_PASTE_C(LOOPC);
#undef LOOPC

#else

/* Static Implementations */
#error Static NXE not supported...
#endif


#endif /* NXE_H__ */
