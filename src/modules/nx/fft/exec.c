/* exec.c - execute FFT plan
 * 
 *
 * @author:    Cade Brown <cade@kscript.org>
 */
#include <ks/impl.h>
#include <ks/nxt.h>


/* Extra information */
struct extra_data {
    nxfft_plan plan;
};


#define KERN_FUNC(_name) NXK_PASTE(kern_BFLY_, _name)

#define NXK_DO_C
#define NXK_FILE "fft.1D_BFLY.kern"
#define K_NAME "fft.1D_BFLY"
#include <ks/nxk.h>
#undef K_NAME
#undef KERN_FUNC

#define KERN_FUNC(_name) NXK_PASTE(kern_BLUE_, _name)

#define NXK_DO_C
#define NXK_FILE "fft.1D_BLUE.kern"
#define K_NAME "fft.1D_BLUE"
#include <ks/nxk.h>
#undef K_NAME
#undef KERN_FUNC

#define K_NAME "fft"




/* FFTW3 wrapper */
#ifdef KS_HAVE_fftw3
static int kern_FFTW3(int nargs, nx_t* args, int rank, ks_size_t* shape, void* extra) {
    nx_t X = args[0], R = args[1];
    nxfft_plan p = ((struct extra_data*)extra)->plan;

    if (!nx_cast(X, p->kND_FFTW3.tmp)) {
        return -1;
    }

    if (false) {}
#ifdef KS_HAVE_fftw3f
    else if (p->kND_FFTW3.planf) {
        fftwf_execute(p->kND_FFTW3.planf);
    }
#endif
#ifdef KS_HAVE_fftw3l
    else if (p->kND_FFTW3.planl) {
        fftwl_execute(p->kND_FFTW3.planl);
    }
#endif
#ifdef KS_HAVE_fftw3q
    else if (p->kND_FFTW3.planq) {
        fftwq_execute(p->kND_FFTW3.planq);
    }
#endif
    else {
        fftw_execute(p->kND_FFTW3.plan);
    }

    if (p->is_inv) {
        /* Scale and divide */
        nx_s64 coef = 1;
        int i;
        for (i = 0; i < p->rank; ++i) {
            coef *= p->shape[i];
        }

        if (!nx_div(p->kND_FFTW3.tmp, nx_make((nx_s64[]){ coef }, nxd_s64, 0, NULL, NULL), R)) {
            return -1;
        }
    } else {
        /* Don't scale */
        if (!nx_cast(p->kND_FFTW3.tmp, R)) {
            return -1;
        }
    }

    return 0;
}

#endif


bool nxfft_exec(nx_t X, nx_t R, int naxes, int* axes, nxfft_plan plan, bool is_inv) {
    if (R.dtype->kind != NX_DTYPE_COMPLEX) {
        KS_THROW(kst_TypeError, "FFT only defined for complex outputs");
        return false;
    }

    int i;
    if (!plan) {
        /* Generate plan */
        ks_size_t Tshape[NX_MAXRANK];
        for (i = 0; i < naxes; ++i) {
            Tshape[i] = X.shape[axes[i]];
        }
        plan = nxfft_make(R.dtype, naxes, Tshape, is_inv);
    } else {
        KS_INCREF(plan);
    }

    if (naxes != plan->rank) {
        KS_THROW(kst_TypeError, "Unsupported ranks for kernel '%s': %i, %i, plan.rank=%i", K_NAME, X.rank, R.rank, plan->rank);
        KS_DECREF(plan);
        return NULL;
    }

    struct extra_data ed;
    ed.plan = plan;

    if (plan->kind == NXFFT_1D_DENSE) {
        /* Do matrix-vector product */
        bool res = nxla_matmulv(plan->k1D_DENSE.W, X, R);
        KS_DECREF(plan);
        return res;
    } else if (plan->kind == NXFFT_1D_BFLY) {
        /* Set to 'R', as the FFT kernel mutates it */
        if (!nx_cast(X, R)) {
            KS_DECREF(plan);
            return false;
        }

        nx_t sR = nx_sinkaxes(R, naxes, axes);

        if (false) {}
        #define LOOP(TYPE, NAME) else if (R.dtype == nxd_##NAME) { \
            bool res = !nx_apply_Nd(kern_BFLY_##NAME, 1, (nx_t[]){ sR }, 1, NULL, &ed); \
            KS_DECREF(plan); \
            return res; \
        }
        NXT_PASTE_C(LOOP);
        #undef LOOP
    } else if (plan->kind == NXFFT_1D_BLUE) {
        /* Set to 'R', as the FFT kernel mutates it */
        if (!nx_cast(X, R)) {
            return false;
        }
        nx_t sR = nx_sinkaxes(R, naxes, axes);
        
        if (false) {}
        #define LOOP(TYPE, NAME) else if (R.dtype == nxd_##NAME) { \
            bool res = !nx_apply_Nd(kern_BLUE_##NAME, 1, (nx_t[]){ sR }, 1, NULL, &ed); \
            KS_DECREF(plan); \
            return res; \
        }
        NXT_PASTE_C(LOOP);
        #undef LOOP
    } else if (plan->kind == NXFFT_ND_DEFAULT) {
        if (!nx_cast(X, R)) {
            KS_DECREF(plan);
            return false;
        }

        int i;
        for (i = 0; i < plan->rank; ++i) {
            if (!nxfft_exec(R, R, 1, (int[]){ axes[i] }, plan->kND_DEFAULT.plans[i], false)) {
                KS_DECREF(plan);
                return false;
            }
        }
        KS_DECREF(plan);
        return true;

    } else if (plan->kind == NXFFT_ND_FFTW3) {
#ifdef KS_HAVE_fftw3
        nx_t sX = nx_sinkaxes(X, naxes, axes);
        nx_t sR = nx_sinkaxes(R, naxes, axes);
        bool res = !nx_apply_Nd(kern_FFTW3, 2, (nx_t[]){ sX, sR }, plan->rank, NULL, &ed);
        KS_DECREF(plan);
        return res;
#endif
    assert(false);
    }

    KS_DECREF(plan);
    KS_THROW(kst_TypeError, "Unsupported types for kernel '%s': %R, %R", K_NAME, X.dtype, R.dtype);
    return false;
}
