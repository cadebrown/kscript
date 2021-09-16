/* ks/m.h - kscript 'm' module C-API
 *
 * The 'm' module is meant to provide a simple interface for common mathematical algorithms,
 *   functions, and constants. It is meant to work with builtin numeric types (i.e. `int`, 
 *   `float`, `complex`). See 'nx' module for tensor operations
 * 
 * Most functions are implemented in double precision, or whatever 'double' is on the platform
 *   its being compiled for
 * 
 * @author: Cade Brown <cade@kscript.org>
 */

#pragma once
#ifndef KS_M_H
#define KS_M_H

#ifndef KS_KS_H
  #include <ks/ks.h>
#endif


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



/** Functions **/


/* Returns whether 'x' and 'y' are close, within a relative and absolute error margin
 *
 * Specifically, whether 'abs(x - y) <= max(relerr * max(abs(x), abs(y)), abserr)'
 */
KS_API bool ksm_isclose(ks_cf x, ks_cf y, ks_cf relerr, ks_cf abserr);

KS_API ks_cf ksm_pow(ks_cf x, ks_cf y);
KS_API ks_cf ksm_mod(ks_cf x, ks_cf y);
KS_API ks_cf ksm_hypot(ks_cf x, ks_cf y);
KS_API ks_cf ksm_frexp(ks_cf x, int* exponent);

KS_API ks_cf ksm_abs(ks_cf x);
KS_API ks_cf ksm_min(ks_cf x, ks_cf y);
KS_API ks_cf ksm_max(ks_cf x, ks_cf y);
KS_API ks_cf ksm_agm(ks_cf x, ks_cf y);

KS_API ks_cf ksm_floor(ks_cf x);
KS_API ks_cf ksm_ceil(ks_cf x);
KS_API ks_cf ksm_round(ks_cf x);
KS_API ks_cf ksm_sqrt(ks_cf x);
KS_API ks_cf ksm_cbrt(ks_cf x);
KS_API ks_cf ksm_exp(ks_cf x);
KS_API ks_cf ksm_log(ks_cf x);
KS_API ks_cf ksm_erf(ks_cf x);
KS_API ks_cf ksm_erfc(ks_cf x);

KS_API ks_cf ksm_gamma(ks_cf x);
KS_API ks_cf ksm_zeta(ks_cf x);

KS_API ks_cf ksm_sin(ks_cf x);
KS_API ks_cf ksm_sinh(ks_cf x);
KS_API ks_cf ksm_asin(ks_cf x);
KS_API ks_cf ksm_asinh(ks_cf x);
KS_API ks_cf ksm_cos(ks_cf x);
KS_API ks_cf ksm_cosh(ks_cf x);
KS_API ks_cf ksm_acos(ks_cf x);
KS_API ks_cf ksm_acosh(ks_cf x);
KS_API ks_cf ksm_tan(ks_cf x);
KS_API ks_cf ksm_tanh(ks_cf x);
KS_API ks_cf ksm_atan(ks_cf x);
KS_API ks_cf ksm_atanh(ks_cf x);

KS_API void ksm_sincos(ks_cf x, ks_cf* sinx, ks_cf* cosx);

/*
 * NOTE: This function takes '(x, y)', whereas most 'atan2' functions take '(y, x)'
 */
KS_API ks_cf ksm_atan2(ks_cf x, ks_cf y);



KS_API ks_cc ksm_cpow(ks_cf x, ks_cf y);
KS_API ks_cc ksm_cmod(ks_cf x, ks_cf y);
KS_API ks_cc ksm_chypot(ks_cf x, ks_cf y);
KS_API ks_cc ksm_cfrexp(ks_cf x, int* exponent);

KS_API ks_cc ksm_csqrt(ks_cc x);
KS_API ks_cc ksm_ccbrt(ks_cc x);
KS_API ks_cc ksm_cexp(ks_cc x);
KS_API ks_cc ksm_clog(ks_cc x);
KS_API ks_cc ksm_cerf(ks_cc x);
KS_API ks_cc ksm_cerfc(ks_cc x);

KS_API ks_cc ksm_cgamma(ks_cc x);
KS_API ks_cc ksm_czeta(ks_cc x);

KS_API ks_cc ksm_csin(ks_cc x);
KS_API ks_cc ksm_csinh(ks_cc x);
KS_API ks_cc ksm_casin(ks_cc x);
KS_API ks_cc ksm_casinh(ks_cc x);
KS_API ks_cc ksm_ccos(ks_cc x);
KS_API ks_cc ksm_ccosh(ks_cc x);
KS_API ks_cc ksm_cacos(ks_cc x);
KS_API ks_cc ksm_cacosh(ks_cc x);
KS_API ks_cc ksm_ctan(ks_cc x);
KS_API ks_cc ksm_ctanh(ks_cc x);
KS_API ks_cc ksm_catan(ks_cc x);
KS_API ks_cc ksm_catanh(ks_cc x);


#endif /* KS_M_H */
