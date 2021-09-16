/* modules/m/impl.c - Implementations of C-style math functions
 *
 * NOTE: No error handling is done for these, that should be done in the module
 *         functions
 *
 * Functions defined elsewhere:
 *   * ksm_gamma: impl_gamma.c
 *   * ksm_zeta: impl_zeta.c
 *
 * @author: Cade Brown <cade@kscript.org>
 */

#include <ks/impl.h>


/* C-API */

bool ksm_isclose(ks_cf x, ks_cf y, ks_cf relerr, ks_cf abserr);

ks_cf ksm_pow(ks_cf x, ks_cf y) {
    return pow(x, y);
}

ks_cf ksm_mod(ks_cf x, ks_cf y) {
    return fmod(x, y);
}

ks_cf ksm_hypot(ks_cf x, ks_cf y) {
    return hypot(x, y);
}

ks_cf ksm_frexp(ks_cf x, int* exponent) {
    return frexp(x, exponent);
}

ks_cf ksm_abs(ks_cf x) {
    return fabs(x);
}

ks_cf ksm_min(ks_cf x, ks_cf y) {
    return fmin(x, y);
}

ks_cf ksm_max(ks_cf x, ks_cf y) {
    return fmax(x, y);
}

ks_cf ksm_agm(ks_cf x, ks_cf y) {
    /* Not defined! */
    if (x * y < 0) return KS_CF_NAN;

    /* Now, repeat the loop:
     *   x' = (x + y) / 2
     *   y' = sqrt(x * y)
     * 
     * Until the error converges
     */
    ks_cf err = KS_CF_EPS;
    while (ksm_abs(x - y) > err) {
        ks_cf tmp = ksm_sqrt(x * y);
        x = (x + y) / 2;
        y = tmp;
    }
    
    return x;
}

ks_cf ksm_floor(ks_cf x) {
    return floor(x);
}

ks_cf ksm_ceil(ks_cf x) {
    return ceil(x);
}

ks_cf ksm_round(ks_cf x) {
    return round(x);
}

ks_cf ksm_sqrt(ks_cf x) {
    return sqrt(x);
}

ks_cf ksm_cbrt(ks_cf x) {
    return cbrt(x);
}

ks_cf ksm_exp(ks_cf x) {
    return exp(x);
}

ks_cf ksm_log(ks_cf x) {
    return log(x);
}

ks_cf ksm_erf(ks_cf x) {
    return erf(x);
}

ks_cf ksm_erfc(ks_cf x) {
    return erfc(x);
}


ks_cf ksm_sin(ks_cf x) {
    return sin(x);
}

ks_cf ksm_sinh(ks_cf x) {
    return sinh(x);
}

ks_cf ksm_asin(ks_cf x) {
    return asin(x);
}

ks_cf ksm_asinh(ks_cf x) {
    return asinh(x);
}

ks_cf ksm_cos(ks_cf x) {
    return cos(x);
}

ks_cf ksm_cosh(ks_cf x) {
    return cosh(x);
}

ks_cf ksm_acos(ks_cf x) {
    return acos(x);
}

ks_cf ksm_acosh(ks_cf x) {
    return acosh(x);
}

ks_cf ksm_tan(ks_cf x) {
    return tan(x);
}

ks_cf ksm_tanh(ks_cf x) {
    return tanh(x);
}

ks_cf ksm_atan(ks_cf x) {
    return atan(x);
}

ks_cf ksm_atanh(ks_cf x) {
    return atanh(x);
}

void ksm_sincos(ks_cf x, ks_cf* sinx, ks_cf* cosx) {
    *sinx = sin(x);
    *cosx = cos(x);
}

ks_cf ksm_atan2(ks_cf x, ks_cf y) {
    /* NOTE: The order must be switched! */
    return atan2(y, x);
}
