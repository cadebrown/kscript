/* State.c - implementation of the 'nx.rand.State' type
 * 
 *
 * SEE: https://www.mcs.anl.gov/~kazutomo/hugepage-old/twister.c
 * 
 * @author:    Cade Brown <cade@kscript.org>
 */
#include <ks/impl.h>
#include <ks/nx.h>
#include <ks/m.h>

#define T_NAME "nx.rand.State"

/* Internals */


/** Math Utilities **/


/* Compute sin and cosine at once */
static void my_sincos(double x, double* sinx, double* cosx) {
#ifdef KS_HAVE_sincos
    sincos(x, sinx, cosx);
#else
    *sinx = sin(x);
    *cosx = cos(x);
#endif
}


/** Mersenne Twister Implementation **/

/* Mask of the highest bit of 'u' */
#define hi_bit(u)       ((u) & 0x80000000U)
/* Mask of the lowest bit of 'u' */
#define lo_bit(u)       ((u) & 0x00000001U)

/* Mask of all but the highest bit of 'u' */
#define lo_bits(u)      ((u) & 0x7FFFFFFFU)
/* Take the highest bit of 'u' and the lowest bits of 'v' */
#define mix_bits(u, v)  (hi_bit(u) | lo_bits(v))

/* Re-seeds a state with a given seed */
static void mt_seed(nxrand_State self, ks_uint seed) {
    ks_uint x = (seed | 1ULL) & 0xFFFFFFFFULL;

    int i;
    for (i = 0; i < NXRAND_MT_N; ++i) {
        self->state[i] = x & 0xFFFFFFFFULL;
        x *= 69069ULL;
    }
}

/* Reload the internal buffer of random bits (refills 'self->state' with 'NXRAND_MT_N' random numbers) */
static void mt_reload(nxrand_State self) {
    ks_uint32_t *p0 = self->state, *p2 = self->state + 2, *pM = self->state + NXRAND_MT_M;
    ks_uint32_t s0, s1;
    int i;

    self->pos = 0;

    for (s0 = self->state[0], s1 = self->state[1], i = NXRAND_MT_N - NXRAND_MT_M + 1; --i; s0 = s1, s1 = *p2++) {
        *p0++ = *pM++ ^ (mix_bits(s0, s1) >> 1) ^ (lo_bit(s1) ? NXRAND_MT_K : 0);
    }

    for (pM = self->state, i = NXRAND_MT_M; --i; s0 = s1, s1 = *p2++) {
        *p0++ = *pM++ ^ (mix_bits(s0, s1) >> 1) ^ (lo_bit(s1) ? NXRAND_MT_K : 0);
    }

    s1 = self->state[1];
    *p0 = *pM ^ (mix_bits(s0, s1) >> 1) ^ (lo_bit(s1) ? NXRAND_MT_K : 0);
}

/* Internal method to generate a number of random bytes */
static void mt_randb(nxrand_State self, int nout, unsigned char* out) {
    int i = 0;
    while (i < nout) {
        /* If we are at the end of the generated numbers, reload the internal state */
        if (self->pos >= NXRAND_MT_N) mt_reload(self);

        /* Maximum number of words to use */
        int mxw = NXRAND_MT_N - self->pos;
        /* Make sure we don't generate more bytes than we asked for */
        if (nout - i < mxw) mxw = nout - i;

        int j;
        for (j = 0; j < mxw; ++j) {
            /* Calculate word from mersenne algorithm */
            ks_uint32_t y = self->state[self->pos++];
            y ^= (y >> 11);
            y ^= (y <<  7) & 0x9D2C5680U;
            y ^= (y << 15) & 0xEFC60000U;
            y ^= y >> 18;

            /* Now, set a single byte from a single word 
             * TODO: we could skip 4 at a time
             */
            out[i + j] = y & 0xFF;
        }

        i += mxw;
    }
}

/* Internal method to return a random 32 bit integer */
static ks_uint32_t mt_randi(nxrand_State self) {
    if (self->pos >= NXRAND_MT_N) mt_reload(self);

    ks_uint32_t y = self->state[self->pos++];
    y ^= (y >> 11);
    y ^= (y <<  7) & 0x9D2C5680U;
    y ^= (y << 15) & 0xEFC60000U;
    y ^= y >> 18;

    return y;
}

/* Internal method to return a random float in [0, 1) */
static ks_cfloat mt_randf(nxrand_State self) {
    ks_uint x = mt_randi(self);
    return (ks_cfloat)(x & 0xFFFFFFFFULL) / 0xFFFFFFFFULL;
}


/** Internal Kernels **/


/* Each of these kernels (unless otherwise noted) takes a single input, which should
 *   be a floating-point type (it should give an error otherwise)
 *
 * 
 */


/* Data passed to each kernel (for state, and parameters) */
struct kern_data {

    /* Random state */
    nxrand_State s;

    /* Parameters (specific to each method) */
    ks_cfloat p0, p1, p2, p3;

};

/* Compute uniform floats in [0, 1) */
static int kern_randf(int N, nxar_t* inp, int len, void* _data) {
    struct kern_data* data = _data;
    assert(N == 1);

    ks_uint pR = (ks_uint)inp[0].data;

    ks_cint i;
    for (i = 0; i < len; i++, pR += inp[0].strides[0]) {
        *(double*)pR = mt_randf(data->s);
    }

    return 0;
}

/* Compute normal floats (i.e. Guassian/Bell Curve distribution)
 *
 * Inputs:
 *   A: This is the array being filled
 *   u: The mean (normally a single value broadcasted, but can be an array)
 *   o: The stddev (normally a single value, but can be an array)
 * 
 */
static int kern_normal(int N, nxar_t* inp, int len, void* _data) {
    struct kern_data* data = _data;
    assert(N == 3);

    ks_uint pR = (ks_uint)inp[0].data, pu = (ks_uint)inp[1].data, po = (ks_uint)inp[2].data;

    /* We use the Box-Muller transform to generate pairs of numbers
     */
    ks_cfloat z0, z1;

    ks_cint i;
    for (i = 0; i < len; i++, pR += inp[0].strides[0], pu += inp[1].strides[0], po += inp[2].strides[0]) {
        /* Load the u/o values */
        ks_cfloat uv = *(double*)pu, ov = *(double*)po;

        if (i % 2 == 0) {
            /* Generate 2 new numbers */
            ks_cfloat t0 = mt_randf(data->s), t1 = mt_randf(data->s);

            /* Apply transform */
            ks_cfloat theta = 2 * KSM_PI * t1;
            ks_cfloat r = sqrt(-2 * log(t0));

            double st, ct;
            my_sincos(r, &st, &ct);

            z0 = r * ct;
            z1 = r * st;

            *(double*)pR = z0 * ov + uv;
        } else {
            /* Just use the other one that was already generated */
            *(double*)pR = z1 * ov + uv;
        }
    }

    return 0;
}


/* C-API */

nxrand_State nxrand_State_new(ks_uint seed) {
    nxrand_State self = KSO_NEW(nxrand_State, nxrandt_State);

    mt_seed(self, seed);

    return self;
}

bool nxrand_get_b(nxrand_State self, int nout, unsigned char* out) {
    mt_randb(self, nout, out);
    return true;
}

bool nxrand_get_i(nxrand_State self, int nout, ks_uint* out) {
    return nxrand_get_b(self, nout * sizeof(*out), (unsigned char*)out);
}

bool nxrand_get_f(nxrand_State self, int nout, ks_cfloat* out) {
    int i;
    for (i = 0; i < nout; ++i) {
        ks_uint x = mt_randi(self);
        out[i] = (ks_cfloat)(x & 0xFFFFFFFF) / 0xFFFFFFFF;
    }

    return true;
}


/** Random Number Generation **/

bool nxrand_randf(nxrand_State self, nxar_t R) {
    struct kern_data data;
    data.s = self;
    return !nx_apply_elem(kern_randf, 1, (nxar_t[]){ R }, (void*)&data);
}

bool nxrand_normal(nxrand_State self, nxar_t R, nxar_t u, nxar_t o) {
    struct kern_data data;
    data.s = self;
    return !nx_apply_elem(kern_normal, 3, (nxar_t[]){ R, u, o }, (void*)&data);
}



/* Type Functions */

static KS_TFUNC(T, new) {
    ks_type tp;
    kso seed = KSO_NONE;
    KS_ARGS("tp:* ?seed", &tp, kst_type, &seed);

    ks_uint su = 0;
    if (seed != KSO_NONE) {
        if (!kso_hash(seed, &su)) return NULL;
    }

    return (kso)nxrand_State_new(su);
}

static KS_TFUNC(T, next) {
    nxrand_State self;
    KS_ARGS("self:*", &self, nxrandt_State);

    ks_cfloat x;
    if (!nxrand_get_f(self, 1, &x)) return NULL;

    return (kso)ks_float_new(x);
}

static KS_TFUNC(T, randb) {
    nxrand_State self;
    ks_cint num = 1;
    KS_ARGS("self:* ?num:cint", &self, nxrandt_State, &num);
    
    unsigned char* data = ks_malloc(num);
    mt_randb(self, num, data);
    return (kso)ks_bytes_newn(num, data);
}

static KS_TFUNC(T, randf) {
    nxrand_State self;
    kso shape = KSO_NONE;
    KS_ARGS("self:* ?shape", &self, nxrandt_State, &shape);
    
    int ns;
    ks_size_t* s = nx_getsize(shape, &ns);
    if (!s) return NULL;

    nx_array res = nx_array_newc(nxt_array, nxd_double, ns, s, NULL, NULL);
    ks_free(s);

    if (!nxrand_randf(self, res->ar)) {
        KS_DECREF(res);
        return NULL;
    }

    return (kso)res;
}

static KS_TFUNC(T, normal) {
    nxrand_State self;
    kso u, o;
    kso shape = KSO_NONE;
    KS_ARGS("self:* u o ?shape", &self, nxrandt_State, &u, &o, &shape);

    nxar_t uar, oar;
    kso uref, oref;
    if (!nxar_get(u, nxd_double, &uar, &uref)) {
        return NULL;
    }
    if (!nxar_get(o, nxd_double, &oar, &oref)) {
        KS_NDECREF(uref);
        return NULL;
    }
    int ns;
    ks_size_t* s = nx_getsize(shape, &ns);
    if (!s) return NULL;

    nx_array res = nx_array_newc(nxt_array, nxd_double, ns, s, NULL, NULL);
    ks_free(s);
    if (!nxrand_normal(self, res->ar, uar, oar)) {
        KS_NDECREF(uref);
        KS_NDECREF(oref);
        KS_DECREF(res);
        return NULL;
    }
    KS_NDECREF(uref);
    KS_NDECREF(oref);
    return (kso)res;
}


/* Export */

static struct ks_type_s tp;
ks_type nxrandt_State = &tp;

void _ksi_nxrand_State() {
    
    _ksinit(nxrandt_State, kst_object, T_NAME, sizeof(struct nxrand_State_s), -1, "Random number state", KS_IKV(
        {"__new",                  ksf_wrap(T_new_, T_NAME ".__new(tp, seed=none)", "")},

        {"__next",                 ksf_wrap(T_next_, T_NAME ".__next(self)", "")},

        {"randb",                  ksf_wrap(T_randb_, T_NAME ".randb(self, num=1)", "Generate 'num' random bytes")},
        {"randf",                  ksf_wrap(T_randf_, T_NAME ".randf(self, shape=none)", "Generate random floats")},
        {"normal",                 ksf_wrap(T_normal_, T_NAME ".normal(self, u=0.0, o=1.0, shape=none)", "Generate random floats in the normal distribution")},

    ));
}
