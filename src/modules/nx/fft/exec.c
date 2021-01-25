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



/** Bluestein/Chirp-Z Transform **/

#define LOOPC(TYPE, NAME) static int kern_BLUE_##NAME(int NUM, nx_t* args, void* extra) { \
    assert(NUM == 1); \
    nx_t R = args[0]; \
    nxfft_plan p = ((struct extra_data*)extra)->plan; \
    assert(p->rank == 1); \
    ks_size_t N = p->shape[0], M = p->k1D_BLUE.M; \
    assert(N == R.shape[0]); \
    ks_uint \
        pR = (ks_uint)R.data  \
    ; \
    ks_cint \
        sR = R.strides[0]  \
    ; \
    TYPE* tmp = p->k1D_BLUE.tmp.data; \
    TYPE* tmpA = &tmp[0]; \
    TYPE* tmpB = &tmp[M]; \
    TYPE t, u, v; \
    TYPE* Ws = p->k1D_BLUE.Ws.data; \
    assert(p->k1D_BLUE.Ws.dtype == R.dtype); \
    ks_cint i, j, k; \
    for (i = 0; i < M; ++i) { \
        tmpA[i].re = tmpA[i].im = 0; \
        tmpB[i].re = tmpB[i].im = 0; \
    } \
    /* Pre-process step */ \
    for (i = 0; i < N; ++i) { \
        t = *(TYPE*)R_(i); \
        tmpA[i].re = t.re*Ws[i].re - t.im*Ws[i].im; \
        tmpA[i].im = t.re*Ws[i].im + t.im*Ws[i].re; \
    } \
    tmpB[0] = Ws[0]; \
    for (i = 1; i < N; ++i) { \
        t.re = Ws[i].re; \
        t.im = -Ws[i].im; \
        tmpB[i].re = tmpB[M - i].re = t.re; \
        tmpB[i].im = tmpB[M - i].im = t.im; \
    } \
    /* Now, compute M-plans on temporary buffers */ \
    nx_t trA = nx_make(tmpA, nxd_##NAME, 1, (ks_size_t[]){ M }, NULL); \
    nx_t trB = nx_make(tmpB, nxd_##NAME, 1, (ks_size_t[]){ M }, NULL); \
    if (!nxfft_exec(trA, trA, p->k1D_BLUE.planM) || !nxfft_exec(trB, trB, p->k1D_BLUE.planM)) { \
        return -1; \
    } \
    /* Pointwise multiply */ \
    for (i = 0; i < M; ++i) { \
        t = tmpA[i]; \
        tmpA[i].re = t.re*tmpB[i].re - t.im*tmpB[i].im; \
        tmpA[i].im = t.re*tmpB[i].im + t.im*tmpB[i].re; \
    } \
    if (!nxfft_exec(trA, trA, p->k1D_BLUE.planM)) { \
        return -1; \
    } \
    if (p->is_inv) { \
        TYPE##r scl = TYPE##rval(1.0) / (M * N); \
        (*(TYPE*)R_(0)).re = scl * (tmpA[0].re*Ws[0].re - tmpA[0].im*Ws[0].im); \
        (*(TYPE*)R_(0)).im = scl * (tmpA[0].re*Ws[0].im + tmpA[0].im*Ws[0].re); \
        for (i = 1; i < N; ++i) { \
            j = i - N + M; \
            (*(TYPE*)R_(N - i)).re = -scl * (tmpA[j].re*Ws[i].re - tmpA[j].im*Ws[i].im); \
            (*(TYPE*)R_(N - i)).im = -scl * (tmpA[j].re*Ws[i].im + tmpA[j].im*Ws[i].re); \
        } \
    } else { \
        TYPE##r scl = TYPE##rval(1.0) / (M); \
        (*(TYPE*)R_(0)).re = scl * (tmpA[0].re*Ws[0].re - tmpA[0].im*Ws[0].im); \
        (*(TYPE*)R_(0)).im = scl * (tmpA[0].re*Ws[0].im + tmpA[0].im*Ws[0].re); \
        for (i = 1; i < N; ++i) { \
            j = M - i; \
            (*(TYPE*)R_(N - i)).re = scl * (tmpA[j].re*Ws[i].re - tmpA[j].im*Ws[i].im); \
            (*(TYPE*)R_(N - i)).im = scl * (tmpA[j].re*Ws[i].im + tmpA[j].im*Ws[i].re); \
        } \
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

    if (R.rank < plan->rank) {
        KS_THROW(kst_TypeError, "Unsupported ranks for kernel '%s': %i, %i, plan.rank=%i", K_NAME, X.rank, R.rank, plan->rank);
        return NULL;

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
    } else if (plan->kind == NXFFT_1D_BLUE) {
        /* Set to 'R', as the FFT kernel mutates it */
        if (!nx_cast(X, R)) {
            return false;
        }

        #define LOOP(NAME) do { \
            bool res = !nx_apply_Nd(kern_BLUE_##NAME, 1, (nx_t[]){ R }, 1, &ed); \
            return res; \
        } while (0);

        NXT_FOR_C(R.dtype, LOOP);
        #undef LOOP
    } else if (plan->kind == NXFFT_ND_DEFAULT) {
        if (!nx_cast(X, R)) {
            return false;
        }

        int i;
        for (i = 0; i < plan->rank; ++i) {
            nx_t aR = nx_switch_axes(R, i + R.rank - plan->rank, R.rank - 1);
            if (!nxfft_exec(aR, aR, plan->kND_DEFAULT.plans[i])) {
                return false;
            }
        }

        return true;
    }


    KS_THROW(kst_TypeError, "Unsupported types for kernel '%s': %R, %R", K_NAME, X.dtype, R.dtype);
    return false;
}
