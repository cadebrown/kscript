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

static KS_TFUNC(M, fft) {
    kso x, r = KSO_NONE;
    kso oaxes = KSO_NONE;
    KS_ARGS("x ?axes ?r", &x, &oaxes, &r);

    nx_t vX, vR;
    kso rX, rR;

    if (!nx_get(x, NULL, &vX, &rX)) {
        return NULL;
    }

    if (vX.rank < 1) {
        KS_THROW(kst_SizeError, "Expected argument to be at least 1-D");
        KS_NDECREF(rX);
        return NULL;
    }

    int naxes;
    int axes[NX_MAXRANK];
    if (!nx_as_axes(vX, oaxes, &naxes, axes)) {
        KS_NDECREF(rX);
        return NULL;
    }

    if (r == KSO_NONE) {
        /* Generate output */
        nx_dtype dtype = nx_complextype(vX.dtype);
        if (!dtype) {
            KS_DECREF(rX);
            return NULL;
        }
        
        nx_t shape = vX;

        r = (kso)nx_array_newc(nxt_array, NULL, dtype, shape.rank, shape.shape, NULL);
        if (!r) {
            KS_NDECREF(rX);
            return NULL;
        }
    } else {
        KS_INCREF(r);
    }

    if (!nx_get(r, NULL, &vR, &rR)) {
        KS_NDECREF(rX);
        KS_DECREF(r);
        return NULL;
    }
    nx_dtype odt = nx_complextype(vX.dtype);
    if (!odt) {
        KS_DECREF(rX);
        KS_DECREF(rR);
        KS_DECREF(r);
        return NULL;
    }
    nxfft_plan plan = nxfft_make(odt, vX.rank, vX.shape, false);
    if (!plan) {
        KS_NDECREF(rX);
        KS_NDECREF(rR);
        KS_DECREF(r);
        return NULL;
    }

    if (!nxfft_exec(vX, vR, plan)) {
        KS_NDECREF(rX);
        KS_NDECREF(rR);
        KS_DECREF(r);
        KS_DECREF(plan);
        return NULL;
    }

    KS_NDECREF(rX);
    KS_NDECREF(rR);
    KS_DECREF(plan);
    return r;
}

static KS_TFUNC(M, ifft) {
    kso x, r = KSO_NONE;
    kso oaxes = KSO_NONE;
    KS_ARGS("x ?axes ?r", &x, &oaxes, &r);

    nx_t vX, vR;
    kso rX, rR;

    if (!nx_get(x, NULL, &vX, &rX)) {
        return NULL;
    }

    if (vX.rank < 1) {
        KS_THROW(kst_SizeError, "Expected argument to be at least 1-D");
        KS_NDECREF(rX);
        return NULL;
    }

    int naxes;
    int axes[NX_MAXRANK];
    if (!nx_as_axes(vX, oaxes, &naxes, axes)) {
        KS_NDECREF(rX);
        return NULL;
    }

    if (r == KSO_NONE) {
        /* Generate output */
        nx_dtype dtype = nx_complextype(vX.dtype);
        if (!dtype) {
            KS_DECREF(rX);
            return NULL;
        }

        nx_t shape = vX;
        
        r = (kso)nx_array_newc(nxt_array, NULL, dtype, shape.rank, shape.shape, NULL);
        if (!r) {
            KS_NDECREF(rX);
            return NULL;
        }
    } else {
        KS_INCREF(r);
    }

    if (!nx_get(r, NULL, &vR, &rR)) {
        KS_NDECREF(rX);
        KS_DECREF(r);
        return NULL;
    }
    nx_dtype odt = nx_complextype(vX.dtype);
    if (!odt) {
        KS_DECREF(rX);
        KS_DECREF(rR);
        KS_DECREF(r);
        return NULL;
    }
    nxfft_plan plan = nxfft_make(odt, vX.rank, vX.shape, true);
    if (!plan) {
        KS_NDECREF(rX);
        KS_NDECREF(rR);
        KS_DECREF(r);
        return NULL;
    }

    if (!nxfft_exec(vX, vR, plan)) {
        KS_NDECREF(rX);
        KS_NDECREF(rR);
        KS_DECREF(r);
        KS_DECREF(plan);
        return NULL;
    }

    KS_NDECREF(rX);
    KS_NDECREF(rR);
    KS_DECREF(plan);
    return r;
}


/* Export */

ks_module _ksi_nx_fft() {
    _ksi_nx_fft_plan();

    ks_module res = ks_module_new(M_NAME, KS_BIMOD_SRC, "FFT", KS_IKV(

        /* Types */
        {"plan",                   KS_NEWREF(nxfftt_plan)},

        /* Functions */
        {"fft",                    ksf_wrap(M_fft_, M_NAME ".fft(x, axes=none)", "Compute forward FFT")},
        {"ifft",                   ksf_wrap(M_ifft_, M_NAME ".ifft(x, axes=none)", "Compute inverse FFT")},

    ));

    return res;
}
