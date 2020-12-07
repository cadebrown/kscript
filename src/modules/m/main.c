/* module.c - source code for the built-in 'm' module
 *
 * See 'impl.c' for actual implementations
 *
 * TODO: Make gamma/zeta function generated code in kscript
 * 
 * @author:    Cade Brown <cade@kscript.org>
 */
#include <ks/impl.h>
#include <ks/m.h>

#define M_NAME "m"


/* C-API */

/* Utility Functions */

// throw an argument error that some math domain error happened
static void* arg_error(char* argname, char* expr) {
    KS_THROW(kst_MathError, "Invalid argument '%s', requirement '%s' failed", argname, expr);
    return NULL;
}


/* MT: Math Templates, to reduce code duplication */

// function taking a single float argument (named _par0)
#define _MT_F(_name, _par0, _func_f, ...) static KS_TFUNC(m, _name) {           \
    kso __arg0;                                                                 \
    KS_ARGS(#_par0, &__arg0);                                                   \
    { __VA_ARGS__; }                                                            \
    ks_cfloat c_arg0;                                                           \
    if (!kso_get_cf(__arg0, &c_arg0)) return NULL;                              \
    return (kso)ks_float_new(_func_f(c_arg0));                                  \
}

// function taking two floating point arguments (named _par0 and _par1)
#define _MT_F_F(_name, _par0, _par1, _func_f, ...) static KS_TFUNC(m, _name) {  \
    kso __arg0, __arg1;                                                         \
    KS_ARGS(#_par0 " " #_par1, &__arg0, &__arg1);                               \
    { __VA_ARGS__; }                                                            \
    ks_cfloat c_arg0, c_arg1;                                                   \
    if (!kso_get_cf(__arg0, &c_arg0)                                            \
     || !kso_get_cf(__arg1, &c_arg1)) return NULL;                              \
    return (kso)ks_float_new(_func_f(c_arg0, c_arg1));                          \
}

// function taking a single argument (named _par0), and can be either a float or complex
// Use `_func_f` and `_func_c` for float and complex arguments, respectively
// You can add `special cases` by just using the varargs
#define _MT_FC(_name, _par0, _func_f, _func_c, ...) static KS_TFUNC(m, _name) { \
    kso __arg0;                                                                 \
    KS_ARGS(#_par0, &__arg0);                                                   \
    { __VA_ARGS__; }                                                            \
    if (kso_is_complex(__arg0)) {                                               \
        ks_ccomplex c_arg0;                                                     \
        if (!kso_get_cc(__arg0, &c_arg0)) return NULL;                          \
        return (kso)ks_complex_new(_func_c(c_arg0));                            \
    } else {                                                                    \
        ks_cfloat c_arg0;                                                       \
        if (!kso_get_cf(__arg0, &c_arg0)) return NULL;                          \
        return (kso)ks_float_new(_func_f(c_arg0));                              \
    }                                                                           \
}

// function taking a two arguments (named _par0 & _par1), and can be either a float or complex
// Use `_func_f` and `_func_c` for float and complex arguments, respectively
// You can add `special cases` by just using the varargs
#define _MT_FC_FC(_name, _par0, _par1, _func_f, _func_c, ...) static KS_TFUNC(m, _name) { \
    kso __arg0, __arg1;                                                         \
    KS_ARGS(#_par0 " " #_par1, &__arg0, &__arg1);                               \
    { __VA_ARGS__; }                                                            \
    if (kso_is_complex(__arg0) || kso_is_complex(__arg1)) {                     \
        ks_ccomplex c_arg0, c_arg1;                                             \
        if (!kso_get_cc(__arg0, &c_arg0)                                        \
         || !kso_get_cc(__arg1, &c_arg1)) return NULL;                          \
        return (kso)ks_complex_new(_func_c(c_arg0, c_arg1));                    \
    } else {                                                                    \
        ks_cfloat c_arg0, c_arg1;                                               \
        if (!kso_get_cf(__arg0, &c_arg0)                                        \
         || !kso_get_cf(__arg1, &c_arg1)) return NULL;                          \
        return (kso)ks_float_new(_func_f(c_arg0, c_arg1));                      \
    }                                                                           \
}


static KS_TFUNC(m, floor) {
    kso x;
    KS_ARGS("x", &x);

    if (kso_is_complex(x)) {
        KS_THROW(kst_MathError, "Cannot take 'floor' of complex number");
        return NULL;
    } else if (kso_is_int(x)) {
        return (kso)kso_int(x);
    } else {
        ks_cfloat fx;
        if (!kso_get_cf(x, &fx)) return NULL;
        /* Constructor already floors it */
        return (kso)ks_int_newf(fx);
    }
}

static KS_TFUNC(m, ceil) {
    kso x;
    KS_ARGS("x", &x);

    if (kso_is_complex(x)) {
        KS_THROW(kst_MathError, "Cannot take 'ceil' of complex number");
        return NULL;
    } else if (kso_is_int(x)) {
        return (kso)kso_int(x);
    } else {
        ks_cfloat fx;
        if (!kso_get_cf(x, &fx)) return NULL;
        return (kso)ks_int_newf(ceil(fx));
    }
}


static KS_TFUNC(m, round) {
    kso x;
    KS_ARGS("x", &x);
    
    if (kso_is_complex(x)) {
        KS_THROW(kst_MathError, "Cannot 'round' complex number");
        return NULL;
    } else if (kso_is_int(x)) {
        return (kso)kso_int(x);
    } else {
        ks_cfloat fx;
        if (!kso_get_cf(x, &fx)) return NULL;
        ks_cfloat ipart;
        ks_cfloat fpart = modf(fx, &ipart);

        ks_int res = ks_int_newf(ipart);

        /* Adjust on which was closer */
        if (fpart >= 0.5) {
            ks_int tmp = (ks_int)ks_bop_add((kso)res, (kso)_ksint_1);
            KS_DECREF(res);
            if (!tmp) {
                return NULL;
            }
            res = tmp;
        } else if (fpart < -0.5) {
            ks_int tmp = (ks_int)ks_bop_sub((kso)res, (kso)_ksint_1);
            KS_DECREF(res);
            if (!tmp) {
                return NULL;
            }
            res = tmp;
        }

        return (kso)res;
    }
}

_MT_F(abs, "x", fabs, { 
    if (kso_is_complex(__arg0)) {
        ks_ccomplex cc;
        if (!kso_get_cc(__arg0, &cc)) return NULL;
        return (kso)ks_float_new(KS_CC_ABS(cc));
    }
})

static ks_cfloat my_rad(ks_cfloat degrees) {
    return KS_M_DEG2RAD * degrees;
}

static ks_cfloat my_deg(ks_cfloat radians) {
    return KS_M_RAD2DEG * radians;
}

_MT_F(rad, "x", my_rad, { })
_MT_F(deg, "x", my_deg, { })


_MT_FC(sin, "x", sin, ksm_csin, { })
_MT_FC(cos, "x", cos, ksm_ccos, { })
_MT_FC(tan, "x", tan, ksm_ctan, { })

_MT_FC(asin, "x", asin, ksm_casin, { })
_MT_FC(acos, "x", acos, ksm_cacos, { })
_MT_FC(atan, "x", atan, ksm_catan, { })

static ks_cfloat my_atan2(ks_cfloat x, ks_cfloat y) {
    /* libc has arguments switched, and returns different range */
    long double res = atan2l(y, x);
    if (res < 0) res += KS_M_TAU;
    return res;
}

_MT_F_F(atan2, "x", "y", my_atan2, { })


_MT_FC(sinh, "x", sinh, ksm_csinh, { })
_MT_FC(cosh, "x", cosh, ksm_ccosh, { })
_MT_FC(tanh, "x", tanh, ksm_ctanh, { })

_MT_FC(asinh, "x", asinh, ksm_casinh, { })
_MT_FC(acosh, "x", acosh, ksm_cacosh, { })
_MT_FC(atanh, "x", atanh, ksm_catanh, { })

_MT_FC(sqrt, "x", sqrt, ksm_csqrt, {
    if (__arg0->type != kst_complex) {
        ks_cfloat x;
        if (!kso_get_cf(__arg0, &x)) {
            kso_catch_ignore();
        } else {
            if (x < 0.0) return arg_error("x", "x >= 0");
        }
    }
})


_MT_FC(cbrt, "x", cbrt, ksm_ccbrt, { })

_MT_F_F(hypot, "x", "y", hypot, { })
_MT_FC(exp, "x", exp, ksm_cexp, { })

static KS_TFUNC(m, log) {
    kso x;
    kso base = NULL;
    KS_ARGS("x ?base", &x, &base);

    if (x->type == kst_complex || (base != NULL && base->type == kst_complex)) {
        ks_ccomplex c_x;
        if (!kso_get_cc(x, &c_x)) return NULL;
        if (KS_CC_EQRI(c_x, 0, 0)) return arg_error("x", "x != 0.0");
        if (base != NULL) {
            ks_ccomplex c_base;
            if(!kso_get_cc(base, &c_base)) return NULL;
            if (KS_CC_EQRI(c_base, 0, 0)) return arg_error("base", "base != 0.0");
            if (KS_CC_EQRI(c_base, 1, 0)) return arg_error("base", "base != 1.0");
            return (kso)ks_complex_new(ksm_clogb(c_x, c_base));
        } else {
            /* base e */
            return (kso)ks_complex_new(ksm_clog(c_x));
        }
    } else {
        ks_cfloat c_x;
        if (!kso_get_cf(x, &c_x)) return NULL;
        if (c_x <= 0.0) return arg_error("x", "x > 0.0");
        if (base != NULL) {
            ks_cfloat c_base;
            if(!kso_get_cf(base, &c_base)) return NULL;
            if (c_base <= 0.0) return arg_error("base", "base > 0.0");
            if (c_base == 1.0) return arg_error("base", "base != 1.0");
            return (kso)ks_float_new(log(c_x) / log(c_base));
        } else {
            /* base e */
            return (kso)ks_float_new(log(c_x));
        }
    }
}

_MT_F(erf, "x", erf, { })
_MT_F(erfc, "x", erfc, { })

_MT_FC(zeta, "x", ksm_zeta, ksm_czeta, { });
_MT_FC(gamma, "x", ksm_gamma, ksm_cgamma, { });


static KS_TFUNC(m, frexp) {
    kso x;
    KS_ARGS("x", &x);
    ks_cfloat c_x;
    if (!kso_get_cf(x, &c_x)) return NULL;

    /* decompose into (mant, exp) */
    double m;
    int e;
    m = frexp(c_x, &e);

    return (kso)ks_tuple_newn(2, (kso[]){
        (kso)ks_float_new(m),
        (kso)ks_int_new(e),
    });
}

static KS_TFUNC(m, sgn) {
    kso x;
    KS_ARGS("x", &x);

    kso r = ks_bop_lt(x, (kso)_ksint_0);
    if (!r) return NULL;

    bool v;
    if (!kso_truthy(r, &v)) {
        KS_DECREF(r);
        return NULL;
    }
    KS_DECREF(r);

    if (v) return (kso)ks_int_new(-1);
    r = ks_bop_gt(x, (kso)_ksint_0);
    if (!r) return NULL;

    v;
    if (!kso_truthy(r, &v)) {
        KS_DECREF(r);
        return NULL;
    }
    KS_DECREF(r);

    if (v) return (kso)ks_int_new(+1);

    return (kso)ks_int_new(0);
}

static KS_TFUNC(m, isprime) {
    kso x;
    KS_ARGS("x", &x);

    ks_int ix = kso_int(x);
    if (!ix) return NULL;

    bool res = false;

    /* Get number of bits, according to Riemann hypothesis */
    int nb = mpz_sizeinbase(ix->val, 2);
    int mr_reps = nb * nb;
    int gr = mpz_probab_prime_p(ix->val, nb);

    res = gr != 0;

    KS_DECREF(ix);

    return KSO_BOOL(res);
}

static KS_TFUNC(m, modinv) {
    kso x, N;
    KS_ARGS("x N", &x, &N);

    ks_int ix = kso_int(x);
    if (!ix) return NULL; 
    ks_int iN = kso_int(N);
    if (!iN) {
        KS_DECREF(ix);
        return NULL; 
    }

    #ifdef KS_INT_GMP

    mpz_t rx;
    mpz_init(rx);
    bool had_inv = mpz_invert(rx, ix->val, iN->val);

    #else

    #endif

    KS_DECREF(ix);
    KS_DECREF(iN);

    if (had_inv) {
        return (kso)ks_int_newzn(rx);
    } else {
        mpz_clear(rx);
        KS_THROW(kst_MathError, "%R was not invertible modulo %R", x, N);
        return NULL;
    }
}

static KS_TFUNC(m, gcd) {
    kso x, y;
    KS_ARGS("x y", &x, &y);

    ks_int ix = kso_int(x);
    if (!ix) return NULL; 
    ks_int iy = kso_int(y);
    if (!iy) {
        KS_DECREF(iy);
        return NULL; 
    }

    mpz_t rx;
    mpz_init(rx);

    mpz_gcd(rx, ix->val, iy->val);

    KS_DECREF(ix);
    KS_DECREF(iy);
    return (kso)ks_int_newzn(rx);
}

static KS_TFUNC(m, egcd) {
    kso x, y;
    KS_ARGS("x y", &x, &y);

    ks_int ix = kso_int(x);
    if (!ix) return NULL; 
    ks_int iy = kso_int(y);
    if (!iy) {
        KS_DECREF(iy);
        return NULL; 
    }

    mpz_t rx, s, t;
    mpz_init(rx);
    mpz_init(s);
    mpz_init(t);

    mpz_gcdext(rx, s, t, ix->val, iy->val);

    KS_DECREF(ix);
    KS_DECREF(iy);

    return (kso)ks_tuple_newn(3, (kso[]){
        (kso)ks_int_newzn(rx),
        (kso)ks_int_newzn(s),
        (kso)ks_int_newzn(t),
    });
}


static KS_TFUNC(m, fact) {
    kso x;
    KS_ARGS("x", &x);

    if (!kso_is_int(x)) {
        KS_THROW(kst_TypeError, "'%T' object was not integral", x);
        return NULL;
    }

    ks_cint cx;
    if (!kso_get_ci(x, &cx)) {
        kso_catch_ignore();
        KS_THROW(kst_MathError, "Argument to factorial too large (overflow)");
        return NULL;
    } else if (cx < 0) {
        kso_catch_ignore();
        KS_THROW(kst_MathError, "Factorial is only defined for non-negative integers");
        return NULL;
    }

    #ifdef KS_INT_GMP
    mpz_t rx;
    mpz_init(rx);

    mpz_fac_ui(rx, cx);
    #else

    #endif

    return (kso)ks_int_newzn(rx);
}

static KS_TFUNC(m, choose) {
    kso n, k;
    KS_ARGS("n k", &n, &k);


    ks_int zin = kso_int(n);
    if (!zin) return NULL;

    if (!kso_is_int(k)) {
        KS_DECREF(zin);
        KS_THROW(kst_TypeError, "'%T' object was not integral", k);
        return NULL;
    }

    ks_cint ck;
    if (!kso_get_ci(k, &ck)) {
        /* Overflow; but it may be okay, since we can reflect back towards 0 */
        kso_catch_ignore();

        ks_int ik = kso_int(k);
        if (!ik) {
            KS_DECREF(zin);
            return NULL;
        }

        #ifdef KS_INT_GMP

        if (mpz_cmp_ui(ik->val, 0) < 0 || mpz_cmp(ik->val, zin->val) > 0) {
            KS_DECREF(ik);
            KS_DECREF(zin);
            KS_THROW(kst_MathError, "Choose is only defined for 'k >= 0' and 'k <= n'");
            return NULL;
        }
        
        mpz_t refl;
        mpz_init(refl);
        mpz_sub(refl, zin->val, ik->val);
        if (!mpz_fits_slong_p(refl)) {
            KS_THROW(kst_MathError, "Argument to choose too large (overflow)");
            mpz_clear(refl);
            KS_DECREF(zin);
            KS_DECREF(ik);
            return NULL;
        }
        ck = mpz_get_si(refl);
        mpz_clear(refl);

        #else

        #endif

        KS_DECREF(ik);
    }

    if (ck < 0 || mpz_cmp_si(zin->val, ck) < 0) {
        KS_DECREF(zin);
        KS_THROW(kst_MathError, "Choose is only defined for 'k >= 0' and 'k <= n'");
        return NULL;
    }

    mpz_t res;
    mpz_init(res);
    
    #ifdef KS_HAVE_GMP
    mpz_bin_ui(res, zin->val, ck);
    #else
    /* assume miniGMP */
    if (!mpz_fits_slong_p(zin->val)) {
        KS_DECREF(zin);
        KS_THROW(kst_Error, "For limited GMP builds, 'choose' requires both arguments to fit C-style integer");
        return NULL;
    }
    ks_cint cn = mpz_get_si(zin->val);
    mpz_bin_uiui(res, cn, ck);
    #endif    

    KS_DECREF(zin);
    return (kso)ks_int_newzn(res);
}


/* Export */

ks_module _ksi_m() {

    ks_module res = ks_module_new(M_NAME, KS_BIMOD_SRC, "'m' - math module\n\n    This module implements common mathematical operations", KS_IKV(
        /* Constants */
        
        {"pi",                     (kso)ks_float_new(KS_M_PI)},
        {"tau",                    (kso)ks_float_new(KS_M_TAU)},
        {"e",                      (kso)ks_float_new(KS_M_E)},
        {"phi",                    (kso)ks_float_new(KS_M_PHI)},
        {"mascheroni",             (kso)ks_float_new(KS_M_MASCHERONI)},

        /* Functions */
        
        {"floor",                  ksf_wrap(m_floor_, M_NAME ".floor(x)", "Calculate the floor of a given quantity, defined as the largest integers such that 'floor(x) <= x'")},
        {"ceil",                   ksf_wrap(m_ceil_, M_NAME ".ceil(x)", "Calculate the floor of a given quantity, defined as the largest integers such that 'ceil(x) <= x'")},
        {"round",                  ksf_wrap(m_round_, M_NAME ".round(x)", "Round a quantitity to the nearest integer, or towards '+inf' if it was in between integers")},

        {"sgn",                    ksf_wrap(m_sgn_, M_NAME ".sgn(x)", "Calculate the sign of 'x', returning either -1, 0, or +1")},
        {"frexp",                  ksf_wrap(m_frexp_, M_NAME ".frexp(x)", "Calculate '(m, e)' such that 'x == m * 2 ** e'\n\n    Real numbers only")},
        {"abs",                    ksf_wrap(m_abs_, M_NAME ".abs(x)", "Calculate the absolute value of 'x'\n\n    For complex numbers, the modulus is returned as a `float` object")},

        {"rad",                    ksf_wrap(m_rad_, M_NAME ".rad(x)", "Convert 'x' (which is understood to be in degrees) to an equivalent amount of radians")},
        {"deg",                    ksf_wrap(m_deg_, M_NAME ".deg(x)", "Convert 'x' (which is understood to be in radians) to an equivalent amount of degrees")},

        {"sin",                    ksf_wrap(m_sin_, M_NAME ".sin(x)", "Calculate the sine of 'x' (which is understood to be in radians)\n\n    More information: https://en.wikipedia.org/wiki/Sine")},
        {"cos",                    ksf_wrap(m_cos_, M_NAME ".cos(x)", "Calculate the cosine of 'x' (which is understood to be in radians)\n\n    More information: https://en.wikipedia.org/wiki/Cosine")},
        {"tan",                    ksf_wrap(m_tan_, M_NAME ".tan(x)", "Calculate the tangent of 'x' (which is understood to be in radians)\n\n    More information: https://en.wikipedia.org/wiki/Trigonometric_functions#tan")},

        {"asin",                   ksf_wrap(m_asin_, M_NAME ".asin(x)", "Calculate the arcsine of 'x' (returned in radians)\n\n    More information: https://en.wikipedia.org/wiki/Inverse_trigonometric_functions")},
        {"acos",                   ksf_wrap(m_acos_, M_NAME ".acos(x)", "Calculate the arccosine of 'x' (returned in radians)\n\n    More information: https://en.wikipedia.org/wiki/Inverse_trigonometric_functions")},
        {"atan",                   ksf_wrap(m_atan_, M_NAME ".atan(x)", "Calculate the arctangent of 'x' (returned in radians)\n\n    More information: https://en.wikipedia.org/wiki/Inverse_trigonometric_functions")},

        {"atan2",                  ksf_wrap(m_atan2_, M_NAME ".atan2(x, y)", "Calculate the 2-argument arctangent function (analogous to 'atan(y / x)', but with different phases for some arguments)\n\n    This gives the unique value such that '0 <= m.atan(y, x) < m.tau' (for all x, y != 0)\n\n    NOTE: This is different than the standard function in C, which returns a value in the range: '(-pi, pi)' and accepts the arguments in reverse order\n\n    More information: https://en.wikipedia.org/wiki/Atan2\n\n    To define a custom function which behaves the way the C one does, you can use: 'func atan2_likec(y, x) { r = m.atan2(x, y); if r > m.pi, r = r - tau; ret r }'")},
        
        {"sinh",                   ksf_wrap(m_sinh_, M_NAME ".sinh(x)", "Calculate the hyperbolic sine of 'x'\n\n    More information: https://en.wikipedia.org/wiki/Hyperbolic_functions")},
        {"cosh",                   ksf_wrap(m_cosh_, M_NAME ".cosh(x)", "Calculate the hyperbolic cosine of 'x'\n\n    More information: https://en.wikipedia.org/wiki/Hyperbolic_functions")},
        {"tanh",                   ksf_wrap(m_tanh_, M_NAME ".tanh(x)", "Calculate the hyperbolic tangent of 'x'\n\n    More information: https://en.wikipedia.org/wiki/Hyperbolic_functions")},

        {"asinh",                  ksf_wrap(m_asinh_, M_NAME ".asinh(x)", "Calculate the inverse hyperbolic sine of 'x'\n\n    More information: https://en.wikipedia.org/wiki/Inverse_hyperbolic_functions")},
        {"acosh",                  ksf_wrap(m_acosh_, M_NAME ".acosh(x)", "Calculate the inverse hyperbolic cosine of 'x'\n\n    More information: https://en.wikipedia.org/wiki/Inverse_hyperbolic_functions")},
        {"atanh",                  ksf_wrap(m_atanh_, M_NAME ".atanh(x)", "Calculate the inverse hyperbolic tangent of 'x'\n\n    More information: https://en.wikipedia.org/wiki/Inverse_hyperbolic_functions")},
        
        {"sqrt",                   ksf_wrap(m_sqrt_, M_NAME ".sqrt(x)", "Calculate the square root of 'x'\n\n    More information: https://en.wikipedia.org/wiki/Square_root")},
        {"cbrt",                   ksf_wrap(m_cbrt_, M_NAME ".cbrt(x)", "Calculate the cube root of 'x'\n\n    More information: https://en.wikipedia.org/wiki/Cube_root")},

        {"hypot",                  ksf_wrap(m_hypot_, M_NAME ".hypot(x, y)", "Calculate the hypotenuse of a right triangle with sides 'x' and 'y'\n\n    The value is equivalent to 'sqrt(x**2 + y**2)', but perhaps with better precision")},

        {"exp",                    ksf_wrap(m_exp_, M_NAME ".exp(x)", "Calculates the exponential function at 'x'\n\n    More information: https://en.wikipedia.org/wiki/Exponential_function")},
        {"pow",                    ksf_wrap(ksf_pow->cfunc, M_NAME ".pow(x, y, m=none)", "Calculates 'x**y'\n\n    Note: This is an alias to the builtin 'pow()' function")},

        {"log",                    ksf_wrap(m_log_, M_NAME ".log(x, base=m.e)", "Calculates the logarithm (default: natural) of 'x'\n\n    More information: https://en.wikipedia.org/wiki/Logarithm")},
        {"erf",                    ksf_wrap(m_erf_, M_NAME ".erf(x)", "Calculate the (Guassian) error function, evaluated at 'x'\n\n    Defined as $\\frac{2}{\\pi}\\int_{0}^{x} e^{-t^{2}} dt$\n\n    More information: https://en.wikipedia.org/wiki/Error_function")},
        {"erfc",                   ksf_wrap(m_erfc_, M_NAME ".erfc(x)", "Calculate the complimentary (Guassian) error function, evaluated at 'x'\n\n    Defined as '1-m.erf(x)'")},
        
        {"zeta",                   ksf_wrap(m_zeta_, M_NAME ".zeta(x)", "Calculate the Riemann Zeta function evaluated at 'x'\n\n    At 'x=0' (a pole), the returned value is non-finite\n\n    More information: https://en.wikipedia.org/wiki/Riemann_zeta_function")},
        {"gamma",                  ksf_wrap(m_gamma_, M_NAME ".gamma(x)", "Calculate the Gamma function at 'x'\n\n    At non-positive integers (i.e. 0, -1, -2, -3, ...), `nan` is returned\n\n    More information: https://en.wikipedia.org/wiki/Gamma_function")},

        /** Integral/Number Theory **/

        {"isprime",                ksf_wrap(m_isprime_, M_NAME ".isprime(x)", "Calculate whether 'x' is prime in the field of integers")},

        {"modinv",                 ksf_wrap(m_modinv_, M_NAME ".modinv(x, N)", "Calculate the multiplicative inverse of 'x' modulo 'N'")},

        {"gcd",                    ksf_wrap(m_gcd_, M_NAME ".gcd(x, y)", "Compute the greatest common denominator of 'x' and 'y'\n\n    The result given by this function is always positive")},
        {"egcd",                   ksf_wrap(m_egcd_, M_NAME ".egcd(x, y)", "Compute (via the extended greatest common denominator algorithm) a tuple of '(g, s, t)' such that: 'x*s + y*t == g == gcd(x, y)'\n\n    There are a few special cases:\n\n      * If 'abs(x) == abs(y)', then 's == 0' and 't == m.sgn(y)'")},

        {"fact",                   ksf_wrap(m_fact_, M_NAME ".fact(x)", "Calculate the factorial of 'x'")},
        {"choose",                 ksf_wrap(m_choose_, M_NAME ".choose(n, k)", "Calculate the binomial coefficient at 'n' and 'k' (also known as 'n choose k')\n\n    This value is 'm.fact(n) // (m.fact(k) * m.fact(n - k))'")},

    ));

    return res;
}
