/* ks/nxm.h - NumeriX macro library
 * 
 * Used internally to disambiguate macros for different operations
 * 
 * 
 * You can use macros like 'nx_Qsin()' for 'fp128' sign
 * 
 * For complex types, there is an alias for the function starting with 'r' for the real component, i.e.:
 *  nx_cDrsin == nx_Dsin
 * 
 * @author:    Cade Brown <cade@kscript.org>
 */

#pragma once
#ifndef KSNXM_H__
#define KSNXM_H__

#include <ks/nx.h>

/* Function: strfrom */

#define nx_Hstrfrom strfromd
#define nx_Fstrfrom strfromd
#define nx_Dstrfrom strfromd
#define nx_Lstrfrom strfromd
#define nx_Qstrfrom strfromd

#define nx_cHrstrfrom nx_Hstrfrom
#define nx_cFrstrfrom nx_Fstrfrom
#define nx_cDrstrfrom nx_Dstrfrom
#define nx_cLrstrfrom nx_Lstrfrom
#define nx_cQrstrfrom nx_Qstrfrom

#if defined(KS_HAVE_strfromf128)
  #undef nx_Qstrfrom
  #define nx_Qstrfrom strfromf128
#elif defined(KS_HAVE_strfroml)
  #undef nx_Qstrfrom
  #define nx_Qstrfrom strfroml
#endif
#if defined(KS_HAVE_strfroml)
  #undef nx_Lstrfrom
  #define nx_Lstrfrom strfroml
#endif
#if defined(KS_HAVE_strfromf)
  #undef nx_Fstrfrom
  #define nx_Fstrfrom strfromf
  #undef nx_Hstrfrom
  #define nx_Hstrfrom strfromf
#endif

/* Function: fmin */

#define nx_Hfmin fmin
#define nx_Ffmin fmin
#define nx_Dfmin fmin
#define nx_Lfmin fmin
#define nx_Qfmin fmin

#define nx_cHrfmin nx_Hfmin
#define nx_cFrfmin nx_Ffmin
#define nx_cDrfmin nx_Dfmin
#define nx_cLrfmin nx_Lfmin
#define nx_cQrfmin nx_Qfmin

#if defined(KS_HAVE_fminf128)
  #undef nx_Qfmin
  #define nx_Qfmin fminf128
#elif defined(KS_HAVE_fminl)
  #undef nx_Qfmin
  #define nx_Qfmin fminl
#endif
#if defined(KS_HAVE_fminl)
  #undef nx_Lfmin
  #define nx_Lfmin fminl
#endif
#if defined(KS_HAVE_fminf)
  #undef nx_Ffmin
  #define nx_Ffmin fminf
  #undef nx_Hfmin
  #define nx_Hfmin fminf
#endif

/* Function: fmax */

#define nx_Hfmax fmax
#define nx_Ffmax fmax
#define nx_Dfmax fmax
#define nx_Lfmax fmax
#define nx_Qfmax fmax

#define nx_cHrfmax nx_Hfmax
#define nx_cFrfmax nx_Ffmax
#define nx_cDrfmax nx_Dfmax
#define nx_cLrfmax nx_Lfmax
#define nx_cQrfmax nx_Qfmax

#if defined(KS_HAVE_fmaxf128)
  #undef nx_Qfmax
  #define nx_Qfmax fmaxf128
#elif defined(KS_HAVE_fmaxl)
  #undef nx_Qfmax
  #define nx_Qfmax fmaxl
#endif
#if defined(KS_HAVE_fmaxl)
  #undef nx_Lfmax
  #define nx_Lfmax fmaxl
#endif
#if defined(KS_HAVE_fmaxf)
  #undef nx_Ffmax
  #define nx_Ffmax fmaxf
  #undef nx_Hfmax
  #define nx_Hfmax fmaxf
#endif

/* Function: fabs */

#define nx_Hfabs fabs
#define nx_Ffabs fabs
#define nx_Dfabs fabs
#define nx_Lfabs fabs
#define nx_Qfabs fabs

#define nx_cHrfabs nx_Hfabs
#define nx_cFrfabs nx_Ffabs
#define nx_cDrfabs nx_Dfabs
#define nx_cLrfabs nx_Lfabs
#define nx_cQrfabs nx_Qfabs

#if defined(KS_HAVE_fabsf128)
  #undef nx_Qfabs
  #define nx_Qfabs fabsf128
#elif defined(KS_HAVE_fabsl)
  #undef nx_Qfabs
  #define nx_Qfabs fabsl
#endif
#if defined(KS_HAVE_fabsl)
  #undef nx_Lfabs
  #define nx_Lfabs fabsl
#endif
#if defined(KS_HAVE_fabsf)
  #undef nx_Ffabs
  #define nx_Ffabs fabsf
  #undef nx_Hfabs
  #define nx_Hfabs fabsf
#endif

/* Function: fmod */

#define nx_Hfmod fmod
#define nx_Ffmod fmod
#define nx_Dfmod fmod
#define nx_Lfmod fmod
#define nx_Qfmod fmod

#define nx_cHrfmod nx_Hfmod
#define nx_cFrfmod nx_Ffmod
#define nx_cDrfmod nx_Dfmod
#define nx_cLrfmod nx_Lfmod
#define nx_cQrfmod nx_Qfmod

#if defined(KS_HAVE_fmodf128)
  #undef nx_Qfmod
  #define nx_Qfmod fmodf128
#elif defined(KS_HAVE_fmodl)
  #undef nx_Qfmod
  #define nx_Qfmod fmodl
#endif
#if defined(KS_HAVE_fmodl)
  #undef nx_Lfmod
  #define nx_Lfmod fmodl
#endif
#if defined(KS_HAVE_fmodf)
  #undef nx_Ffmod
  #define nx_Ffmod fmodf
  #undef nx_Hfmod
  #define nx_Hfmod fmodf
#endif

/* Function: ceil */

#define nx_Hceil ceil
#define nx_Fceil ceil
#define nx_Dceil ceil
#define nx_Lceil ceil
#define nx_Qceil ceil

#define nx_cHrceil nx_Hceil
#define nx_cFrceil nx_Fceil
#define nx_cDrceil nx_Dceil
#define nx_cLrceil nx_Lceil
#define nx_cQrceil nx_Qceil

#if defined(KS_HAVE_ceilf128)
  #undef nx_Qceil
  #define nx_Qceil ceilf128
#elif defined(KS_HAVE_ceill)
  #undef nx_Qceil
  #define nx_Qceil ceill
#endif
#if defined(KS_HAVE_ceill)
  #undef nx_Lceil
  #define nx_Lceil ceill
#endif
#if defined(KS_HAVE_ceilf)
  #undef nx_Fceil
  #define nx_Fceil ceilf
  #undef nx_Hceil
  #define nx_Hceil ceilf
#endif

/* Function: floor */

#define nx_Hfloor floor
#define nx_Ffloor floor
#define nx_Dfloor floor
#define nx_Lfloor floor
#define nx_Qfloor floor

#define nx_cHrfloor nx_Hfloor
#define nx_cFrfloor nx_Ffloor
#define nx_cDrfloor nx_Dfloor
#define nx_cLrfloor nx_Lfloor
#define nx_cQrfloor nx_Qfloor

#if defined(KS_HAVE_floorf128)
  #undef nx_Qfloor
  #define nx_Qfloor floorf128
#elif defined(KS_HAVE_floorl)
  #undef nx_Qfloor
  #define nx_Qfloor floorl
#endif
#if defined(KS_HAVE_floorl)
  #undef nx_Lfloor
  #define nx_Lfloor floorl
#endif
#if defined(KS_HAVE_floorf)
  #undef nx_Ffloor
  #define nx_Ffloor floorf
  #undef nx_Hfloor
  #define nx_Hfloor floorf
#endif

/* Function: trunc */

#define nx_Htrunc trunc
#define nx_Ftrunc trunc
#define nx_Dtrunc trunc
#define nx_Ltrunc trunc
#define nx_Qtrunc trunc

#define nx_cHrtrunc nx_Htrunc
#define nx_cFrtrunc nx_Ftrunc
#define nx_cDrtrunc nx_Dtrunc
#define nx_cLrtrunc nx_Ltrunc
#define nx_cQrtrunc nx_Qtrunc

#if defined(KS_HAVE_truncf128)
  #undef nx_Qtrunc
  #define nx_Qtrunc truncf128
#elif defined(KS_HAVE_truncl)
  #undef nx_Qtrunc
  #define nx_Qtrunc truncl
#endif
#if defined(KS_HAVE_truncl)
  #undef nx_Ltrunc
  #define nx_Ltrunc truncl
#endif
#if defined(KS_HAVE_truncf)
  #undef nx_Ftrunc
  #define nx_Ftrunc truncf
  #undef nx_Htrunc
  #define nx_Htrunc truncf
#endif

/* Function: copysign */

#define nx_Hcopysign copysign
#define nx_Fcopysign copysign
#define nx_Dcopysign copysign
#define nx_Lcopysign copysign
#define nx_Qcopysign copysign

#define nx_cHrcopysign nx_Hcopysign
#define nx_cFrcopysign nx_Fcopysign
#define nx_cDrcopysign nx_Dcopysign
#define nx_cLrcopysign nx_Lcopysign
#define nx_cQrcopysign nx_Qcopysign

#if defined(KS_HAVE_copysignf128)
  #undef nx_Qcopysign
  #define nx_Qcopysign copysignf128
#elif defined(KS_HAVE_copysignl)
  #undef nx_Qcopysign
  #define nx_Qcopysign copysignl
#endif
#if defined(KS_HAVE_copysignl)
  #undef nx_Lcopysign
  #define nx_Lcopysign copysignl
#endif
#if defined(KS_HAVE_copysignf)
  #undef nx_Fcopysign
  #define nx_Fcopysign copysignf
  #undef nx_Hcopysign
  #define nx_Hcopysign copysignf
#endif

/* Function: exp */

#define nx_Hexp exp
#define nx_Fexp exp
#define nx_Dexp exp
#define nx_Lexp exp
#define nx_Qexp exp

#define nx_cHrexp nx_Hexp
#define nx_cFrexp nx_Fexp
#define nx_cDrexp nx_Dexp
#define nx_cLrexp nx_Lexp
#define nx_cQrexp nx_Qexp

#if defined(KS_HAVE_expf128)
  #undef nx_Qexp
  #define nx_Qexp expf128
#elif defined(KS_HAVE_expl)
  #undef nx_Qexp
  #define nx_Qexp expl
#endif
#if defined(KS_HAVE_expl)
  #undef nx_Lexp
  #define nx_Lexp expl
#endif
#if defined(KS_HAVE_expf)
  #undef nx_Fexp
  #define nx_Fexp expf
  #undef nx_Hexp
  #define nx_Hexp expf
#endif

/* Function: expm1 */

#define nx_Hexpm1 expm1
#define nx_Fexpm1 expm1
#define nx_Dexpm1 expm1
#define nx_Lexpm1 expm1
#define nx_Qexpm1 expm1

#define nx_cHrexpm1 nx_Hexpm1
#define nx_cFrexpm1 nx_Fexpm1
#define nx_cDrexpm1 nx_Dexpm1
#define nx_cLrexpm1 nx_Lexpm1
#define nx_cQrexpm1 nx_Qexpm1

#if defined(KS_HAVE_expm1f128)
  #undef nx_Qexpm1
  #define nx_Qexpm1 expm1f128
#elif defined(KS_HAVE_expm1l)
  #undef nx_Qexpm1
  #define nx_Qexpm1 expm1l
#endif
#if defined(KS_HAVE_expm1l)
  #undef nx_Lexpm1
  #define nx_Lexpm1 expm1l
#endif
#if defined(KS_HAVE_expm1f)
  #undef nx_Fexpm1
  #define nx_Fexpm1 expm1f
  #undef nx_Hexpm1
  #define nx_Hexpm1 expm1f
#endif

/* Function: log */

#define nx_Hlog log
#define nx_Flog log
#define nx_Dlog log
#define nx_Llog log
#define nx_Qlog log

#define nx_cHrlog nx_Hlog
#define nx_cFrlog nx_Flog
#define nx_cDrlog nx_Dlog
#define nx_cLrlog nx_Llog
#define nx_cQrlog nx_Qlog

#if defined(KS_HAVE_logf128)
  #undef nx_Qlog
  #define nx_Qlog logf128
#elif defined(KS_HAVE_logl)
  #undef nx_Qlog
  #define nx_Qlog logl
#endif
#if defined(KS_HAVE_logl)
  #undef nx_Llog
  #define nx_Llog logl
#endif
#if defined(KS_HAVE_logf)
  #undef nx_Flog
  #define nx_Flog logf
  #undef nx_Hlog
  #define nx_Hlog logf
#endif

/* Function: log1p */

#define nx_Hlog1p log1p
#define nx_Flog1p log1p
#define nx_Dlog1p log1p
#define nx_Llog1p log1p
#define nx_Qlog1p log1p

#define nx_cHrlog1p nx_Hlog1p
#define nx_cFrlog1p nx_Flog1p
#define nx_cDrlog1p nx_Dlog1p
#define nx_cLrlog1p nx_Llog1p
#define nx_cQrlog1p nx_Qlog1p

#if defined(KS_HAVE_log1pf128)
  #undef nx_Qlog1p
  #define nx_Qlog1p log1pf128
#elif defined(KS_HAVE_log1pl)
  #undef nx_Qlog1p
  #define nx_Qlog1p log1pl
#endif
#if defined(KS_HAVE_log1pl)
  #undef nx_Llog1p
  #define nx_Llog1p log1pl
#endif
#if defined(KS_HAVE_log1pf)
  #undef nx_Flog1p
  #define nx_Flog1p log1pf
  #undef nx_Hlog1p
  #define nx_Hlog1p log1pf
#endif

/* Function: sqrt */

#define nx_Hsqrt sqrt
#define nx_Fsqrt sqrt
#define nx_Dsqrt sqrt
#define nx_Lsqrt sqrt
#define nx_Qsqrt sqrt

#define nx_cHrsqrt nx_Hsqrt
#define nx_cFrsqrt nx_Fsqrt
#define nx_cDrsqrt nx_Dsqrt
#define nx_cLrsqrt nx_Lsqrt
#define nx_cQrsqrt nx_Qsqrt

#if defined(KS_HAVE_sqrtf128)
  #undef nx_Qsqrt
  #define nx_Qsqrt sqrtf128
#elif defined(KS_HAVE_sqrtl)
  #undef nx_Qsqrt
  #define nx_Qsqrt sqrtl
#endif
#if defined(KS_HAVE_sqrtl)
  #undef nx_Lsqrt
  #define nx_Lsqrt sqrtl
#endif
#if defined(KS_HAVE_sqrtf)
  #undef nx_Fsqrt
  #define nx_Fsqrt sqrtf
  #undef nx_Hsqrt
  #define nx_Hsqrt sqrtf
#endif

/* Function: cbrt */

#define nx_Hcbrt cbrt
#define nx_Fcbrt cbrt
#define nx_Dcbrt cbrt
#define nx_Lcbrt cbrt
#define nx_Qcbrt cbrt

#define nx_cHrcbrt nx_Hcbrt
#define nx_cFrcbrt nx_Fcbrt
#define nx_cDrcbrt nx_Dcbrt
#define nx_cLrcbrt nx_Lcbrt
#define nx_cQrcbrt nx_Qcbrt

#if defined(KS_HAVE_cbrtf128)
  #undef nx_Qcbrt
  #define nx_Qcbrt cbrtf128
#elif defined(KS_HAVE_cbrtl)
  #undef nx_Qcbrt
  #define nx_Qcbrt cbrtl
#endif
#if defined(KS_HAVE_cbrtl)
  #undef nx_Lcbrt
  #define nx_Lcbrt cbrtl
#endif
#if defined(KS_HAVE_cbrtf)
  #undef nx_Fcbrt
  #define nx_Fcbrt cbrtf
  #undef nx_Hcbrt
  #define nx_Hcbrt cbrtf
#endif

/* Function: hypot */

#define nx_Hhypot hypot
#define nx_Fhypot hypot
#define nx_Dhypot hypot
#define nx_Lhypot hypot
#define nx_Qhypot hypot

#define nx_cHrhypot nx_Hhypot
#define nx_cFrhypot nx_Fhypot
#define nx_cDrhypot nx_Dhypot
#define nx_cLrhypot nx_Lhypot
#define nx_cQrhypot nx_Qhypot

#if defined(KS_HAVE_hypotf128)
  #undef nx_Qhypot
  #define nx_Qhypot hypotf128
#elif defined(KS_HAVE_hypotl)
  #undef nx_Qhypot
  #define nx_Qhypot hypotl
#endif
#if defined(KS_HAVE_hypotl)
  #undef nx_Lhypot
  #define nx_Lhypot hypotl
#endif
#if defined(KS_HAVE_hypotf)
  #undef nx_Fhypot
  #define nx_Fhypot hypotf
  #undef nx_Hhypot
  #define nx_Hhypot hypotf
#endif

/* Function: pow */

#define nx_Hpow pow
#define nx_Fpow pow
#define nx_Dpow pow
#define nx_Lpow pow
#define nx_Qpow pow

#define nx_cHrpow nx_Hpow
#define nx_cFrpow nx_Fpow
#define nx_cDrpow nx_Dpow
#define nx_cLrpow nx_Lpow
#define nx_cQrpow nx_Qpow

#if defined(KS_HAVE_powf128)
  #undef nx_Qpow
  #define nx_Qpow powf128
#elif defined(KS_HAVE_powl)
  #undef nx_Qpow
  #define nx_Qpow powl
#endif
#if defined(KS_HAVE_powl)
  #undef nx_Lpow
  #define nx_Lpow powl
#endif
#if defined(KS_HAVE_powf)
  #undef nx_Fpow
  #define nx_Fpow powf
  #undef nx_Hpow
  #define nx_Hpow powf
#endif

/* Function: erf */

#define nx_Herf erf
#define nx_Ferf erf
#define nx_Derf erf
#define nx_Lerf erf
#define nx_Qerf erf

#define nx_cHrerf nx_Herf
#define nx_cFrerf nx_Ferf
#define nx_cDrerf nx_Derf
#define nx_cLrerf nx_Lerf
#define nx_cQrerf nx_Qerf

#if defined(KS_HAVE_erff128)
  #undef nx_Qerf
  #define nx_Qerf erff128
#elif defined(KS_HAVE_erfl)
  #undef nx_Qerf
  #define nx_Qerf erfl
#endif
#if defined(KS_HAVE_erfl)
  #undef nx_Lerf
  #define nx_Lerf erfl
#endif
#if defined(KS_HAVE_erff)
  #undef nx_Ferf
  #define nx_Ferf erff
  #undef nx_Herf
  #define nx_Herf erff
#endif

/* Function: erfc */

#define nx_Herfc erfc
#define nx_Ferfc erfc
#define nx_Derfc erfc
#define nx_Lerfc erfc
#define nx_Qerfc erfc

#define nx_cHrerfc nx_Herfc
#define nx_cFrerfc nx_Ferfc
#define nx_cDrerfc nx_Derfc
#define nx_cLrerfc nx_Lerfc
#define nx_cQrerfc nx_Qerfc

#if defined(KS_HAVE_erfcf128)
  #undef nx_Qerfc
  #define nx_Qerfc erfcf128
#elif defined(KS_HAVE_erfcl)
  #undef nx_Qerfc
  #define nx_Qerfc erfcl
#endif
#if defined(KS_HAVE_erfcl)
  #undef nx_Lerfc
  #define nx_Lerfc erfcl
#endif
#if defined(KS_HAVE_erfcf)
  #undef nx_Ferfc
  #define nx_Ferfc erfcf
  #undef nx_Herfc
  #define nx_Herfc erfcf
#endif

/* Function: tgamma */

#define nx_Htgamma tgamma
#define nx_Ftgamma tgamma
#define nx_Dtgamma tgamma
#define nx_Ltgamma tgamma
#define nx_Qtgamma tgamma

#define nx_cHrtgamma nx_Htgamma
#define nx_cFrtgamma nx_Ftgamma
#define nx_cDrtgamma nx_Dtgamma
#define nx_cLrtgamma nx_Ltgamma
#define nx_cQrtgamma nx_Qtgamma

#if defined(KS_HAVE_tgammaf128)
  #undef nx_Qtgamma
  #define nx_Qtgamma tgammaf128
#elif defined(KS_HAVE_tgammal)
  #undef nx_Qtgamma
  #define nx_Qtgamma tgammal
#endif
#if defined(KS_HAVE_tgammal)
  #undef nx_Ltgamma
  #define nx_Ltgamma tgammal
#endif
#if defined(KS_HAVE_tgammaf)
  #undef nx_Ftgamma
  #define nx_Ftgamma tgammaf
  #undef nx_Htgamma
  #define nx_Htgamma tgammaf
#endif

/* Function: atan2 */

#define nx_Hatan2 atan2
#define nx_Fatan2 atan2
#define nx_Datan2 atan2
#define nx_Latan2 atan2
#define nx_Qatan2 atan2

#define nx_cHratan2 nx_Hatan2
#define nx_cFratan2 nx_Fatan2
#define nx_cDratan2 nx_Datan2
#define nx_cLratan2 nx_Latan2
#define nx_cQratan2 nx_Qatan2

#if defined(KS_HAVE_atan2f128)
  #undef nx_Qatan2
  #define nx_Qatan2 atan2f128
#elif defined(KS_HAVE_atan2l)
  #undef nx_Qatan2
  #define nx_Qatan2 atan2l
#endif
#if defined(KS_HAVE_atan2l)
  #undef nx_Latan2
  #define nx_Latan2 atan2l
#endif
#if defined(KS_HAVE_atan2f)
  #undef nx_Fatan2
  #define nx_Fatan2 atan2f
  #undef nx_Hatan2
  #define nx_Hatan2 atan2f
#endif

/* Function: sin */

#define nx_Hsin sin
#define nx_Fsin sin
#define nx_Dsin sin
#define nx_Lsin sin
#define nx_Qsin sin

#define nx_cHrsin nx_Hsin
#define nx_cFrsin nx_Fsin
#define nx_cDrsin nx_Dsin
#define nx_cLrsin nx_Lsin
#define nx_cQrsin nx_Qsin

#if defined(KS_HAVE_sinf128)
  #undef nx_Qsin
  #define nx_Qsin sinf128
#elif defined(KS_HAVE_sinl)
  #undef nx_Qsin
  #define nx_Qsin sinl
#endif
#if defined(KS_HAVE_sinl)
  #undef nx_Lsin
  #define nx_Lsin sinl
#endif
#if defined(KS_HAVE_sinf)
  #undef nx_Fsin
  #define nx_Fsin sinf
  #undef nx_Hsin
  #define nx_Hsin sinf
#endif

/* Function: cos */

#define nx_Hcos cos
#define nx_Fcos cos
#define nx_Dcos cos
#define nx_Lcos cos
#define nx_Qcos cos

#define nx_cHrcos nx_Hcos
#define nx_cFrcos nx_Fcos
#define nx_cDrcos nx_Dcos
#define nx_cLrcos nx_Lcos
#define nx_cQrcos nx_Qcos

#if defined(KS_HAVE_cosf128)
  #undef nx_Qcos
  #define nx_Qcos cosf128
#elif defined(KS_HAVE_cosl)
  #undef nx_Qcos
  #define nx_Qcos cosl
#endif
#if defined(KS_HAVE_cosl)
  #undef nx_Lcos
  #define nx_Lcos cosl
#endif
#if defined(KS_HAVE_cosf)
  #undef nx_Fcos
  #define nx_Fcos cosf
  #undef nx_Hcos
  #define nx_Hcos cosf
#endif

/* Function: tan */

#define nx_Htan tan
#define nx_Ftan tan
#define nx_Dtan tan
#define nx_Ltan tan
#define nx_Qtan tan

#define nx_cHrtan nx_Htan
#define nx_cFrtan nx_Ftan
#define nx_cDrtan nx_Dtan
#define nx_cLrtan nx_Ltan
#define nx_cQrtan nx_Qtan

#if defined(KS_HAVE_tanf128)
  #undef nx_Qtan
  #define nx_Qtan tanf128
#elif defined(KS_HAVE_tanl)
  #undef nx_Qtan
  #define nx_Qtan tanl
#endif
#if defined(KS_HAVE_tanl)
  #undef nx_Ltan
  #define nx_Ltan tanl
#endif
#if defined(KS_HAVE_tanf)
  #undef nx_Ftan
  #define nx_Ftan tanf
  #undef nx_Htan
  #define nx_Htan tanf
#endif

/* Function: asin */

#define nx_Hasin asin
#define nx_Fasin asin
#define nx_Dasin asin
#define nx_Lasin asin
#define nx_Qasin asin

#define nx_cHrasin nx_Hasin
#define nx_cFrasin nx_Fasin
#define nx_cDrasin nx_Dasin
#define nx_cLrasin nx_Lasin
#define nx_cQrasin nx_Qasin

#if defined(KS_HAVE_asinf128)
  #undef nx_Qasin
  #define nx_Qasin asinf128
#elif defined(KS_HAVE_asinl)
  #undef nx_Qasin
  #define nx_Qasin asinl
#endif
#if defined(KS_HAVE_asinl)
  #undef nx_Lasin
  #define nx_Lasin asinl
#endif
#if defined(KS_HAVE_asinf)
  #undef nx_Fasin
  #define nx_Fasin asinf
  #undef nx_Hasin
  #define nx_Hasin asinf
#endif

/* Function: acos */

#define nx_Hacos acos
#define nx_Facos acos
#define nx_Dacos acos
#define nx_Lacos acos
#define nx_Qacos acos

#define nx_cHracos nx_Hacos
#define nx_cFracos nx_Facos
#define nx_cDracos nx_Dacos
#define nx_cLracos nx_Lacos
#define nx_cQracos nx_Qacos

#if defined(KS_HAVE_acosf128)
  #undef nx_Qacos
  #define nx_Qacos acosf128
#elif defined(KS_HAVE_acosl)
  #undef nx_Qacos
  #define nx_Qacos acosl
#endif
#if defined(KS_HAVE_acosl)
  #undef nx_Lacos
  #define nx_Lacos acosl
#endif
#if defined(KS_HAVE_acosf)
  #undef nx_Facos
  #define nx_Facos acosf
  #undef nx_Hacos
  #define nx_Hacos acosf
#endif

/* Function: atan */

#define nx_Hatan atan
#define nx_Fatan atan
#define nx_Datan atan
#define nx_Latan atan
#define nx_Qatan atan

#define nx_cHratan nx_Hatan
#define nx_cFratan nx_Fatan
#define nx_cDratan nx_Datan
#define nx_cLratan nx_Latan
#define nx_cQratan nx_Qatan

#if defined(KS_HAVE_atanf128)
  #undef nx_Qatan
  #define nx_Qatan atanf128
#elif defined(KS_HAVE_atanl)
  #undef nx_Qatan
  #define nx_Qatan atanl
#endif
#if defined(KS_HAVE_atanl)
  #undef nx_Latan
  #define nx_Latan atanl
#endif
#if defined(KS_HAVE_atanf)
  #undef nx_Fatan
  #define nx_Fatan atanf
  #undef nx_Hatan
  #define nx_Hatan atanf
#endif

/* Function: sinh */

#define nx_Hsinh sinh
#define nx_Fsinh sinh
#define nx_Dsinh sinh
#define nx_Lsinh sinh
#define nx_Qsinh sinh

#define nx_cHrsinh nx_Hsinh
#define nx_cFrsinh nx_Fsinh
#define nx_cDrsinh nx_Dsinh
#define nx_cLrsinh nx_Lsinh
#define nx_cQrsinh nx_Qsinh

#if defined(KS_HAVE_sinhf128)
  #undef nx_Qsinh
  #define nx_Qsinh sinhf128
#elif defined(KS_HAVE_sinhl)
  #undef nx_Qsinh
  #define nx_Qsinh sinhl
#endif
#if defined(KS_HAVE_sinhl)
  #undef nx_Lsinh
  #define nx_Lsinh sinhl
#endif
#if defined(KS_HAVE_sinhf)
  #undef nx_Fsinh
  #define nx_Fsinh sinhf
  #undef nx_Hsinh
  #define nx_Hsinh sinhf
#endif

/* Function: cosh */

#define nx_Hcosh cosh
#define nx_Fcosh cosh
#define nx_Dcosh cosh
#define nx_Lcosh cosh
#define nx_Qcosh cosh

#define nx_cHrcosh nx_Hcosh
#define nx_cFrcosh nx_Fcosh
#define nx_cDrcosh nx_Dcosh
#define nx_cLrcosh nx_Lcosh
#define nx_cQrcosh nx_Qcosh

#if defined(KS_HAVE_coshf128)
  #undef nx_Qcosh
  #define nx_Qcosh coshf128
#elif defined(KS_HAVE_coshl)
  #undef nx_Qcosh
  #define nx_Qcosh coshl
#endif
#if defined(KS_HAVE_coshl)
  #undef nx_Lcosh
  #define nx_Lcosh coshl
#endif
#if defined(KS_HAVE_coshf)
  #undef nx_Fcosh
  #define nx_Fcosh coshf
  #undef nx_Hcosh
  #define nx_Hcosh coshf
#endif

/* Function: tanh */

#define nx_Htanh tanh
#define nx_Ftanh tanh
#define nx_Dtanh tanh
#define nx_Ltanh tanh
#define nx_Qtanh tanh

#define nx_cHrtanh nx_Htanh
#define nx_cFrtanh nx_Ftanh
#define nx_cDrtanh nx_Dtanh
#define nx_cLrtanh nx_Ltanh
#define nx_cQrtanh nx_Qtanh

#if defined(KS_HAVE_tanhf128)
  #undef nx_Qtanh
  #define nx_Qtanh tanhf128
#elif defined(KS_HAVE_tanhl)
  #undef nx_Qtanh
  #define nx_Qtanh tanhl
#endif
#if defined(KS_HAVE_tanhl)
  #undef nx_Ltanh
  #define nx_Ltanh tanhl
#endif
#if defined(KS_HAVE_tanhf)
  #undef nx_Ftanh
  #define nx_Ftanh tanhf
  #undef nx_Htanh
  #define nx_Htanh tanhf
#endif

/* Function: asinh */

#define nx_Hasinh asinh
#define nx_Fasinh asinh
#define nx_Dasinh asinh
#define nx_Lasinh asinh
#define nx_Qasinh asinh

#define nx_cHrasinh nx_Hasinh
#define nx_cFrasinh nx_Fasinh
#define nx_cDrasinh nx_Dasinh
#define nx_cLrasinh nx_Lasinh
#define nx_cQrasinh nx_Qasinh

#if defined(KS_HAVE_asinhf128)
  #undef nx_Qasinh
  #define nx_Qasinh asinhf128
#elif defined(KS_HAVE_asinhl)
  #undef nx_Qasinh
  #define nx_Qasinh asinhl
#endif
#if defined(KS_HAVE_asinhl)
  #undef nx_Lasinh
  #define nx_Lasinh asinhl
#endif
#if defined(KS_HAVE_asinhf)
  #undef nx_Fasinh
  #define nx_Fasinh asinhf
  #undef nx_Hasinh
  #define nx_Hasinh asinhf
#endif

/* Function: acosh */

#define nx_Hacosh acosh
#define nx_Facosh acosh
#define nx_Dacosh acosh
#define nx_Lacosh acosh
#define nx_Qacosh acosh

#define nx_cHracosh nx_Hacosh
#define nx_cFracosh nx_Facosh
#define nx_cDracosh nx_Dacosh
#define nx_cLracosh nx_Lacosh
#define nx_cQracosh nx_Qacosh

#if defined(KS_HAVE_acoshf128)
  #undef nx_Qacosh
  #define nx_Qacosh acoshf128
#elif defined(KS_HAVE_acoshl)
  #undef nx_Qacosh
  #define nx_Qacosh acoshl
#endif
#if defined(KS_HAVE_acoshl)
  #undef nx_Lacosh
  #define nx_Lacosh acoshl
#endif
#if defined(KS_HAVE_acoshf)
  #undef nx_Facosh
  #define nx_Facosh acoshf
  #undef nx_Hacosh
  #define nx_Hacosh acoshf
#endif

/* Function: atanh */

#define nx_Hatanh atanh
#define nx_Fatanh atanh
#define nx_Datanh atanh
#define nx_Latanh atanh
#define nx_Qatanh atanh

#define nx_cHratanh nx_Hatanh
#define nx_cFratanh nx_Fatanh
#define nx_cDratanh nx_Datanh
#define nx_cLratanh nx_Latanh
#define nx_cQratanh nx_Qatanh

#if defined(KS_HAVE_atanhf128)
  #undef nx_Qatanh
  #define nx_Qatanh atanhf128
#elif defined(KS_HAVE_atanhl)
  #undef nx_Qatanh
  #define nx_Qatanh atanhl
#endif
#if defined(KS_HAVE_atanhl)
  #undef nx_Latanh
  #define nx_Latanh atanhl
#endif
#if defined(KS_HAVE_atanhf)
  #undef nx_Fatanh
  #define nx_Fatanh atanhf
  #undef nx_Hatanh
  #define nx_Hatanh atanhf
#endif

#endif /* KSNXM_H__ */
