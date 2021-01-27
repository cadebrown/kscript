/* ks/nxk.cf.kern.h - Complex functions
 *
 * 
 * @author: Cade Brown <cade@kscript.org> 
 */

#define rNAN NXK_ATTR(rNAN)
#define rINF NXK_ATTR(rINF)

/* pow(x, y) */
static inline 
NXK_TYPE NXK_FUNC(pow)(NXK_TYPE x, NXK_TYPE y) {
    NXK_TYPE r;
    if (x.re == 0 && x.im == 0) {
        r.re = 1;
        r.im = 0;
    } else if (x.re == 0 && x.im == 0) {
        if (y.re <= 0) {
            r.re = rNAN;
            r.im = rNAN;
        } else {
            r.re = 0;
            r.im = 0;
        }
    } else if (x.re == 0 && y.im == 0 && (y.re == y.re && y.re < rINF && y.re > -rINF) && ((int)y.re) == y.re && y.re > 0) {
        int b = (int)y.re;
        int bm4 = b % 4;
        NXK_ATTR(r) v = NXK_FUNC(rpow)(x.im, b);
        if (bm4 == 0) {
            r.re = v;
            r.im = 0;
        } else if (bm4 == 1) {
            r.re = 0;
            r.im = v;
        } else if (bm4 == 2) {
            r.re = -v;
            r.im = 0;
        } else {
            r.re = 0;
            r.im = -v;
        }
    } else {
        NXK_TYPE a, p, v;
        a.re = NXK_FUNC(rhypot)(x.re, x.im);
        a.im = NXK_FUNC(rhypot)(y.re, y.im);

        p.re = NXK_FUNC(ratan2)(x.im, x.re);

        v.re = NXK_FUNC(rpow)(a.re, y.re);
        v.im = p.re * y.re;

        if (y.im != 0) {
            v.re *= NXK_FUNC(rexp)(-p.re * y.im);
            v.im += y.im * NXK_FUNC(rlog)(a.re);
        }

        r.re = v.re * NXK_FUNC(rcos)(v.im);
        r.im = v.re * NXK_FUNC(rsin)(v.im);
    }
    return r;
}

/* sqrt(x) */
static inline 
NXK_TYPE NXK_FUNC(sqrt)(NXK_TYPE x) {
    NXK_TYPE r;
    if (x.re == 0 && x.im == 0) {
        r = x;
    } else {
        NXK_TYPE t;
        t.re = NXK_FUNC(rfabs)(x.re) / 8;
        t.im = NXK_FUNC(rfabs)(x.im);
        NXK_TYPE s;
        s.re = 2 * NXK_FUNC(rsqrt)(t.re + NXK_FUNC(rhypot)(t.re, t.im / 8));
        s.im = t.im / (2 * s.re);
        if (x.re >= 0.0) {
            r.re = s.re;
            r.im = NXK_FUNC(rcopysign)(s.im, x.im);
        } else {
            r.re = s.im;
            r.im = NXK_FUNC(rcopysign)(s.re, x.im);
        }
    }
    return r;
}

/* exp(x) */
static inline 
NXK_TYPE NXK_FUNC(exp)(NXK_TYPE x) {
    NXK_TYPE r;
    if (x.im == 0) {
        r.re = NXK_FUNC(rexp)(x.re);
        r.im = 0;
    } else {
        x.re = NXK_FUNC(rexp)(x.re);
        r.re = x.re * NXK_FUNC(rcos)(x.im);
        r.re = x.re * NXK_FUNC(rsin)(x.im);
    }
    return r;
}

/* log(x) */
static inline 
NXK_TYPE NXK_FUNC(log)(NXK_TYPE x) {
    NXK_TYPE r;
    if (x.im == 0) {
        r.re = NXK_FUNC(rlog)(x.re);
        r.im = 0;
    } else {
        NXK_TYPE t;
        t.re = NXK_FUNC(rhypot)(x.re, x.im);
        r.re = x.re * NXK_FUNC(rlog)(t.re);
        r.re = x.re * NXK_FUNC(ratan2)(x.im, x.re);
    }
    return r;
}
