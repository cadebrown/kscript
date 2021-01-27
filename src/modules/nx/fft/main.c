/* nx/fft/main.c - 'nx.fft' module
 *
 * 
 * @author:    Cade Brown <cade@kscript.org>
 */
#include <ks/impl.h>
#include <ks/nxt.h>

#define M_NAME "nx.fft"

/* C-API */


/* Module Functions */


/* Templates */

/* Reduction function taking 1 argument, to real conversion */
#define T_FFT1(_name, _isinv) static KS_TFUNC(M, _name) { \
    kso ax, aaxes = KSO_NONE, ar = KSO_NONE; \
    KS_ARGS("x ?axes ?r", &ax, &aaxes, &ar); \
    nx_t x, r; \
    kso xr, rr; \
    if (!nx_get(ax, NULL, &x, &xr)) { \
        return NULL; \
    } \
    int naxes; \
    int axes[NX_MAXRANK]; \
    if (!nx_getaxes(aaxes, x.rank, &naxes, axes)) { \
        KS_NDECREF(xr); \
        return NULL; \
    } \
    ks_size_t Tshape[NX_MAXRANK]; \
    int i; \
    for (i = 0; i < naxes; ++i) { \
        Tshape[i] = x.shape[axes[i]]; \
    } \
    if (ar == KSO_NONE) { \
        nx_dtype dtype = nx_complextype(x.dtype); \
        if (!dtype) { \
            KS_NDECREF(xr); \
            return NULL; \
        } \
        ar = (kso)nx_array_newc(nxt_array, NULL, dtype, x.rank, x.shape, NULL); \
        if (!ar) { \
            KS_NDECREF(xr); \
            return NULL; \
        } \
    } else { \
        KS_INCREF(ar); \
    } \
    if (!nx_get(ar, NULL, &r, &rr)) { \
        KS_NDECREF(xr); \
        KS_DECREF(ar); \
        return NULL; \
    } \
    if (!nxfft_exec(x, r, naxes, axes, NULL, _isinv)) { \
        KS_NDECREF(xr); \
        KS_NDECREF(rr); \
        KS_DECREF(ar); \
        return NULL; \
    } \
    KS_NDECREF(xr); \
    KS_NDECREF(rr); \
    return ar; \
}


T_FFT1(fft, false);
T_FFT1(ifft, true);



/* Export */

ks_module _ksi_nx_fft() {
    _ksi_nx_fft_plan();

    ks_module res = ks_module_new(M_NAME, KS_BIMOD_SRC, "FFT", KS_IKV(

        /* Types */
        {"plan",                   KS_NEWREF(nxfftt_plan)},

        /* Functions */
        {"fft",                    ksf_wrap(M_fft_, M_NAME ".fft(x, axes=none, r=none)", "Compute forward FFT")},
        {"ifft",                   ksf_wrap(M_ifft_, M_NAME ".ifft(x, axes=none, r=none)", "Compute inverse FFT")},

    ));

    return res;
}
