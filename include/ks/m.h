/* ks/m.h - header file for the kscript math builtin module `import m`
 *
 * Defines constants, and functions (which are almost all for 'ks_cfloat' or 'ks_ccomplex', which will
 *   depend on the specific platform being compiled) related to mathematic.
 *
 * Functions with an '_' appended are my internal implementations of the functions. They are
 *   used as fallbacks for when 'libm' didn't contain them. Use the non-underscore versions (which
 *   will be defined to the fastest and most accurate version for a given platform, in 'ks/m_macros.h')
 *
 * @author: Cade Brown <cade@kscript.org>
 */

#pragma once
#ifndef KSM_H__
#define KSM_H__

#ifndef KS_H__
#include <ks/ks.h>
#endif /* KS_H__ */

/* If defined, we assume that the Riemann Hypothesis is true */
#define KSM_ASSUME_RH


/* Constants */

/* PI, circle constant */
#define KSM_PI                    3.14159265358979323846264338327950288419716939937510

/* TAU, REAL circle constant (2*PI) */
#define KSM_TAU                   6.28318530717958647692528676655900576839433879875021

/* E, Euler's number, (sum(1/n!)) */
#define KSM_E                     2.71828182845904523536028747135266249775724709369995

/* PHI, golden ratio, (1+sqrt(5))/2 */
#define KSM_PHI                   1.61803398874989484820458683436563811772030917980576

/* GAMMA, the Euler-Mascheroni constant (lim H_N - sum(1 / i for i in range(1, N+1))) */
#define KSM_MASCHERONI            0.57721566490153286060651209008240243104215933593992

/* Multiplier to convert degrees to radians */
#define KSM_DEG2RAD               (KSM_PI / 180.0)

/* Multiplier to convert radians to degrees */
#define KSM_RAD2DEG               (180.0 / KSM_PI)


/* log(pi) */
#define KSM_LOG_PI                1.14472988584940017414342735135305871164729481291531

/* sqrt(2 * pi) */
#define KSM_SQRT_2PI              2.50662827463100050241576528481104525300698674060993

/* log(sqrt(2 * pi)) */
#define KSM_LOG_SQRT_2PI          0.91893853320467274178032973640561763986139747363778


/** All Functions **/

/* Returns whether 'x' and 'y' are close, within a relative and absolute error margin
 *
 * Specifically:
 *   abs(x - y) <= max(rel_err * max(abs(x), abs(y)), abs_err)
 */
KS_API bool ksm_isclose(ks_cfloat x, ks_cfloat y, ks_cfloat rel_err, ks_cfloat abs_err);

KS_API ks_cfloat ksm_gamma(ks_cfloat x);
KS_API ks_cfloat ksm_zeta(ks_cfloat x);

/*** 'ks_ccomplex' functions (these do not throw errors) ***/

KS_API ks_ccomplex ksm_csqrt(ks_ccomplex x);
KS_API ks_ccomplex ksm_ccbrt(ks_ccomplex x);
KS_API ks_ccomplex ksm_cexp(ks_ccomplex x);
KS_API ks_ccomplex ksm_clog(ks_ccomplex x);
KS_API ks_ccomplex ksm_cerf(ks_ccomplex x);
KS_API ks_ccomplex ksm_cerfc(ks_ccomplex x);
KS_API ks_ccomplex ksm_cgamma(ks_ccomplex x);
KS_API ks_ccomplex ksm_czeta(ks_ccomplex x);

KS_API ks_ccomplex ksm_csin(ks_ccomplex x);
KS_API ks_ccomplex ksm_ccos(ks_ccomplex x);
KS_API ks_ccomplex ksm_ctan(ks_ccomplex x);
KS_API ks_ccomplex ksm_casin(ks_ccomplex x);
KS_API ks_ccomplex ksm_cacos(ks_ccomplex x);
KS_API ks_ccomplex ksm_catan(ks_ccomplex x);
KS_API ks_ccomplex ksm_csinh(ks_ccomplex x);
KS_API ks_ccomplex ksm_ccosh(ks_ccomplex x);
KS_API ks_ccomplex ksm_ctanh(ks_ccomplex x);
KS_API ks_ccomplex ksm_casinh(ks_ccomplex x);
KS_API ks_ccomplex ksm_cacosh(ks_ccomplex x);
KS_API ks_ccomplex ksm_catanh(ks_ccomplex x);

KS_API ks_ccomplex ksm_cmul(ks_ccomplex x, ks_ccomplex y);
KS_API ks_ccomplex ksm_cdiv(ks_ccomplex x, ks_ccomplex y);
KS_API ks_ccomplex ksm_cpow(ks_ccomplex x, ks_ccomplex y);
KS_API ks_ccomplex ksm_clogb(ks_ccomplex x, ks_ccomplex y);

#endif /* KSM_H__ */
