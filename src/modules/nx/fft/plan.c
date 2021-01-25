/* plan.c - implementation of the 'nx.fft.plan' type
 * 
 *
 * @author:    Cade Brown <cade@kscript.org>
 */
#include <ks/impl.h>
#include <ks/m.h>
#include <ks/nxt.h>
#include <ks/nxm.h>

#define T_NAME "nx.fft.plan"

/* Internals */


/* Make 1D dense plan */
static nxfft_plan make_1D_DENSE(nx_dtype dtype, ks_size_t N, bool is_inv) {

    nxfft_plan self = KSO_NEW(nxfft_plan, nxfftt_plan);

    self->rank = 1;
    self->shape[0] = N;

    self->kind = NXFFT_1D_DENSE;
    self->is_inv = is_inv;
    self->k1D_DENSE.W.data = NULL;

    void* data = ks_malloc(N * N * dtype->size);
    self->k1D_DENSE.W = nx_make(data, dtype, 2, (ks_size_t[]){ N, N }, NULL);

    #define LOOP(TYPE, NAME) else if (dtype == nxd_##NAME) { \
        int j; \
        for (j = 0; j < N; ++j) { \
            int k; \
            for (k = 0; k < N; ++k) { \
                TYPE##r theta = (is_inv ? -2.0 : +2.0) * TYPE##rPI * j * k / N; \
                ((TYPE*)(data))[j * N + k].re = TYPE##rcos(theta); \
                ((TYPE*)(data))[j * N + k].im = TYPE##rsin(theta); \
                if (is_inv) { \
                    ((TYPE*)(data))[j * N + k].re /= N; \
                    ((TYPE*)(data))[j * N + k].im /= N; \
                } \
            } \
            /* Inverse transforms need to reverse the columns starting at 1 */ \
            if (is_inv) { \
                int rk; \
                for (k = 1, rk = N - 1; k < rk; ++k, --rk) { \
                    TYPE t = ((TYPE*)(data))[j * N + k]; \
                    ((TYPE*)(data))[j * N + k] = ((TYPE*)(data))[j * N + rk]; \
                    ((TYPE*)(data))[j * N + rk] = t; \
                    \
                } \
            } \
        } \
    }

    if (false) {}
    NXT_PASTE_C(LOOP)
    else {
        KS_DECREF(self);
        KS_THROW(kst_TypeError, "Unsupported type: %R", dtype);
        return NULL;
    }
    #undef LOOP

    return self;
}

static nxfft_plan make_1D_BLUE(nx_dtype dtype, ks_size_t N, bool is_inv) {
    nxfft_plan self = KSO_NEW(nxfft_plan, nxfftt_plan);

    self->rank = 1;
    self->shape[0] = N;

    self->kind = NXFFT_1D_BLUE;
    self->is_inv = is_inv;

    self->k1D_BLUE.Ws.data = NULL;
    self->k1D_BLUE.tmp.data = NULL;
    self->k1D_BLUE.planM = NULL;

    void* data = ks_malloc(N * dtype->size);
    self->k1D_BLUE.Ws = nx_make(data, dtype, 1, (ks_size_t[]){ N }, NULL);

    #define LOOP(TYPE, NAME) else if (dtype == nxd_##NAME) { \
        int j; \
        for (j = 0; j < N; ++j) { \
            /* To avoid roundoff errors, perform modulo here */ \
            int j2 = (j * j) % (N * 2); \
            TYPE##r theta = TYPE##rPI * j2 / N; \
            ((TYPE*)(data))[j].re = TYPE##rcos(theta); \
            ((TYPE*)(data))[j].im = TYPE##rsin(theta); \
        } \
    }

    if (false) {}
    NXT_PASTE_C(LOOP)
    else {
        KS_DECREF(self);
        KS_THROW(kst_TypeError, "Unsupported type: %R", dtype);
        return NULL;
    }
    #undef LOOP

    /* Now, find 'M', smallest power of 2 that is >= 2*N+1 */

    ks_size_t M = 1;
    while (M < 2 * N + 1) {
        if (M >= SIZE_MAX / 2) {
            KS_DECREF(self);
            KS_THROW(kst_SizeError, "Too big for FFT: %u", N);
            return NULL;
        }

        M *= 2;
    }

    /* Create plan for size 'M' */
    self->k1D_BLUE.M = M;
    self->k1D_BLUE.planM = nxfft_make(dtype, 1, (ks_size_t[]){ M }, false);

    /* Create temporary buffer */
    self->k1D_BLUE.tmp = nx_make(ks_malloc(2 * M * dtype->size), dtype, 1, (ks_size_t[]){ 2 * M }, NULL);

    return self;
}


static nxfft_plan make_1D_BFLY(nx_dtype dtype, ks_size_t N, bool is_inv) {
    if (N & (N - 1) != 0) {
        KS_THROW(kst_SizeError, "Invalid BFLY FFT size %u: must be power of 2", N);
        return NULL;
    }

    nxfft_plan self = KSO_NEW(nxfft_plan, nxfftt_plan);

    self->rank = 1;
    self->shape[0] = N;

    self->is_inv = is_inv;

    self->kind = NXFFT_1D_BFLY;
    self->k1D_BFLY.W.data = NULL;

    /* Twiddle table */
    void* data = ks_malloc(N * dtype->size);
    self->k1D_BFLY.W = nx_make(data, dtype, 1, (ks_size_t[]){ N }, NULL);

    #define LOOP(TYPE, NAME) else if (dtype == nxd_##NAME) { \
        int j; \
        for (j = 0; j < N; ++j) { \
            /* To avoid roundoff errors, perform modulo here */ \
            TYPE##r theta = -2.0 * TYPE##rPI * j / N; \
            ((TYPE*)(data))[j].re = TYPE##rcos(theta); \
            ((TYPE*)(data))[j].im = TYPE##rsin(theta); \
        } \
    }

    if (false) {}
    NXT_PASTE_C(LOOP)
    else {
        KS_DECREF(self);
        KS_THROW(kst_TypeError, "Unsupported type: %R", dtype);
        return NULL;
    }
    #undef LOOP

    return self;
}


static nxfft_plan make_ND_DEFAULT(nx_dtype dtype, int rank, ks_size_t* shape, bool is_inv) {
    nxfft_plan self = KSO_NEW(nxfft_plan, nxfftt_plan);

    int i;
    self->rank = rank;
    for (i = 0; i < rank; ++i) {
        self->shape[i] = shape[i];
    }

    self->is_inv = is_inv;

    self->kind = NXFFT_ND_DEFAULT;

    self->kND_DEFAULT.plans = NULL;


    self->kND_DEFAULT.plans = ks_malloc(sizeof(*self->kND_DEFAULT.plans) * rank);
    for (i = 0; i < rank; ++i) {
        self->kND_DEFAULT.plans[i] = NULL;
    }
    for (i = 0; i < rank; ++i) {
        self->kND_DEFAULT.plans[i] = nxfft_make(dtype, 1, &shape[i], is_inv);
        if (!self->kND_DEFAULT.plans[i]) {
            KS_DECREF(self);
            return NULL;
        }
    }

    return self;
}

#ifdef KS_HAVE_fftw3

static nxfft_plan make_ND_FFTW3(nx_dtype dtype, int rank, ks_size_t* shape, bool is_inv) {
    nxfft_plan self = KSO_NEW(nxfft_plan, nxfftt_plan);

    int i;
    int fsz[NX_MAXRANK];
    self->rank = rank;

    nx_dtype idt = nxd_cD;

#ifdef KS_HAVE_fftw3f
    if (dtype == nxd_cF) {
        idt = nxd_cF;
    }
#elif defined(KS_HAVE_fftw3q)
    if (dtype == nxd_cQ) {
        idt = nxd_cQ;
    }
#elif defined(KS_HAVE_fftw3l)
    if (dtype == nxd_cL) {
        return make_ND_FFTW3(dtype, rank, dims, is_inv);
    } else if (dtype == nxd_cQ) {
        return make_ND_FFTW3(dtype, rank, dims, is_inv);
    }
#endif

    //ks_size_t tsz = dtype->size;
    ks_size_t tsz = idt->size;
    for (i = 0; i < rank; ++i) {
        self->shape[i] = shape[i];
        fsz[i] = shape[i];
        tsz *= shape[i];
    }

    self->is_inv = is_inv;

    self->kind = NXFFT_ND_FFTW3;
    self->kND_FFTW3.plan = NULL;

    self->kND_FFTW3.tmp.data = NULL;

    self->kND_FFTW3.tmp = nx_make(
        ks_malloc(tsz),
        idt,
        rank,
        shape,
        NULL
    );

    if (idt == nxd_cF) {
#ifdef KS_HAVE_fftw3f
        self->kND_FFTW3.planf = fftwf_plan_many_dft(rank, fsz, 1,
            self->kND_FFTW3.tmp.data, NULL, 1, 0,
            self->kND_FFTW3.tmp.data, NULL, 1, 0,
            is_inv ? FFTW_FORWARD : FFTW_BACKWARD,
            FFTW_ESTIMATE
        );

        if (!self->kND_FFTW3.planf) {
            KS_DECREF(self);
            KS_THROW(kst_Error, "FFTW3 failed to create plan");
            return NULL;
        }
#else
        assert(false);
#endif

    } else if (idt == nxd_cD) {
        self->kND_FFTW3.plan = fftw_plan_many_dft(rank, fsz, 1,
            self->kND_FFTW3.tmp.data, NULL, 1, 0,
            self->kND_FFTW3.tmp.data, NULL, 1, 0,
            is_inv ? FFTW_FORWARD : FFTW_BACKWARD,
            FFTW_ESTIMATE
        );

        if (!self->kND_FFTW3.plan) {
            KS_DECREF(self);
            KS_THROW(kst_Error, "FFTW3 failed to create plan");
            return NULL;
        }
    } else if (idt == nxd_cL) {
#ifdef KS_HAVE_fftw3l
        self->kND_FFTW3.planl = fftwl_plan_many_dft(rank, fsz, 1,
            self->kND_FFTW3.tmp.data, NULL, 1, 0,
            self->kND_FFTW3.tmp.data, NULL, 1, 0,
            is_inv ? FFTW_FORWARD : FFTW_BACKWARD,
            FFTW_ESTIMATE
        );
        if (!self->kND_FFTW3.planl) {
            KS_DECREF(self);
            KS_THROW(kst_Error, "FFTW3 failed to create plan");
            return NULL;
        }

#else
        assert(false);
#endif
    } else if (idt == nxd_cQ) {
#ifdef KS_HAVE_fftw3q
        self->kND_FFTW3.planq = fftwq_plan_many_dft(rank, fsz, 1,
            self->kND_FFTW3.tmp.data, NULL, 1, 0,
            self->kND_FFTW3.tmp.data, NULL, 1, 0,
            is_inv ? FFTW_FORWARD : FFTW_BACKWARD,
            FFTW_ESTIMATE
        );
        if (!self->kND_FFTW3.planq) {
            KS_DECREF(self);
            KS_THROW(kst_Error, "FFTW3 failed to create plan");
            return NULL;
        }
#else
        assert(false);
#endif
    }

    return self;
}

#endif

nxfft_plan nxfft_make(nx_dtype dtype, int rank, ks_size_t* dims, bool is_inv) {
    if (dtype->kind != NX_DTYPE_COMPLEX) {
        KS_THROW(kst_TypeError, "FFT only defined for complex types");
        return NULL;
    }

    //return make_1D_DENSE(dtype, dims[0], is_inv);
    //return make_1D_BFLY(dtype, dims[0], is_inv);
#ifdef KS_HAVE_fftw3f
    if (dtype == nxd_cF) {
        return make_ND_FFTW3(dtype, rank, dims, is_inv);
    }
#endif

#ifdef KS_HAVE_fftw3
    if (dtype == nxd_cD) {
        return make_ND_FFTW3(dtype, rank, dims, is_inv);
    } else if (dtype == nxd_cF) {
        return make_ND_FFTW3(dtype, rank, dims, is_inv);
    }
#endif

#ifdef KS_HAVE_fftw3q
    if (dtype == nxd_cQ) {
        return make_ND_FFTW3(dtype, rank, dims, is_inv);
    }
#endif
#ifdef KS_HAVE_fftw3l
    if (dtype == nxd_cL) {
        return make_ND_FFTW3(dtype, rank, dims, is_inv);
    } else if (dtype == nxd_cQ) {
        return make_ND_FFTW3(dtype, rank, dims, is_inv);
    }
#endif

    if (rank == 1 && (dims[0] & (dims[0] - 1)) == 0) {
        return make_1D_BFLY(dtype, dims[0], is_inv);
    } else if (rank == 1) {
        return make_1D_BLUE(dtype, dims[0], is_inv);
    } else {
        return make_ND_DEFAULT(dtype, rank, dims, is_inv);
    }
}



/* Type Functions */

static KS_TFUNC(T, free) {
    nxfft_plan self;
    KS_ARGS("self:*", &self, nxfftt_plan);


    if (self->kind == NXFFT_1D_BFLY) {
        ks_free(self->k1D_BFLY.W.data);
    } else if (self->kind == NXFFT_1D_BLUE) {
        ks_free(self->k1D_BLUE.Ws.data);
        ks_free(self->k1D_BLUE.tmp.data);
        KS_NDECREF(self->k1D_BLUE.planM);
    } else if (self->kind == NXFFT_1D_DENSE) {
        ks_free(self->k1D_DENSE.W.data);
    } else if (self->kind == NXFFT_ND_DEFAULT) {
        int i;
        for (i = 0; i < self->rank; ++i) {
            KS_NDECREF(self->kND_DEFAULT.plans[i]);
        }
        ks_free(self->kND_DEFAULT.plans);
    } else if (self->kind == NXFFT_ND_FFTW3) {
#ifdef KS_HAVE_fftw3
        fftw_destroy_plan(self->kND_FFTW3.plan);
        ks_free(self->kND_FFTW3.tmp.data);
#endif

    }

    KSO_DEL(self);

    return KSO_NONE;
}

static KS_TFUNC(T, new) {
    ks_type tp;
    kso obj;
    nx_dtype dtype = nxd_D;
    KS_ARGS("tp:* obj ?dtype:*", &tp, kst_type, &obj, &dtype, nxt_dtype);

    return (kso)nx_array_newo(tp, obj, dtype);
}


/* Export */

static struct ks_type_s tp;
ks_type nxfftt_plan = &tp;

void _ksi_nx_fft_plan() {
    
    _ksinit(nxfftt_plan, kst_object, T_NAME, sizeof(struct nxfft_plan_s), -1, "FFT Plan", KS_IKV(
        {"__free",                 ksf_wrap(T_free_, T_NAME ".__free(self)", "")},
        {"__new",                  ksf_wrap(T_new_, T_NAME ".__new(tp, obj, dtype=nx.float64)", "")},

    ));

}



