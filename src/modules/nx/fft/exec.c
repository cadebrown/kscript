/* exec.c - execute FFT plan
 * 
 *
 * @author:    Cade Brown <cade@kscript.org>
 */
#include <ks/impl.h>
#include <ks/nxt.h>

#define K_NAME "fft"

/* Extra information */
struct extra_data {
    nxfft_plan plan;
};


/* Access Macros */
#define X_(_i) (pX + sX * (_i))
#define R_(_i) (pR + sR * (_i))


/** Butterfly Transform **/

#define LOOPC(TYPE, NAME) static int kern_BFLY_##NAME(int NUM, nx_t* args, void* extra) { \
    assert(NUM == 1); \
    nx_t R = args[0]; \
    nxfft_plan p = ((struct extra_data*)extra)->plan; \
    assert(p->rank == 1); \
    ks_size_t N = p->shape[0]; \
    assert((N & (N - 1)) == 0); \
    assert(N == R.shape[0]); \
    ks_uint \
        pR = (ks_uint)R.data  \
    ; \
    ks_cint \
        sR = R.strides[0]  \
    ; \
    TYPE t, u, v; \
    TYPE* W = p->k1D_BFLY.W.data; \
    assert(p->k1D_BFLY.W.dtype == R.dtype); \
    ks_cint i, j = 0, k; \
    ks_size_t m; /* Current sub-transform size (inner size) */ \
    /* Do bit-reversal */ \
    for (i = 1; i < N; ++i) { \
        ks_size_t b = N >> 1; \
        while (j >= b) { \
            j -= b; \
            b >>= 1; \
        } \
        j += b; \
        /* Swap if first matching */ \
        if (j > i) { \
            t = *(TYPE*)R_(i); \
            *(TYPE*)R_(i) = *(TYPE*)R_(j); \
            *(TYPE*)R_(j) = t; \
        } \
    } \
    \
    for (m = 1; m <= N; m *= 2) { \
        /* Do butterfly transform of length 'm' */ \
        ks_size_t m2 = m / 2; \
        /* How many steps in the twiddle table per each entry? */ \
        ks_size_t step = N / m; \
        /* 'j' index step */ \
        ks_size_t jstep = step * m; \
        for (i = 0; i < m2; ++i) { \
            /* W[i], current root of unity (precomputed) */ \
            TYPE Wi = W[i * N / m]; \
            /*nx_getstr(ksos_stdout, nx_make(&Wi, nxd_##NAME, 0, NULL, NULL)); */\
            for (j = i; j < jstep + i; j += m) { \
                /* Inner-most butterfly */ \
                u = *(TYPE*)R_(j); \
                t = *(TYPE*)R_(j + m2); \
                v.re = t.re*Wi.re - t.im*Wi.im; \
                v.im = t.re*Wi.im + t.im*Wi.re; \
                \
                (*(TYPE*)R_(j)).re = u.re + v.re; \
                (*(TYPE*)R_(j)).im = u.im + v.im; \
                (*(TYPE*)R_(j + m2)).re = u.re - v.re; \
                (*(TYPE*)R_(j + m2)).im = u.im - v.im; \
            } \
        } \
    } \
    /* Now, reverse some columns */ \
    if (p->is_inv) { \
        (*(TYPE*)R_(0)).re /= N; \
        (*(TYPE*)R_(0)).im /= N; \
        for (i = 1; 2 * i <= N; ++i) { \
            TYPE tmp = *(TYPE*)R_(i); \
            (*(TYPE*)R_(i)).re = (*(TYPE*)R_(N - i)).re / N; \
            (*(TYPE*)R_(i)).im = (*(TYPE*)R_(N - i)).im / N; \
            (*(TYPE*)R_(N - i)).re = tmp.re / N; \
            (*(TYPE*)R_(N - i)).im = tmp.im / N; \
        } \
    } else { \
    } \
    return 0; \
}


NXT_PASTE_C(LOOPC);
#undef LOOPC


bool nxfft_exec(nx_t X, nx_t R, nxfft_plan plan) {
    if (R.dtype->kind != NX_DTYPE_COMPLEX) {
        KS_THROW(kst_TypeError, "FFT only defined for complex outputs");
        return false;
    }

    struct extra_data ed;
    ed.plan = plan;

    if (plan->kind == NXFFT_1D_DENSE) {
        /* Do matrix-vector product */
        bool res = nxla_matmulv(plan->k1D_DENSE.W, X, R);
        return res;
    } else if (plan->kind == NXFFT_1D_BFLY) {

        /* Set to 'R', as the FFT kernel mutates it */
        if (!nx_cast(X, R)) {
            return false;
        }

        #define LOOP(NAME) do { \
            bool res = !nx_apply_Nd(kern_BFLY_##NAME, 1, (nx_t[]){ R }, 1, &ed); \
            return res; \
        } while (0);

        NXT_FOR_C(R.dtype, LOOP);
        #undef LOOP
    }

    KS_THROW(kst_TypeError, "Unsupported types for kernel '%s': %R, %R", K_NAME, X.dtype, R.dtype);
    return false;
}
