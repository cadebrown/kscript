/* impl.c - implementation of math library routines
 *
 * Most C-float functions are implemented directly with 'math.h' functions, but all the C-complex functions
 *   are implemented in this function, using base-cases only (i.e. 'complex.h' is not required)
 * 
 * See 'gz.c' for Gamma/Zeta functions
 * 
 * @author:    Cade Brown <cade@kscript.org>
 */
#include <ks/impl.h>
#include <ks/m.h>


/** Internal **/

/** Fallbacks **/

#ifndef KS_HAVE_sincos

static void sincos(double x, double* sinx, double* cosx) {
    *sinx = sin(x);
    *cosx = cos(x);
}

#endif

#ifndef KS_HAVE_sincosh

static void sincosh(double x, double* sinhx, double* coshx) {
    *sinhx = sinh(x);
    *coshx = cosh(x);
}

#endif



/** complex **/

ks_ccomplex ksm_csqrt(ks_ccomplex x) {
    if (x.re == 0.0 && x.im == 0.0) return KS_CC_MAKE(0, 0);

    ks_cfloat ar = fabs(x.re) / 8.0, ai = fabs(x.im);

    ks_cfloat s = 2.0 * sqrt(ar + hypot(ar, ai/8.0));
    ks_cfloat d = ai / (2.0 * s);

    ks_cfloat rr, ri;

    if (x.re >= 0.0) {
        rr = s;
        ri = copysign(d, x.im);
    } else {
        rr = d;
        ri = copysign(s, x.im);
    }

    return KS_CC_MAKE(rr, ri);
}

ks_ccomplex ksm_ccbrt(ks_ccomplex x) {
    return ksm_cpow(x, KS_CC_MAKE(0, 1.0 / 3.0));
}

ks_ccomplex ksm_cexp(ks_ccomplex x) {
    if (x.im == 0.0) return KS_CC_MAKE(exp(x.re), 0);

    ks_cfloat a = exp(x.re);
    ks_cfloat sxi, cxi;
    sincos(x.im, &sxi, &cxi);

    ks_cfloat rr = a * cxi, ri = a * sxi;

    return KS_CC_MAKE(x.re, x.im);
}
ks_ccomplex ksm_clog(ks_ccomplex x) {
    ks_cfloat a_xr = fabs(x.re), a_xi = fabs(x.im);
    ks_cfloat h = hypot(a_xr, a_xi);

    ks_cfloat rr = log(h), ri = atan2(x.im, x.re);

    return KS_CC_MAKE(rr, ri);
}

ks_ccomplex ksm_csin(ks_ccomplex x) {
    /* sin(x) = -i * sinh(xi) */
    ks_ccomplex t0 = ksm_csinh(KS_CC_MAKE(-x.im, x.re));

    return KS_CC_MAKE(t0.im, -t0.re);
}
ks_ccomplex ksm_ccos(ks_ccomplex x) {
    /* cos(x) = cosh(x.im) */
    return ksm_ccosh(KS_CC_MAKE(-x.im, x.re));
}
ks_ccomplex ksm_ctan(ks_ccomplex x) {
    /* tan(x) = -i * tanh(x.im) */
    ks_ccomplex t0 = ksm_ctanh(KS_CC_MAKE(-x.im, x.re));
    return KS_CC_MAKE(t0.im, -t0.re);
}
ks_ccomplex ksm_casin(ks_ccomplex x) {
    /* asin(x) = -i * asinh(x.im) */
    ks_ccomplex t0 = ksm_casinh(KS_CC_MAKE(-x.im, x.re));
    return KS_CC_MAKE(t0.im, -t0.re);
}
ks_ccomplex ksm_cacos(ks_ccomplex x) {
    ks_ccomplex t0 = ksm_csqrt(KS_CC_MAKE(1.0 - x.re, -x.im));
    ks_ccomplex t1 = ksm_csqrt(KS_CC_MAKE(1.0 + x.re, +x.im));

    return KS_CC_MAKE(2.0 * atan2(t0.re, t1.re), asinh(t1.re*t0.im - t1.im*t0.re));
}
ks_ccomplex ksm_catan(ks_ccomplex x) {
    /* atan(x) = -i * atanh(x.im) */
    ks_ccomplex t0 = ksm_catanh(KS_CC_MAKE(-x.im, x.re));
    return KS_CC_MAKE(t0.im, -t0.re);
}

ks_ccomplex ksm_csinh(ks_ccomplex x) {
    ks_cfloat shxr, chxr; /* sincosh(re(x)) */
    sincosh(x.re, &shxr, &chxr);
    ks_cfloat sxi, cxi; /* sincos(im(i)) */
    sincos(x.im, &sxi, &cxi);

    return KS_CC_MAKE(cxi*shxr, sxi*chxr);
}
ks_ccomplex ksm_ccosh(ks_ccomplex x) {
    ks_cfloat shxr, chxr; /* sincosh(re(x)) */
    sincosh(x.re, &shxr, &chxr);
    ks_cfloat sxi, cxi; /* sincos(im(i)) */
    sincos(x.im, &sxi, &cxi);

    return KS_CC_MAKE(cxi*chxr, sxi*shxr);
}
ks_ccomplex ksm_ctanh(ks_ccomplex x) {
    ks_cfloat thxr = tanh(x.re), txi = tan(x.im); /* tanh(re(x)), tan(im(x)) */

    ks_cfloat cr = 1.0 / cosh(x.re), m_tht = thxr * txi;
    ks_cfloat den = 1.0 + m_tht*m_tht;

    return KS_CC_MAKE(thxr * (1.0 + txi*txi) / den, ((txi / den) * cr) * cr);
}
ks_ccomplex ksm_casinh(ks_ccomplex x) {
    ks_ccomplex t0 = ksm_csqrt(KS_CC_MAKE(1.0 + x.im, -x.re));
    ks_ccomplex t1 = ksm_csqrt(KS_CC_MAKE(1.0 - x.im, +x.re));

    return KS_CC_MAKE(asinh(t0.re*t1.im - t1.re*t0.im), atan2(x.im, t0.re*t1.re - t0.im*t1.im));
}
ks_ccomplex ksm_cacosh(ks_ccomplex x) {
    ks_ccomplex t0 = ksm_csqrt(KS_CC_MAKE(x.re - 1.0, x.im));
    ks_ccomplex t1 = ksm_csqrt(KS_CC_MAKE(x.re + 1.0, x.im));

    return KS_CC_MAKE(asinh(t0.re*t1.re + t0.im*t1.im), 2.0*atan2(t1.re, t0.re));
}
ks_ccomplex ksm_catanh(ks_ccomplex x) {
    /* atanh(x) = -atanh(-x) */
    if (x.re < 0) {
        ks_ccomplex n_r = ksm_catanh(KS_CC_NEG(x));
        return KS_CC_NEG(n_r);
    }
    ks_cfloat ai = fabs(x.im);
    ks_cfloat rr, ri;
    if (x.re == 1.0 && ai < KS_CFLOAT_MIN) {
        /* atan(1+/-0.0) == inf */
        if (ai == 0.0) {
            return KS_CC_MAKE(KS_CFLOAT_INF, x.im);
        } else {
            return KS_CC_MAKE(
                -log(sqrt(ai) / sqrt(hypot(ai, 2.0))),
                copysign(atan2(2.0, -ai) / 2.0, x.im)
            );
        }
    } else {
        /* TODO: find log1p, detect, or implement */
        return KS_CC_MAKE(
            log1p(4.0 * x.re / ((1-x.re)*(1-x.re) + ai*ai)) / 4.0,
            -atan2(-2.0 * x.im, (1-x.re)*(1+x.re) - ai*ai) / 2.0
        );
    }
}

ks_ccomplex ksm_cmul(ks_ccomplex x, ks_ccomplex y) {
    return KS_CC_MAKE(x.re*y.re - x.im*y.im, x.re*y.im + x.im*y.re);
}

ks_ccomplex ksm_cdiv(ks_ccomplex x, ks_ccomplex y) {
    /* x / y = (x * ~y) / (y * ~y) = (x * ~y) / |y|^2 */
    y = KS_CC_CONJ(y); /* deal with 'y' as conjugated */ 
    ks_cfloat ay2 = KS_CC_SQABS(y);
    if (ay2 == 0.0) return KS_CC_MAKE(KS_CFLOAT_NAN, KS_CFLOAT_NAN); /* div by 0 */

    return KS_CC_MAKE((x.re*y.re - x.im*y.im) / ay2, (x.re*y.im + x.im*y.re) / ay2);
}

ks_ccomplex ksm_cpow(ks_ccomplex x, ks_ccomplex y) {
    if (KS_CC_EQRI(y, 0, 0)) {
        /* x**0 == 1 */
        return KS_CC_MAKE(1, 0);
    } else if (KS_CC_EQRI(x, 0, 0)) {
        if (y.re <= 0) { /* actually, re(y) < 0 || (re(y) == 0 && im(y) != 0), but equiv since we already checked 'y==0+0i' */
            /* Cannot raise 0 to negative or complex power */
            return  KS_CC_MAKE(KS_CFLOAT_NAN, KS_CFLOAT_NAN);
        }
        /* 0 ** y == 0 */
        return KS_CC_MAKE(0, 0);
    } else {

        /* absolute value & phase */
        ks_cfloat a_x = KS_CC_ABS(x), a_y = KS_CC_ABS(y);
        ks_cfloat p_x = KS_CC_PHASE(x);

        /* result: 'v' */
        ks_cfloat a_v = pow(a_x, y.re), p_v = p_x * y.re;
        if (y.im != 0) {
            /* adjust for complex exponent */
            a_v *= exp(-p_x * y.im);
            p_v += y.im * log(a_x);
        }

        return KS_CC_POLAR(a_v, p_v);
    }
}

ks_ccomplex ksm_clogb(ks_ccomplex x, ks_ccomplex y) {
    if (KS_CC_EQRI(y, 0, 0) || KS_CC_EQRI(y, 1, 0)) {
        /* either cannot take log, or would divide by 0 */
        return  KS_CC_MAKE(KS_CFLOAT_NAN, KS_CFLOAT_NAN);
    }
    /* compute 'log(x) / log(y)' */
    ks_ccomplex lx = ksm_clog(x), ly = ksm_clog(y);

    return ksm_cdiv(lx, ly);
}
