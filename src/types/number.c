/* types/number.c - 'number' type
 *
 * This is the abstract type, but it handles most functionality
 *
 * @author: Cade Brown <cade@kscript.org>
 */
#include <ks/impl.h>
#include <ks/m.h>

#define T_NAME "number"


/* Internals */

/* Calculate a rational precision, and return a floating point value which works even
 *   when one of the integers would overflow. (i.e. (10**10000/10**9999))
 *
 * Basic idea:
 *   n/d  = (2**s * n / (2**s * d))
 *        = (2**s * n / d) / 2**s
 * 
 * Choose 's' such that 2**-s <= KSF_EPS, i.e. 1 ULP is insignificant, and thus
 *   full precision is generated
 * 
 * SEE: https://gmplib.org/list-archives/gmp-discuss/2012-June/005064.html
 */
static ks_cfloat i_mpz_div_d(mpz_t n, mpz_t d) {
    /* bitlen(n), bitlen(d) */
    size_t nb_n = mpz_sizeinbase(n, 2), nb_d = mpz_sizeinbase(d, 2);
    
    /* -log(KSF_EPS) / log(2) == KSF_DIG * log(10) / log(2) (well-behaved bound) */
    int s = (int)((KS_CFLOAT_DIG) * 3.4) + 4;
    
    /* q = (2**s * n / d) */
    mpz_t q;
    mpz_init(q);
    mpz_mul_2exp(q, n, s);
    mpz_tdiv_q(q, q, d);


    /* Convert 'q' back */
    ks_cfloat qf = mpz_get_d(q);
    mpz_clear(q);

    if (qf == INFINITY) return qf;
    
    /* q /= 2 ** s */
    qf = ldexp(qf, -s);
    return qf;
}


/* C-API */

ks_number ks_number_news(const char* src, ks_ssize_t len_b, int base) {
    if (len_b < 0) len_b = strlen(src);

    int i = len_b - 1;

    while (i > 0 && src[i] == ')') i--;

    // detect complex numbers
    bool isComplex = i >= 0 && src[i] == 'i' || src[i] == 'I';
    if (isComplex) {

        ks_ccomplex v;
        if (!ks_ccomplex_from_str(src, len_b, &v)) {
            return NULL;
        }
        return (kso)ks_complex_new(v);
    }

    if (base == 0) {
        if (len_b < 2) base = 10;
        else if (src[0] == '0') {
            /**/ if (src[1] == 'b' || src[1] == 'B') base = 2;
            else if (src[1] == 'o' || src[1] == 'O') base = 8;
            else if (src[1] == 'x' || src[1] == 'X') base = 16;
            else if (src[1] == 'd' || src[1] == 'D') base = 10;
        }
    }

    if (base == 0) base = 10;

    /*  */
    bool isFloat = false;
    for (i = 0; i < len_b && !isFloat; ++i) {
        if (src[i] == '.' || (base == 10 && (src[i] == 'e' || src[i] == 'E'))) isFloat = true;
    }

    if (isFloat) {
        ks_cfloat v;
        if (!ks_cfloat_from_str(src, len_b, &v)) return NULL;
        return (kso)ks_float_new(v);
    } else {
        return (kso)ks_int_news(len_b, src, base);
    }
}

/* Type Functions */


/** Unary Operators **/


static KS_TFUNC(T, new) {
    ks_type tp;
    kso obj = KSO_NONE;
    KS_ARGS("tp:* ?obj", &tp, kst_type, &obj);

    if (kso_issub(obj->type, kst_type)) return KS_NEWREF(obj);

    if (tp != kst_number) {
        KS_THROW(kst_Error, "Derived type of 'number' (%R) did not define '__new()' method", tp);
        return NULL;
    }

   if (obj == KSO_NONE) {
        return (kso)ks_int_new(0);
    } else if (kso_issub(obj->type, tp)) {
        return KS_NEWREF(obj);
    } else if (kso_issub(obj->type, kst_str)) {
        return (kso)ks_number_news(((ks_str)obj)->data, ((ks_str)obj)->len_b, 0);
    }

    KS_THROW_CONV(obj->type, tp);
    return NULL;
}
static KS_TFUNC(T, bool) {
    kso V;
    KS_ARGS("V", &V);

    if (kso_is_complex(V)) {
        ks_ccomplex Vc;
        if (!kso_get_cc(V, &Vc)) return NULL;

        return KSO_BOOL(!KS_CC_EQRI(Vc, 0, 0));
    } else if (kso_is_float(V)) {
        ks_cfloat Vf;
        if (!kso_get_cf(V, &Vf)) return NULL;
        return KSO_BOOL(Vf != 0);
    } else if (kso_is_int(V)) {
        ks_int Vi, R = NULL;
        if (!(Vi = kso_int(V))) return NULL;

        bool res = ks_int_cmp_c(Vi, 0) != 0;
        KS_DECREF(Vi);

        return KSO_BOOL(res);
    }

    return KSO_UNDEFINED;
}

static KS_TFUNC(T, abs) {
    kso V;
    KS_ARGS("V", &V);

    if (kso_is_complex(V)) {
        ks_ccomplex Vc, R;
        if (!kso_get_cc(V, &Vc)) return NULL;

        return (kso)ks_float_new(KS_CC_ABS(Vc));
    } else if (kso_is_float(V)) {
        ks_cfloat Vf, R;
        if (!kso_get_cf(V, &Vf)) return NULL;
        if (Vf >= 0) return KS_NEWREF(V);
        return (kso)ks_float_new(-Vf);
    } else if (kso_is_int(V)) {
        ks_int Vi, R = NULL;
        if (!(Vi = kso_int(V))) return NULL;

        if (ks_int_cmp_c(Vi, 0) >= 0) return (kso)Vi;

        /* Otherwise, negate */

        mpz_t v;
        mpz_init(v);
        mpz_neg(v, Vi->val);
        R = ks_int_newzn(v);

        KS_DECREF(Vi);
        return (kso)R;
    }

    return KSO_UNDEFINED;
}

static KS_TFUNC(T, pos) {
    kso V;
    KS_ARGS("V", &V);

    return KS_NEWREF(V);
}

static KS_TFUNC(T, neg) {
    kso V;
    KS_ARGS("V", &V);

    if (kso_is_complex(V)) {
        ks_ccomplex Vc, R;
        if (!kso_get_cc(V, &Vc)) return NULL;

        R = KS_CC_NEG(Vc);

        return (kso)ks_complex_new(R);
    } else if (kso_is_float(V)) {
        ks_cfloat Vf, R;
        if (!kso_get_cf(V, &Vf)) return NULL;
        return (kso)ks_float_new(-Vf);
    } else if (kso_is_int(V)) {
        ks_int Vi, R = NULL;
        if (!(Vi = kso_int(V))) return NULL;

        mpz_t v;
        mpz_init(v);
        mpz_neg(v, Vi->val);
        R = ks_int_newzn(v);

        KS_DECREF(Vi);
        return (kso)R;
    }

    return KSO_UNDEFINED;
}

static KS_TFUNC(T, sqig) {
    kso V;
    KS_ARGS("V", &V);

    if (kso_is_complex(V)) {
        ks_ccomplex Vc, R;
        if (!kso_get_cc(V, &Vc)) return NULL;

        R = KS_CC_CONJ(Vc);

        return (kso)ks_complex_new(R);
    } else if (kso_is_int(V)) {
        ks_int Vi, R = NULL;
        if (!(Vi = kso_int(V))) return NULL;

        mpz_t v;
        mpz_init(v);
        mpz_neg(v, Vi->val);
        mpz_sub_ui(v, v, 1);
        R = ks_int_newzn(v);

        KS_DECREF(Vi);
        return (kso)R;
    }

    return KSO_UNDEFINED;
}


/** Arithmetic Operations **/

static KS_TFUNC(T, add) {
    kso L, R;
    KS_ARGS("L R", &L, &R);
    if (kso_is_num(L) && kso_is_num(R)) {
        if (kso_is_complex(L) || kso_is_complex(R)) {
            ks_ccomplex Lc, Rc, V;
            if (!kso_get_cc(L, &Lc) || !kso_get_cc(R, &Rc)) return NULL;

            V = KS_CC_MAKE(Lc.re + Rc.re, Lc.im + Rc.im);

            return (kso)ks_complex_new(V);
        } else if (kso_is_float(L) || kso_is_float(R)) {
            ks_cfloat Lf, Rf, V;
            if (!kso_get_cf(L, &Lf) || !kso_get_cf(R, &Rf)) return NULL;
            return (kso)ks_float_new(Lf + Rf);
        } else if (kso_is_int(L) || kso_is_int(R)) {
            ks_int Li, Ri, V = NULL;
            if (!(Li = kso_int(L))) return NULL;
            if (!(Ri = kso_int(R))) {
                KS_DECREF(Li);
                return NULL;
            }

            mpz_t v;
            mpz_init(v);
            mpz_add(v, Li->val, Ri->val);
            V = ks_int_newzn(v);

            KS_DECREF(Li);
            KS_DECREF(Ri);
            return (kso)V;
        }
    }

    return KSO_UNDEFINED;
}

static KS_TFUNC(T, sub) {
    kso L, R;
    KS_ARGS("L R", &L, &R);
    if (kso_is_num(L) && kso_is_num(R)) {
        if (kso_is_complex(L) || kso_is_complex(R)) {
            ks_ccomplex Lc, Rc, V;
            if (!kso_get_cc(L, &Lc) || !kso_get_cc(R, &Rc)) return NULL;

            V = KS_CC_MAKE(Lc.re - Rc.re, Lc.im - Rc.im);

            return (kso)ks_complex_new(V);
        } else if (kso_is_float(L) || kso_is_float(R)) {
            ks_cfloat Lf, Rf, V;
            if (!kso_get_cf(L, &Lf) || !kso_get_cf(R, &Rf)) return NULL;
            return (kso)ks_float_new(Lf - Rf);
        } else if (kso_is_int(L) || kso_is_int(R)) {
            ks_int Li, Ri, V = NULL;
            if (!(Li = kso_int(L))) return NULL;
            if (!(Ri = kso_int(R))) {
                KS_DECREF(Li);
                return NULL;
            }

            mpz_t v;
            mpz_init(v);
            mpz_sub(v, Li->val, Ri->val);
            V = ks_int_newzn(v);

            KS_DECREF(Li);
            KS_DECREF(Ri);
            return (kso)V;
        }
    }

    return KSO_UNDEFINED;
}

static KS_TFUNC(T, mul) {
    kso L, R;
    KS_ARGS("L R", &L, &R);
    if (kso_is_num(L) && kso_is_num(R)) {
        if (kso_is_complex(L) || kso_is_complex(R)) {
            ks_ccomplex Lc, Rc, V;
            if (!kso_get_cc(L, &Lc) || !kso_get_cc(R, &Rc)) return NULL;

            V = KS_CC_MAKE(Lc.re * Rc.re - Lc.im * Rc.im, Lc.re * Rc.im + Lc.im * Rc.re);

            return (kso)ks_complex_new(V);
        } else if (kso_is_float(L) || kso_is_float(R)) {
            ks_cfloat Lf, Rf, V;
            if (!kso_get_cf(L, &Lf) || !kso_get_cf(R, &Rf)) return NULL;
            
            return (kso)ks_float_new(Lf * Rf);
        } else if (kso_is_int(L) || kso_is_int(R)) {
            ks_int Li, Ri, V = NULL;
            if (!(Li = kso_int(L))) return NULL;
            if (!(Ri = kso_int(R))) {
                KS_DECREF(Li);
                return NULL;
            }

            mpz_t v;
            mpz_init(v);
            mpz_mul(v, Li->val, Ri->val);
            V = ks_int_newzn(v);

            KS_DECREF(Li);
            KS_DECREF(Ri);
            return (kso)V;
        }
    }

    return KSO_UNDEFINED;
}

static KS_TFUNC(T, div) {
    kso L, R;
    KS_ARGS("L R", &L, &R);
    if (kso_is_num(L) && kso_is_num(R)) {
        if (kso_is_complex(L) || kso_is_complex(R)) {
            ks_ccomplex Lc, Rc, V;
            if (!kso_get_cc(L, &Lc) || !kso_get_cc(R, &Rc)) return NULL;

            /* L / R == L * R^C / (R * R^C) == L * R^C / |R|^2 */
            if (KS_CC_EQRI(Rc, 0, 0)) {
                KS_THROW(kst_MathError, "Division by 0");
                return NULL;
            }

            /* |R|^2 */
            ks_cfloat Ra2 = KS_CC_SQABS(Rc);
            Ra2 *= Ra2;

            /* Conjugate 'R' */
            Rc.im = -Rc.im;

            V = KS_CC_MAKE(Lc.re * Rc.re - Lc.im * Rc.im, Lc.re * Rc.im + Lc.im * Rc.re);
            V.re /= Ra2;
            V.im /= Ra2;

            return (kso)ks_complex_new(V);
        } else if (kso_is_float(L) || kso_is_float(R)) {
            ks_cfloat Lf, Rf, V;
            if (!kso_get_cf(L, &Lf) || !kso_get_cf(R, &Rf)) return NULL;
            if (Rf == 0) {
                KS_THROW(kst_MathError, "Division by 0");
                return NULL;
            }
            return (kso)ks_float_new(Lf / Rf);
        } else if (kso_is_int(L) || kso_is_int(R)) {
            ks_int Li, Ri, V = NULL;
            if (!(Li = kso_int(L))) return NULL;
            if (!(Ri = kso_int(R))) {
                KS_DECREF(Li);
                return NULL;
            }
            if (ks_int_cmp_c(Ri, 0) == 0) {
                KS_DECREF(Li);
                KS_DECREF(Ri);
                KS_THROW(kst_MathError, "Division by 0");
                return NULL;
            }

            ks_cfloat res = 0.0;
            res = i_mpz_div_d(Li->val, Ri->val);

            KS_DECREF(Li);
            KS_DECREF(Ri);
            return (kso)ks_float_new(res);
        }
    }

    return KSO_UNDEFINED;
}


static KS_TFUNC(T, floordiv) {
    kso L, R = NULL;
    KS_ARGS("L R", &L, &R);

    if (kso_is_num(L) && kso_is_num(R)) {
        if (kso_is_complex(L) || kso_is_complex(R)) {
            KS_THROW(kst_MathError, "Cannot 'floordiv' complex numbers");
            return NULL;

        } else if (kso_is_float(L) || kso_is_float(R)) {
            ks_cfloat Lf, Rf, V;
            if (!kso_get_cf(L, &Lf) || !kso_get_cf(R, &Rf)) return NULL;
            if (Rf == 0) {
                KS_THROW(kst_MathError, "Division by 0");
                return NULL;
            }
            return (kso)ks_int_newf(Lf / Rf);
        } else if (kso_is_int(L) || kso_is_int(R)) {
            ks_int Li, Ri;
            if (!(Li = kso_int(L))) return NULL;
            if (!(Ri = kso_int(R))) {
                KS_DECREF(Li);
                return NULL;
            }
            if (ks_int_cmp_c(Ri, 0) == 0) {
                KS_DECREF(Li);
                KS_DECREF(Ri);
                KS_THROW(kst_MathError, "Division by 0");
                return NULL;
            }

            ks_int res = NULL;
            mpz_t Rzd;
            mpz_init(Rzd);
            mpz_fdiv_q(Rzd, Li->val, Ri->val);
            res = ks_int_newzn(Rzd);

            KS_DECREF(Li);
            KS_DECREF(Ri);
            return (kso)res;
        }
    }

    return KSO_UNDEFINED;
}


static KS_TFUNC(T, mod) {
    kso L, R = NULL;
    KS_ARGS("L R", &L, &R);

    if (kso_is_num(L) && kso_is_num(R)) {
        if (kso_is_complex(L) || kso_is_complex(R)) {
            KS_THROW(kst_MathError, "Cannot 'mod' complex numbers");
            return NULL;

        } else if (kso_is_float(L) || kso_is_float(R)) {
            ks_cfloat Lf, Rf, V;
            if (!kso_get_cf(L, &Lf) || !kso_get_cf(R, &Rf)) return NULL;
            if (Rf == 0) {
                KS_THROW(kst_MathError, "Modulo by 0");
                return NULL;
            }
            V = fmod(Lf, Rf);
            /**/ if (Rf > 0 && V < 0) V += Rf;
            else if (Rf < 0 && V > 0) V += Rf;
            return (kso)ks_float_new(V);
        } else if (kso_is_int(L) || kso_is_int(R)) {
            ks_int Li, Ri;
            if (!(Li = kso_int(L))) return NULL;
            if (!(Ri = kso_int(R))) {
                KS_DECREF(Li);
                return NULL;
            }
            if (ks_int_cmp_c(Ri, 0) == 0) {
                KS_DECREF(Li);
                KS_DECREF(Ri);
                KS_THROW(kst_MathError, "Modulo by 0");
                return NULL;
            }

            ks_int res = NULL;
            mpz_t Rzd;
            mpz_init(Rzd);
            mpz_fdiv_r(Rzd, Li->val, Ri->val);
            res = ks_int_newzn(Rzd);

            KS_DECREF(Li);
            KS_DECREF(Ri);
            return (kso)res;
        }
    }

    return KSO_UNDEFINED;
}

static KS_TFUNC(T, pow) {
    kso L, R = NULL;
    KS_ARGS("L R", &L, &R);

    if (kso_is_num(L) && kso_is_num(R)) {
        if (kso_is_complex(L) || kso_is_complex(R)) {
            ks_ccomplex Lc, Rc, V;
            if (!kso_get_cc(L, &Lc) || !kso_get_cc(R, &Rc)) return NULL;

            if (KS_CC_EQRI(Lc, 0, 0) && (Rc.re < 0 || (Rc.re == 0 && Rc.im != 0))) {
                /* Cannot raise 0 to negative or complex power */
                KS_THROW(kst_MathError, "Cannot raise 0 to negative (or complex) power");
                return NULL;
            }

            V = ksm_cpow(Lc, Rc);
            return (kso)ks_complex_new(V);
        } else if (kso_is_float(L) || kso_is_float(R)) {
            ks_cfloat Lf, Rf, V;
            if (!kso_get_cf(L, &Lf) || !kso_get_cf(R, &Rf)) return NULL;
            if (Lf == 0.0 && Rf < 0.0) {
                KS_THROW(kst_MathError, "Cannot raise 0 to negative power");
                return NULL;
            }
            return (kso)ks_float_new(pow(Lf, Rf));
        } else if (kso_is_int(L) || kso_is_int(R)) {
            ks_int Li, V = NULL;
            ks_cint Rc;
            if (!(Li = kso_int(L))) return NULL;
            if (!kso_get_ci(R, &Rc)) {
                KS_DECREF(Li);
                return NULL;
            }
            if (Rc == 0) {
                KS_DECREF(Li);
                return (kso)ks_int_new(1);
            } else if (Rc == 1) {
                return (kso)Li;
            } else if (Rc < 0) {
                if (ks_int_cmp_c(Li, 0) == 0) {
                    /* (L <= 0) ** (R < 0), invalid */
                    KS_THROW(kst_MathError, "Cannot raise 0 to negative power");
                    KS_DECREF(Li);
                    return NULL;
                } else {
                    /* (L > 0) ** (R > 0), valid, but we should convert to float */
                    ks_cfloat Lf;
                    if (!kso_get_cf((kso)Li, &Lf)) {
                        KS_DECREF(Li);
                        return NULL;
                    }

                    KS_DECREF(Li);
                    return (kso)ks_float_new(pow(Lf, Rc));
                }
            }
            mpz_t v;
            mpz_init(v);
            mpz_pow_ui(v, Li->val, Rc);
            V = ks_int_newzn(v);

            KS_DECREF(Li);
            return (kso)V;
        }
    }

    return KSO_UNDEFINED;
}



/** Bitwise **/


static KS_TFUNC(T, lsh) {
    kso L, R = NULL;
    KS_ARGS("L R", &L, &R);

    if (kso_is_num(L) && kso_is_num(R)) {
        if (kso_is_int(L) || kso_is_int(R)) {
            ks_int Li, Ri;
            if (!(Li = kso_int(L))) return NULL;
            if (!(Ri = kso_int(R))) {
                KS_DECREF(Li);
                return NULL;
            }

            ks_cint Rc;
            if (!kso_get_ci((kso)Ri, &Rc)) {
                KS_DECREF(Li);
                KS_DECREF(Ri);
                return NULL;
            }

            ks_int res = NULL;
            mpz_t Rz;
            mpz_init(Rz);
            if (Rc > 0) mpz_mul_2exp(Rz, Li->val, Rc);
            else mpz_tdiv_q_2exp(Rz, Li->val, -Rc);
            res = ks_int_newzn(Rz);

            KS_DECREF(Li);
            KS_DECREF(Ri);
            return (kso)res;
        }
    }

    return KSO_UNDEFINED;
}


static KS_TFUNC(T, rsh) {
    kso L, R = NULL;
    KS_ARGS("L R", &L, &R);

    if (kso_is_num(L) && kso_is_num(R)) {
        if (kso_is_int(L) || kso_is_int(R)) {
            ks_int Li, Ri;
            if (!(Li = kso_int(L))) return NULL;
            if (!(Ri = kso_int(R))) {
                KS_DECREF(Li);
                return NULL;
            }

            ks_cint Rc;
            if (!kso_get_ci((kso)Ri, &Rc)) {
                KS_DECREF(Li);
                KS_DECREF(Ri);
                return NULL;
            }

            ks_int res = NULL;
            mpz_t Rz;
            mpz_init(Rz);
            if (Rc > 0) mpz_tdiv_q_2exp(Rz, Li->val, Rc);
            else mpz_mul_2exp(Rz, Li->val, -Rc);
            res = ks_int_newzn(Rz);

            KS_DECREF(Li);
            KS_DECREF(Ri);
            return (kso)res;
        }
    }

    return KSO_UNDEFINED;
}

static KS_TFUNC(T, binior) {
    kso L, R = NULL;
    KS_ARGS("L R", &L, &R);

    if (kso_is_num(L) && kso_is_num(R)) {
        if (kso_is_int(L) || kso_is_int(R)) {
            ks_int Li, Ri;
            if (!(Li = kso_int(L))) return NULL;
            if (!(Ri = kso_int(R))) {
                KS_DECREF(Li);
                return NULL;
            }


            ks_int res = NULL;
            mpz_t Rz;
            mpz_init(Rz);
            mpz_ior(Rz, Li->val, Ri->val);
            res = ks_int_newzn(Rz);

            KS_DECREF(Li);
            KS_DECREF(Ri);
            return (kso)res;
        }
    }

    return KSO_UNDEFINED;
}
static KS_TFUNC(T, binxor) {
    kso L, R = NULL;
    KS_ARGS("L R", &L, &R);

    if (kso_is_num(L) && kso_is_num(R)) {
        if (kso_is_int(L) || kso_is_int(R)) {
            ks_int Li, Ri;
            if (!(Li = kso_int(L))) return NULL;
            if (!(Ri = kso_int(R))) {
                KS_DECREF(Li);
                return NULL;
            }
            ks_int res = NULL;
            mpz_t Rz;
            mpz_init(Rz);
            mpz_xor(Rz, Li->val, Ri->val);
            res = ks_int_newzn(Rz);

            KS_DECREF(Li);
            KS_DECREF(Ri);
            return (kso)res;
        }
    }

    return KSO_UNDEFINED;
}

static KS_TFUNC(T, binand) {
    kso L, R = NULL;
    KS_ARGS("L R", &L, &R);

    if (kso_is_num(L) && kso_is_num(R)) {
        if (kso_is_int(L) || kso_is_int(R)) {
            ks_int Li, Ri;
            if (!(Li = kso_int(L))) return NULL;
            if (!(Ri = kso_int(R))) {
                KS_DECREF(Li);
                return NULL;
            }

            ks_int res = NULL;
            mpz_t Rz;
            mpz_init(Rz);
            mpz_and(Rz, Li->val, Ri->val);
            res = ks_int_newzn(Rz);

            KS_DECREF(Li);
            KS_DECREF(Ri);
            return (kso)res;
        }
    }

    return KSO_UNDEFINED;
}

/** Comparisons **/


/* Internal numerical comparison */
static bool i_num_cmp(kso L, kso R, int* res) {
    if (kso_is_complex(L) || kso_is_complex(R)) {
        KS_THROW(kst_MathError, "'complex' numbers cannot be compared");
        return false;
    } else if (kso_is_float(L) || kso_is_float(R)) {
        ks_cfloat Lf, Rf;
        if (!kso_get_cf(L, &Lf) || !kso_get_cf(R, &Rf)) return false;

        *res = (Lf > Rf) - (Lf < Rf);
        return true;
    } else if (kso_is_int(L) || kso_is_int(R)) {
        ks_int Li, Ri;
        if (!(Li = kso_int(L))) return NULL;
        if (!(Ri = kso_int(R))) {
            KS_DECREF(Li);
            return NULL;
        }

        *res = mpz_cmp(Li->val, Ri->val);
        KS_DECREF(Li);
        KS_DECREF(Ri);

        return true;
    }

    KS_THROW(kst_TypeError, "'%T' and '%T' objects cannot be compared", L, R);
    return false;
}

static KS_TFUNC(T, cmp) {
    kso L, R = NULL;
    KS_ARGS("L R", &L, &R);

    if (kso_is_num(L) && kso_is_num(R)) {
        int cv;
        if (!i_num_cmp(L, R, &cv)) return NULL;

        return (kso)ks_int_new(cv);
    }

    return KSO_UNDEFINED;
}

static KS_TFUNC(T, lt) {
    kso L, R = NULL;
    KS_ARGS("L R", &L, &R);

    if (kso_is_num(L) && kso_is_num(R)) {
        int cv;
        if (!i_num_cmp(L, R, &cv)) return NULL;

        return KSO_BOOL(cv < 0);
    }

    return KSO_UNDEFINED;
}

static KS_TFUNC(T, le) {
    kso L, R = NULL;
    KS_ARGS("L R", &L, &R);

    if (kso_is_num(L) && kso_is_num(R)) {
        int cv;
        if (!i_num_cmp(L, R, &cv)) return NULL;

        return KSO_BOOL(cv <= 0);
    }

    return KSO_UNDEFINED;
}

static KS_TFUNC(T, gt) {
    kso L, R = NULL;
    KS_ARGS("L R", &L, &R);

    if (kso_is_num(L) && kso_is_num(R)) {
        int cv;
        if (!i_num_cmp(L, R, &cv)) return NULL;

        return KSO_BOOL(cv > 0);
    }

    return KSO_UNDEFINED;
}

static KS_TFUNC(T, ge) {
    kso L, R = NULL;
    KS_ARGS("L R", &L, &R);

    if (kso_is_num(L) && kso_is_num(R)) {
        int cv;
        if (!i_num_cmp(L, R, &cv)) return NULL;

        return KSO_BOOL(cv >= 0);
    }

    return KSO_UNDEFINED;
}

static KS_TFUNC(T, eq) {
    kso L, R = NULL;
    KS_ARGS("L R", &L, &R);

    if (kso_is_num(L) && kso_is_num(R)) {
        if (kso_is_complex(L) || kso_is_complex(R)) {
            ks_ccomplex Lc, Rc;
            if (!kso_get_cc(L, &Lc) || !kso_get_cc(R, &Rc)) return NULL;
            return KSO_BOOL(KS_CC_EQ(Lc, Rc));
        } else if (kso_is_float(L) || kso_is_float(R)) {
            ks_cfloat Lf, Rf;
            if (!kso_get_cf(L, &Lf) || !kso_get_cf(R, &Rf)) return NULL;
            return KSO_BOOL(Lf == Rf);
        } else if (kso_is_int(L) || kso_is_int(R)) {
            ks_int Li, Ri;
            if (!(Li = kso_int(L))) return NULL;
            if (!(Ri = kso_int(R))) {
                KS_DECREF(Li);
                return NULL;
            }
            bool res = false;

            res = mpz_cmp(Li->val, Ri->val) == 0;

            KS_DECREF(Li);
            KS_DECREF(Ri);
            return KSO_BOOL(res);
        }
    }

    return KSO_BOOL(L == R);
}

static KS_TFUNC(T, ne) {
    kso L, R = NULL;
    KS_ARGS("L R", &L, &R);

    if (kso_is_num(L) && kso_is_num(R)) {
        if (kso_is_complex(L) || kso_is_complex(R)) {
            ks_ccomplex Lc, Rc;
            if (!kso_get_cc(L, &Lc) || !kso_get_cc(R, &Rc)) return NULL;
            return KSO_BOOL(!KS_CC_EQ(Lc, Rc));
        } else if (kso_is_float(L) || kso_is_float(R)) {
            ks_cfloat Lf, Rf;
            if (!kso_get_cf(L, &Lf) || !kso_get_cf(R, &Rf)) return NULL;
            return KSO_BOOL(Lf != Rf);
        } else if (kso_is_int(L) || kso_is_int(R)) {
            ks_int Li, Ri;
            if (!(Li = kso_int(L))) return NULL;
            if (!(Ri = kso_int(R))) {
                KS_DECREF(Li);
                return NULL;
            }
            bool res = false;

            res = mpz_cmp(Li->val, Ri->val) != 0;

            KS_DECREF(Li);
            KS_DECREF(Ri);
            return KSO_BOOL(res);
        }
    }

    return KSO_BOOL(L == R);
}



/* Export */

static struct ks_type_s tp;
ks_type kst_number = &tp;

void _ksi_number() {
    _ksinit(kst_number, kst_object, T_NAME, sizeof(struct kso_s), -1, "Abstract base type of other numeric quantities\n\n   Defines operators for different types of numbers, but cannot be instantiated directly (see types 'int', 'float', 'complex' for examples)", KS_IKV(

        {"__new",                  ksf_wrap(T_new_, T_NAME ".__new(self, obj=none)", "")},
        {"__bool",                 ksf_wrap(T_bool_, T_NAME ".__bool(self)", "")},
        {"__abs",                  ksf_wrap(T_abs_, T_NAME ".__abs(self)", "")},
        {"__pos",                  ksf_wrap(T_pos_, T_NAME ".__pos(self)", "")},
        {"__neg",                  ksf_wrap(T_neg_, T_NAME ".__neg(self)", "")},
        {"__sqig",                 ksf_wrap(T_sqig_, T_NAME ".__sqig(self)", "")},

        {"__add",                  ksf_wrap(T_add_, T_NAME ".__add(L, R)", "")},
        {"__sub",                  ksf_wrap(T_sub_, T_NAME ".__sub(L, R)", "")},
        {"__mul",                  ksf_wrap(T_mul_, T_NAME ".__mul(L, R)", "")},
        {"__div",                  ksf_wrap(T_div_, T_NAME ".__div(L, R)", "")},
        {"__floordiv",             ksf_wrap(T_floordiv_, T_NAME ".__floordiv(L, R)", "")},
        {"__mod",                  ksf_wrap(T_mod_, T_NAME ".__mod(L, R)", "")},
        {"__pow",                  ksf_wrap(T_pow_, T_NAME ".__pow(L, R)", "")},

        {"__lsh",                  ksf_wrap(T_lsh_, T_NAME ".__lsh(L, R)", "")},
        {"__rsh",                  ksf_wrap(T_rsh_, T_NAME ".__rsh(L, R)", "")},
        {"__binior",               ksf_wrap(T_binior_, T_NAME ".__binior(L, R)", "")},
        {"__binxor",               ksf_wrap(T_binxor_, T_NAME ".__binxor(L, R)", "")},
        {"__binand",               ksf_wrap(T_binand_, T_NAME ".__binand(L, R)", "")},

        {"__lt",                   ksf_wrap(T_lt_, T_NAME ".__lt(L, R)", "")},
        {"__gt",                   ksf_wrap(T_gt_, T_NAME ".__gt(L, R)", "")},
        {"__le",                   ksf_wrap(T_le_, T_NAME ".__le(L, R)", "")},
        {"__ge",                   ksf_wrap(T_ge_, T_NAME ".__ge(L, R)", "")},
        {"__eq",                   ksf_wrap(T_eq_, T_NAME ".__eq(L, R)", "")},
        {"__ne",                   ksf_wrap(T_ne_, T_NAME ".__ne(L, R)", "")},

    ));
}
