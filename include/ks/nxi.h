/* ks/nxi.h - internal header with utilities
 * 
 * 
 * @author:    Cade Brown <cade@kscript.org>
 */

#pragma once
#ifndef NXI_H__
#define NXI_H__

#ifndef KSNX_H__
  #include <ks/nx.h>
#endif

/** General Utilities **/

/* Computes 'data + strides[:] * idxs[:]'
 */
static inline void* szdot(void* data, int rank, ks_ssize_t* strides, ks_size_t* idxs) {
    ks_uint res = (ks_uint)data;
    int i;
    for (i = 0; i < rank; ++i) {
        res += strides[i] * idxs[i];
    }
    return (void*)res;
}

/* Computes product of sizes
 */
static inline ks_size_t szprod(int rank, ks_size_t* shape) {
    ks_size_t res = 1;
    int i;
    for (i = 0; i < rank; ++i) {
        res *= shape[i];
    }
    return res;
}



/** Type macros **/


/* Function: strfrom */

#ifndef KS_HAVE_strfromd
  #define strfromd ks_strfromd
#endif

#define nx_Fstrfrom strfromd
#define nx_Dstrfrom strfromd
#define nx_Estrfrom strfromd
#define nx_Qstrfrom strfromd

#define nx_cFrstrfrom nx_Fstrfrom
#define nx_cDrstrfrom nx_Dstrfrom
#define nx_cErstrfrom nx_Estrfrom
#define nx_cQrstrfrom nx_Qstrfrom

#if defined(KS_HAVE_strfromf128)
  #undef nx_Qstrfrom
  #define nx_Qstrfrom strfromf128
#elif defined(KS_HAVE_strfroml)
  #undef nx_Qstrfrom
  #define nx_Qstrfrom strfroml
#endif
#if defined(KS_HAVE_strfroml)
  #undef nx_Estrfrom
  #define nx_Estrfrom strfroml
#endif
#if defined(KS_HAVE_strfromf)
  #undef nx_Fstrfrom
  #define nx_Fstrfrom strfromf
#endif

/* Function: fmin */

#define nx_Ffmin fmin
#define nx_Dfmin fmin
#define nx_Efmin fmin
#define nx_Qfmin fmin

#define nx_cFrfmin nx_Ffmin
#define nx_cDrfmin nx_Dfmin
#define nx_cErfmin nx_Efmin
#define nx_cQrfmin nx_Qfmin

#if defined(KS_HAVE_fminf128)
  #undef nx_Qfmin
  #define nx_Qfmin fminf128
#elif defined(KS_HAVE_fminl)
  #undef nx_Qfmin
  #define nx_Qfmin fminl
#endif
#if defined(KS_HAVE_fminl)
  #undef nx_Efmin
  #define nx_Efmin fminl
#endif
#if defined(KS_HAVE_fminf)
  #undef nx_Ffmin
  #define nx_Ffmin fminf
#endif

/* Function: fmax */

#define nx_Ffmax fmax
#define nx_Dfmax fmax
#define nx_Efmax fmax
#define nx_Qfmax fmax

#define nx_cFrfmax nx_Ffmax
#define nx_cDrfmax nx_Dfmax
#define nx_cErfmax nx_Efmax
#define nx_cQrfmax nx_Qfmax

#if defined(KS_HAVE_fmaxf128)
  #undef nx_Qfmax
  #define nx_Qfmax fmaxf128
#elif defined(KS_HAVE_fmaxl)
  #undef nx_Qfmax
  #define nx_Qfmax fmaxl
#endif
#if defined(KS_HAVE_fmaxl)
  #undef nx_Efmax
  #define nx_Efmax fmaxl
#endif
#if defined(KS_HAVE_fmaxf)
  #undef nx_Ffmax
  #define nx_Ffmax fmaxf
#endif

/* Function: fabs */

#define nx_Ffabs fabs
#define nx_Dfabs fabs
#define nx_Efabs fabs
#define nx_Qfabs fabs

#define nx_cFrfabs nx_Ffabs
#define nx_cDrfabs nx_Dfabs
#define nx_cErfabs nx_Efabs
#define nx_cQrfabs nx_Qfabs

#if defined(KS_HAVE_fabsf128)
  #undef nx_Qfabs
  #define nx_Qfabs fabsf128
#elif defined(KS_HAVE_fabsl)
  #undef nx_Qfabs
  #define nx_Qfabs fabsl
#endif
#if defined(KS_HAVE_fabsl)
  #undef nx_Efabs
  #define nx_Efabs fabsl
#endif
#if defined(KS_HAVE_fabsf)
  #undef nx_Ffabs
  #define nx_Ffabs fabsf
#endif

/* Function: fmod */

#define nx_Ffmod fmod
#define nx_Dfmod fmod
#define nx_Efmod fmod
#define nx_Qfmod fmod

#define nx_cFrfmod nx_Ffmod
#define nx_cDrfmod nx_Dfmod
#define nx_cErfmod nx_Efmod
#define nx_cQrfmod nx_Qfmod

#if defined(KS_HAVE_fmodf128)
  #undef nx_Qfmod
  #define nx_Qfmod fmodf128
#elif defined(KS_HAVE_fmodl)
  #undef nx_Qfmod
  #define nx_Qfmod fmodl
#endif
#if defined(KS_HAVE_fmodl)
  #undef nx_Efmod
  #define nx_Efmod fmodl
#endif
#if defined(KS_HAVE_fmodf)
  #undef nx_Ffmod
  #define nx_Ffmod fmodf
#endif

/* Function: ceil */

#define nx_Fceil ceil
#define nx_Dceil ceil
#define nx_Eceil ceil
#define nx_Qceil ceil

#define nx_cFrceil nx_Fceil
#define nx_cDrceil nx_Dceil
#define nx_cErceil nx_Eceil
#define nx_cQrceil nx_Qceil

#if defined(KS_HAVE_ceilf128)
  #undef nx_Qceil
  #define nx_Qceil ceilf128
#elif defined(KS_HAVE_ceill)
  #undef nx_Qceil
  #define nx_Qceil ceill
#endif
#if defined(KS_HAVE_ceill)
  #undef nx_Eceil
  #define nx_Eceil ceill
#endif
#if defined(KS_HAVE_ceilf)
  #undef nx_Fceil
  #define nx_Fceil ceilf
#endif

/* Function: floor */

#define nx_Ffloor floor
#define nx_Dfloor floor
#define nx_Efloor floor
#define nx_Qfloor floor

#define nx_cFrfloor nx_Ffloor
#define nx_cDrfloor nx_Dfloor
#define nx_cErfloor nx_Efloor
#define nx_cQrfloor nx_Qfloor

#if defined(KS_HAVE_floorf128)
  #undef nx_Qfloor
  #define nx_Qfloor floorf128
#elif defined(KS_HAVE_floorl)
  #undef nx_Qfloor
  #define nx_Qfloor floorl
#endif
#if defined(KS_HAVE_floorl)
  #undef nx_Efloor
  #define nx_Efloor floorl
#endif
#if defined(KS_HAVE_floorf)
  #undef nx_Ffloor
  #define nx_Ffloor floorf
#endif

/* Function: trunc */

#define nx_Ftrunc trunc
#define nx_Dtrunc trunc
#define nx_Etrunc trunc
#define nx_Qtrunc trunc

#define nx_cFrtrunc nx_Ftrunc
#define nx_cDrtrunc nx_Dtrunc
#define nx_cErtrunc nx_Etrunc
#define nx_cQrtrunc nx_Qtrunc

#if defined(KS_HAVE_truncf128)
  #undef nx_Qtrunc
  #define nx_Qtrunc truncf128
#elif defined(KS_HAVE_truncl)
  #undef nx_Qtrunc
  #define nx_Qtrunc truncl
#endif
#if defined(KS_HAVE_truncl)
  #undef nx_Etrunc
  #define nx_Etrunc truncl
#endif
#if defined(KS_HAVE_truncf)
  #undef nx_Ftrunc
  #define nx_Ftrunc truncf
#endif

/* Function: copysign */

#define nx_Fcopysign copysign
#define nx_Dcopysign copysign
#define nx_Ecopysign copysign
#define nx_Qcopysign copysign

#define nx_cFrcopysign nx_Fcopysign
#define nx_cDrcopysign nx_Dcopysign
#define nx_cErcopysign nx_Ecopysign
#define nx_cQrcopysign nx_Qcopysign

#if defined(KS_HAVE_copysignf128)
  #undef nx_Qcopysign
  #define nx_Qcopysign copysignf128
#elif defined(KS_HAVE_copysignl)
  #undef nx_Qcopysign
  #define nx_Qcopysign copysignl
#endif
#if defined(KS_HAVE_copysignl)
  #undef nx_Ecopysign
  #define nx_Ecopysign copysignl
#endif
#if defined(KS_HAVE_copysignf)
  #undef nx_Fcopysign
  #define nx_Fcopysign copysignf
#endif

/* Function: exp */

#define nx_Fexp exp
#define nx_Dexp exp
#define nx_Eexp exp
#define nx_Qexp exp

#define nx_cFrexp nx_Fexp
#define nx_cDrexp nx_Dexp
#define nx_cErexp nx_Eexp
#define nx_cQrexp nx_Qexp

#if defined(KS_HAVE_expf128)
  #undef nx_Qexp
  #define nx_Qexp expf128
#elif defined(KS_HAVE_expl)
  #undef nx_Qexp
  #define nx_Qexp expl
#endif
#if defined(KS_HAVE_expl)
  #undef nx_Eexp
  #define nx_Eexp expl
#endif
#if defined(KS_HAVE_expf)
  #undef nx_Fexp
  #define nx_Fexp expf
#endif

/* Function: expm1 */

#define nx_Fexpm1 expm1
#define nx_Dexpm1 expm1
#define nx_Eexpm1 expm1
#define nx_Qexpm1 expm1

#define nx_cFrexpm1 nx_Fexpm1
#define nx_cDrexpm1 nx_Dexpm1
#define nx_cErexpm1 nx_Eexpm1
#define nx_cQrexpm1 nx_Qexpm1

#if defined(KS_HAVE_expm1f128)
  #undef nx_Qexpm1
  #define nx_Qexpm1 expm1f128
#elif defined(KS_HAVE_expm1l)
  #undef nx_Qexpm1
  #define nx_Qexpm1 expm1l
#endif
#if defined(KS_HAVE_expm1l)
  #undef nx_Eexpm1
  #define nx_Eexpm1 expm1l
#endif
#if defined(KS_HAVE_expm1f)
  #undef nx_Fexpm1
  #define nx_Fexpm1 expm1f
#endif

/* Function: log */

#define nx_Flog log
#define nx_Dlog log
#define nx_Elog log
#define nx_Qlog log

#define nx_cFrlog nx_Flog
#define nx_cDrlog nx_Dlog
#define nx_cErlog nx_Elog
#define nx_cQrlog nx_Qlog

#if defined(KS_HAVE_logf128)
  #undef nx_Qlog
  #define nx_Qlog logf128
#elif defined(KS_HAVE_logl)
  #undef nx_Qlog
  #define nx_Qlog logl
#endif
#if defined(KS_HAVE_logl)
  #undef nx_Elog
  #define nx_Elog logl
#endif
#if defined(KS_HAVE_logf)
  #undef nx_Flog
  #define nx_Flog logf
#endif

/* Function: log1p */

#define nx_Flog1p log1p
#define nx_Dlog1p log1p
#define nx_Elog1p log1p
#define nx_Qlog1p log1p

#define nx_cFrlog1p nx_Flog1p
#define nx_cDrlog1p nx_Dlog1p
#define nx_cErlog1p nx_Elog1p
#define nx_cQrlog1p nx_Qlog1p

#if defined(KS_HAVE_log1pf128)
  #undef nx_Qlog1p
  #define nx_Qlog1p log1pf128
#elif defined(KS_HAVE_log1pl)
  #undef nx_Qlog1p
  #define nx_Qlog1p log1pl
#endif
#if defined(KS_HAVE_log1pl)
  #undef nx_Elog1p
  #define nx_Elog1p log1pl
#endif
#if defined(KS_HAVE_log1pf)
  #undef nx_Flog1p
  #define nx_Flog1p log1pf
#endif

/* Function: sqrt */

#define nx_Fsqrt sqrt
#define nx_Dsqrt sqrt
#define nx_Esqrt sqrt
#define nx_Qsqrt sqrt

#define nx_cFrsqrt nx_Fsqrt
#define nx_cDrsqrt nx_Dsqrt
#define nx_cErsqrt nx_Esqrt
#define nx_cQrsqrt nx_Qsqrt

#if defined(KS_HAVE_sqrtf128)
  #undef nx_Qsqrt
  #define nx_Qsqrt sqrtf128
#elif defined(KS_HAVE_sqrtl)
  #undef nx_Qsqrt
  #define nx_Qsqrt sqrtl
#endif
#if defined(KS_HAVE_sqrtl)
  #undef nx_Esqrt
  #define nx_Esqrt sqrtl
#endif
#if defined(KS_HAVE_sqrtf)
  #undef nx_Fsqrt
  #define nx_Fsqrt sqrtf
#endif

/* Function: cbrt */

#define nx_Fcbrt cbrt
#define nx_Dcbrt cbrt
#define nx_Ecbrt cbrt
#define nx_Qcbrt cbrt

#define nx_cFrcbrt nx_Fcbrt
#define nx_cDrcbrt nx_Dcbrt
#define nx_cErcbrt nx_Ecbrt
#define nx_cQrcbrt nx_Qcbrt

#if defined(KS_HAVE_cbrtf128)
  #undef nx_Qcbrt
  #define nx_Qcbrt cbrtf128
#elif defined(KS_HAVE_cbrtl)
  #undef nx_Qcbrt
  #define nx_Qcbrt cbrtl
#endif
#if defined(KS_HAVE_cbrtl)
  #undef nx_Ecbrt
  #define nx_Ecbrt cbrtl
#endif
#if defined(KS_HAVE_cbrtf)
  #undef nx_Fcbrt
  #define nx_Fcbrt cbrtf
#endif

/* Function: hypot */

#define nx_Fhypot hypot
#define nx_Dhypot hypot
#define nx_Ehypot hypot
#define nx_Qhypot hypot

#define nx_cFrhypot nx_Fhypot
#define nx_cDrhypot nx_Dhypot
#define nx_cErhypot nx_Ehypot
#define nx_cQrhypot nx_Qhypot

#if defined(KS_HAVE_hypotf128)
  #undef nx_Qhypot
  #define nx_Qhypot hypotf128
#elif defined(KS_HAVE_hypotl)
  #undef nx_Qhypot
  #define nx_Qhypot hypotl
#endif
#if defined(KS_HAVE_hypotl)
  #undef nx_Ehypot
  #define nx_Ehypot hypotl
#endif
#if defined(KS_HAVE_hypotf)
  #undef nx_Fhypot
  #define nx_Fhypot hypotf
#endif

/* Function: pow */

#define nx_Fpow pow
#define nx_Dpow pow
#define nx_Epow pow
#define nx_Qpow pow

#define nx_cFrpow nx_Fpow
#define nx_cDrpow nx_Dpow
#define nx_cErpow nx_Epow
#define nx_cQrpow nx_Qpow

#if defined(KS_HAVE_powf128)
  #undef nx_Qpow
  #define nx_Qpow powf128
#elif defined(KS_HAVE_powl)
  #undef nx_Qpow
  #define nx_Qpow powl
#endif
#if defined(KS_HAVE_powl)
  #undef nx_Epow
  #define nx_Epow powl
#endif
#if defined(KS_HAVE_powf)
  #undef nx_Fpow
  #define nx_Fpow powf
#endif

/* Function: erf */

#define nx_Ferf erf
#define nx_Derf erf
#define nx_Eerf erf
#define nx_Qerf erf

#define nx_cFrerf nx_Ferf
#define nx_cDrerf nx_Derf
#define nx_cErerf nx_Eerf
#define nx_cQrerf nx_Qerf

#if defined(KS_HAVE_erff128)
  #undef nx_Qerf
  #define nx_Qerf erff128
#elif defined(KS_HAVE_erfl)
  #undef nx_Qerf
  #define nx_Qerf erfl
#endif
#if defined(KS_HAVE_erfl)
  #undef nx_Eerf
  #define nx_Eerf erfl
#endif
#if defined(KS_HAVE_erff)
  #undef nx_Ferf
  #define nx_Ferf erff
#endif

/* Function: erfc */

#define nx_Ferfc erfc
#define nx_Derfc erfc
#define nx_Eerfc erfc
#define nx_Qerfc erfc

#define nx_cFrerfc nx_Ferfc
#define nx_cDrerfc nx_Derfc
#define nx_cErerfc nx_Eerfc
#define nx_cQrerfc nx_Qerfc

#if defined(KS_HAVE_erfcf128)
  #undef nx_Qerfc
  #define nx_Qerfc erfcf128
#elif defined(KS_HAVE_erfcl)
  #undef nx_Qerfc
  #define nx_Qerfc erfcl
#endif
#if defined(KS_HAVE_erfcl)
  #undef nx_Eerfc
  #define nx_Eerfc erfcl
#endif
#if defined(KS_HAVE_erfcf)
  #undef nx_Ferfc
  #define nx_Ferfc erfcf
#endif

/* Function: tgamma */

#define nx_Ftgamma tgamma
#define nx_Dtgamma tgamma
#define nx_Etgamma tgamma
#define nx_Qtgamma tgamma

#define nx_cFrtgamma nx_Ftgamma
#define nx_cDrtgamma nx_Dtgamma
#define nx_cErtgamma nx_Etgamma
#define nx_cQrtgamma nx_Qtgamma

#if defined(KS_HAVE_tgammaf128)
  #undef nx_Qtgamma
  #define nx_Qtgamma tgammaf128
#elif defined(KS_HAVE_tgammal)
  #undef nx_Qtgamma
  #define nx_Qtgamma tgammal
#endif
#if defined(KS_HAVE_tgammal)
  #undef nx_Etgamma
  #define nx_Etgamma tgammal
#endif
#if defined(KS_HAVE_tgammaf)
  #undef nx_Ftgamma
  #define nx_Ftgamma tgammaf
#endif

/* Function: atan2 */

#define nx_Fatan2 atan2
#define nx_Datan2 atan2
#define nx_Eatan2 atan2
#define nx_Qatan2 atan2

#define nx_cFratan2 nx_Fatan2
#define nx_cDratan2 nx_Datan2
#define nx_cEratan2 nx_Eatan2
#define nx_cQratan2 nx_Qatan2

#if defined(KS_HAVE_atan2f128)
  #undef nx_Qatan2
  #define nx_Qatan2 atan2f128
#elif defined(KS_HAVE_atan2l)
  #undef nx_Qatan2
  #define nx_Qatan2 atan2l
#endif
#if defined(KS_HAVE_atan2l)
  #undef nx_Eatan2
  #define nx_Eatan2 atan2l
#endif
#if defined(KS_HAVE_atan2f)
  #undef nx_Fatan2
  #define nx_Fatan2 atan2f
#endif

/* Function: sin */

#define nx_Fsin sin
#define nx_Dsin sin
#define nx_Esin sin
#define nx_Qsin sin

#define nx_cFrsin nx_Fsin
#define nx_cDrsin nx_Dsin
#define nx_cErsin nx_Esin
#define nx_cQrsin nx_Qsin

#if defined(KS_HAVE_sinf128)
  #undef nx_Qsin
  #define nx_Qsin sinf128
#elif defined(KS_HAVE_sinl)
  #undef nx_Qsin
  #define nx_Qsin sinl
#endif
#if defined(KS_HAVE_sinl)
  #undef nx_Esin
  #define nx_Esin sinl
#endif
#if defined(KS_HAVE_sinf)
  #undef nx_Fsin
  #define nx_Fsin sinf
#endif

/* Function: cos */

#define nx_Fcos cos
#define nx_Dcos cos
#define nx_Ecos cos
#define nx_Qcos cos

#define nx_cFrcos nx_Fcos
#define nx_cDrcos nx_Dcos
#define nx_cErcos nx_Ecos
#define nx_cQrcos nx_Qcos

#if defined(KS_HAVE_cosf128)
  #undef nx_Qcos
  #define nx_Qcos cosf128
#elif defined(KS_HAVE_cosl)
  #undef nx_Qcos
  #define nx_Qcos cosl
#endif
#if defined(KS_HAVE_cosl)
  #undef nx_Ecos
  #define nx_Ecos cosl
#endif
#if defined(KS_HAVE_cosf)
  #undef nx_Fcos
  #define nx_Fcos cosf
#endif

/* Function: tan */

#define nx_Ftan tan
#define nx_Dtan tan
#define nx_Etan tan
#define nx_Qtan tan

#define nx_cFrtan nx_Ftan
#define nx_cDrtan nx_Dtan
#define nx_cErtan nx_Etan
#define nx_cQrtan nx_Qtan

#if defined(KS_HAVE_tanf128)
  #undef nx_Qtan
  #define nx_Qtan tanf128
#elif defined(KS_HAVE_tanl)
  #undef nx_Qtan
  #define nx_Qtan tanl
#endif
#if defined(KS_HAVE_tanl)
  #undef nx_Etan
  #define nx_Etan tanl
#endif
#if defined(KS_HAVE_tanf)
  #undef nx_Ftan
  #define nx_Ftan tanf
#endif

/* Function: asin */

#define nx_Fasin asin
#define nx_Dasin asin
#define nx_Easin asin
#define nx_Qasin asin

#define nx_cFrasin nx_Fasin
#define nx_cDrasin nx_Dasin
#define nx_cErasin nx_Easin
#define nx_cQrasin nx_Qasin

#if defined(KS_HAVE_asinf128)
  #undef nx_Qasin
  #define nx_Qasin asinf128
#elif defined(KS_HAVE_asinl)
  #undef nx_Qasin
  #define nx_Qasin asinl
#endif
#if defined(KS_HAVE_asinl)
  #undef nx_Easin
  #define nx_Easin asinl
#endif
#if defined(KS_HAVE_asinf)
  #undef nx_Fasin
  #define nx_Fasin asinf
#endif

/* Function: acos */

#define nx_Facos acos
#define nx_Dacos acos
#define nx_Eacos acos
#define nx_Qacos acos

#define nx_cFracos nx_Facos
#define nx_cDracos nx_Dacos
#define nx_cEracos nx_Eacos
#define nx_cQracos nx_Qacos

#if defined(KS_HAVE_acosf128)
  #undef nx_Qacos
  #define nx_Qacos acosf128
#elif defined(KS_HAVE_acosl)
  #undef nx_Qacos
  #define nx_Qacos acosl
#endif
#if defined(KS_HAVE_acosl)
  #undef nx_Eacos
  #define nx_Eacos acosl
#endif
#if defined(KS_HAVE_acosf)
  #undef nx_Facos
  #define nx_Facos acosf
#endif

/* Function: atan */

#define nx_Fatan atan
#define nx_Datan atan
#define nx_Eatan atan
#define nx_Qatan atan

#define nx_cFratan nx_Fatan
#define nx_cDratan nx_Datan
#define nx_cEratan nx_Eatan
#define nx_cQratan nx_Qatan

#if defined(KS_HAVE_atanf128)
  #undef nx_Qatan
  #define nx_Qatan atanf128
#elif defined(KS_HAVE_atanl)
  #undef nx_Qatan
  #define nx_Qatan atanl
#endif
#if defined(KS_HAVE_atanl)
  #undef nx_Eatan
  #define nx_Eatan atanl
#endif
#if defined(KS_HAVE_atanf)
  #undef nx_Fatan
  #define nx_Fatan atanf
#endif

/* Function: sinh */

#define nx_Fsinh sinh
#define nx_Dsinh sinh
#define nx_Esinh sinh
#define nx_Qsinh sinh

#define nx_cFrsinh nx_Fsinh
#define nx_cDrsinh nx_Dsinh
#define nx_cErsinh nx_Esinh
#define nx_cQrsinh nx_Qsinh

#if defined(KS_HAVE_sinhf128)
  #undef nx_Qsinh
  #define nx_Qsinh sinhf128
#elif defined(KS_HAVE_sinhl)
  #undef nx_Qsinh
  #define nx_Qsinh sinhl
#endif
#if defined(KS_HAVE_sinhl)
  #undef nx_Esinh
  #define nx_Esinh sinhl
#endif
#if defined(KS_HAVE_sinhf)
  #undef nx_Fsinh
  #define nx_Fsinh sinhf
#endif

/* Function: cosh */

#define nx_Fcosh cosh
#define nx_Dcosh cosh
#define nx_Ecosh cosh
#define nx_Qcosh cosh

#define nx_cFrcosh nx_Fcosh
#define nx_cDrcosh nx_Dcosh
#define nx_cErcosh nx_Ecosh
#define nx_cQrcosh nx_Qcosh

#if defined(KS_HAVE_coshf128)
  #undef nx_Qcosh
  #define nx_Qcosh coshf128
#elif defined(KS_HAVE_coshl)
  #undef nx_Qcosh
  #define nx_Qcosh coshl
#endif
#if defined(KS_HAVE_coshl)
  #undef nx_Ecosh
  #define nx_Ecosh coshl
#endif
#if defined(KS_HAVE_coshf)
  #undef nx_Fcosh
  #define nx_Fcosh coshf
#endif

/* Function: tanh */

#define nx_Ftanh tanh
#define nx_Dtanh tanh
#define nx_Etanh tanh
#define nx_Qtanh tanh

#define nx_cFrtanh nx_Ftanh
#define nx_cDrtanh nx_Dtanh
#define nx_cErtanh nx_Etanh
#define nx_cQrtanh nx_Qtanh

#if defined(KS_HAVE_tanhf128)
  #undef nx_Qtanh
  #define nx_Qtanh tanhf128
#elif defined(KS_HAVE_tanhl)
  #undef nx_Qtanh
  #define nx_Qtanh tanhl
#endif
#if defined(KS_HAVE_tanhl)
  #undef nx_Etanh
  #define nx_Etanh tanhl
#endif
#if defined(KS_HAVE_tanhf)
  #undef nx_Ftanh
  #define nx_Ftanh tanhf
#endif

/* Function: asinh */

#define nx_Fasinh asinh
#define nx_Dasinh asinh
#define nx_Easinh asinh
#define nx_Qasinh asinh

#define nx_cFrasinh nx_Fasinh
#define nx_cDrasinh nx_Dasinh
#define nx_cErasinh nx_Easinh
#define nx_cQrasinh nx_Qasinh

#if defined(KS_HAVE_asinhf128)
  #undef nx_Qasinh
  #define nx_Qasinh asinhf128
#elif defined(KS_HAVE_asinhl)
  #undef nx_Qasinh
  #define nx_Qasinh asinhl
#endif
#if defined(KS_HAVE_asinhl)
  #undef nx_Easinh
  #define nx_Easinh asinhl
#endif
#if defined(KS_HAVE_asinhf)
  #undef nx_Fasinh
  #define nx_Fasinh asinhf
#endif

/* Function: acosh */

#define nx_Facosh acosh
#define nx_Dacosh acosh
#define nx_Eacosh acosh
#define nx_Qacosh acosh

#define nx_cFracosh nx_Facosh
#define nx_cDracosh nx_Dacosh
#define nx_cEracosh nx_Eacosh
#define nx_cQracosh nx_Qacosh

#if defined(KS_HAVE_acoshf128)
  #undef nx_Qacosh
  #define nx_Qacosh acoshf128
#elif defined(KS_HAVE_acoshl)
  #undef nx_Qacosh
  #define nx_Qacosh acoshl
#endif
#if defined(KS_HAVE_acoshl)
  #undef nx_Eacosh
  #define nx_Eacosh acoshl
#endif
#if defined(KS_HAVE_acoshf)
  #undef nx_Facosh
  #define nx_Facosh acoshf
#endif

/* Function: atanh */

#define nx_Fatanh atanh
#define nx_Datanh atanh
#define nx_Eatanh atanh
#define nx_Qatanh atanh

#define nx_cFratanh nx_Fatanh
#define nx_cDratanh nx_Datanh
#define nx_cEratanh nx_Eatanh
#define nx_cQratanh nx_Qatanh

#if defined(KS_HAVE_atanhf128)
  #undef nx_Qatanh
  #define nx_Qatanh atanhf128
#elif defined(KS_HAVE_atanhl)
  #undef nx_Qatanh
  #define nx_Qatanh atanhl
#endif
#if defined(KS_HAVE_atanhl)
  #undef nx_Eatanh
  #define nx_Eatanh atanhl
#endif
#if defined(KS_HAVE_atanhf)
  #undef nx_Fatanh
  #define nx_Fatanh atanhf
#endif


/** Include generated functions **/

#define NXK_DO_C
#define NXK_FILE "ks/nxk.cf.kern.h"
#include <ks/nxk.h>

#endif /* NXI_H__ */
