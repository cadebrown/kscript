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


nxfft_plan nxfft_make(nx_dtype dtype, int rank, ks_size_t* dims, bool is_inv) {
    if (dtype->kind != NX_DTYPE_COMPLEX) {
        KS_THROW(kst_TypeError, "FFT only defined for complex types");
        return NULL;
    }

    //return make_1D_DENSE(dtype, dims[0], is_inv);
    //return make_1D_BFLY(dtype, dims[0], is_inv);

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



