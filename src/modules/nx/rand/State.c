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



/** Internal Generator Routines **/


/* Fill 'data' with new random words and reset 'pos' */
static void rs_fill(nxrand_State self) {
    self->pos = 0;
    ks_uint x = self->lw;
    int i;
    for (i = 0; i < NXRAND_N; ++i) {

        /* Generate */
        x ^= x << 13;
        x ^= x >> 7;
        x ^= x << 17;

        self->data[i] = x;
    }

    self->lw = x;
}

/* Seed a random state */
static void rs_seed(nxrand_State self, ks_uint seed) {

    /* Initialize the last word */
    self->lw = seed | 1ULL;

    /* Refill */
    rs_fill(self);

}

/* Generate 'nout' bytes and store in 'out' */
static void rs_bytes(nxrand_State self, int nout, unsigned char* out) {

    int i = 0;
    while (i < nout) {
        if (self->pos >= NXRAND_N) rs_fill(self);


#if 0
        /* Copy from entire integers (assumes no zero space) */

        /* Calculate the number of bytes we can copy directly over */
        int nb = (NXRAND_N - self->pos) * sizeof(*self->data);
        if (nout - i < nb) nb = nout - i;

        /* Just copy bytes */
        memcpy(&out[i], &self->data[self->pos], nb);

        /* Consume integers */
        self->pos += (nb + sizeof(*self->data) - 1) / sizeof(*self->data);
        i += nb;
#else
        /* Take each byte as the lowest byte of each word */

        /* Calculate the number of bytes we can copy directly over */
        int nb = NXRAND_N - self->pos;
        if (nout - i < nb) nb = nout - i;
        
        int j;
        for (j = 0; j < nb; ++j) {
            out[i + j] = self->data[self->pos + j];
        }

        i += nb;
        self->pos += nb;

#endif
    }
}


/* Return a random integer */
static ks_uint rs_randu(nxrand_State self) {
    if (self->pos >= NXRAND_N) rs_fill(self);

    return self->data[self->pos++];
}

/* Internal method to return a random float in [0, 1) */
static ks_cfloat rs_randf(nxrand_State self) {
    ks_uint x = rs_randu(self);
    return (ks_cfloat)(x & 0xFFFFFFFFULL) / 0xFFFFFFFFULL;
}


/** Internal Kernels **/


/* Each of these kernels (unless otherwise noted) takes a single input, which should
 *   be a floating-point type (it should give an error otherwise)
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
static int kern_randf(int N, nx_t* inp, int len, void* _data) {
    struct kern_data* data = _data;
    assert(N == 1);
    assert(inp[0].dtype == nxd_D);

    ks_uint pR = (ks_uint)inp[0].data;

    ks_cint i;
    for (i = 0; i < len; i++, pR += inp[0].strides[0]) {
        *(nx_D*)pR = rs_randf(data->s);
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
static int kern_normal(int N, nx_t* inp, int len, void* _data) {
    struct kern_data* data = _data;
    assert(N == 3);
    assert(inp[0].dtype == nxd_D && inp[1].dtype == nxd_D && inp[2].dtype == nxd_D);

    ks_uint pR = (ks_uint)inp[0].data, pu = (ks_uint)inp[1].data, po = (ks_uint)inp[2].data;

    /* We use the Box-Muller transform to generate pairs of numbers
     */
    ks_cfloat z0, z1;

    ks_cint i;
    for (i = 0; i < len; i++, pR += inp[0].strides[0], pu += inp[1].strides[0], po += inp[2].strides[0]) {
        /* Load the u/o values */
        ks_cfloat uv = *(nx_D*)pu, ov = *(nx_D*)po;

        if (i % 2 == 0) {
            /* Generate 2 new numbers */
            ks_cfloat t0 = rs_randf(data->s), t1 = rs_randf(data->s);

            /* Apply transform */
            ks_cfloat theta = 2 * KSM_PI * t1;
            ks_cfloat r = sqrt(-2 * log(t0));

            double st, ct;
            my_sincos(r, &st, &ct);

            z0 = r * ct;
            z1 = r * st;

            *(nx_D*)pR = z0 * ov + uv;
        } else {
            /* Just use the other one that was already generated */
            *(nx_D*)pR = z1 * ov + uv;
        }
    }

    return 0;
}


/* C-API */

nxrand_State nxrand_State_new(ks_uint seed) {
    nxrand_State self = KSO_NEW(nxrand_State, nxrandt_State);

    rs_seed(self, seed);

    return self;
}

void nxrand_State_seed(nxrand_State self, ks_uint seed) {
    rs_seed(self, seed);
}


bool nxrand_randb(nxrand_State self, int nout, unsigned char* out) {
    rs_bytes(self, nout, out);
    return true;
}

/** Random Number Generation **/

bool nxrand_randf(nxrand_State self, nx_t R) {
    struct kern_data data;
    data.s = self;
    return !nx_apply_elem(kern_randf, 1, (nx_t[]){ R }, (void*)&data);
}

bool nxrand_normal(nxrand_State self, nx_t R, nx_t u, nx_t o) {
    struct kern_data data;
    data.s = self;
    return !nx_apply_elem(kern_normal, 3, (nx_t[]){ R, u, o }, (void*)&data);
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

    return (kso)ks_float_new(rs_randf(self));
}
static KS_TFUNC(T, seed) {
    nxrand_State self;
    ks_cint seed;
    KS_ARGS("self:* ?seed:cint", &self, nxrandt_State, &seed);
    
    nxrand_State_seed(self, seed);

    return KSO_NONE;
}

static KS_TFUNC(T, randb) {
    nxrand_State self;
    ks_cint num = 1;
    KS_ARGS("self:* ?num:cint", &self, nxrandt_State, &num);
    
    unsigned char* data = ks_malloc(num);
    nxrand_randb(self, num, data);
    return (kso)ks_bytes_newn(num, data);
}

static KS_TFUNC(T, randf) {
    nxrand_State self;
    kso shape = KSO_NONE;
    KS_ARGS("self:* ?shape", &self, nxrandt_State, &shape);
    
    nx_t ns = nx_getshape(shape);
    if (ns.rank < 0) return NULL;

    nx_array res = nx_array_newc(nxt_array, NULL, nxd_D, ns.rank, ns.shape, NULL);
    if (!nxrand_randf(self, res->val)) {
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

    nx_t uar, oar;
    kso uref, oref;
    if (!nx_get(u, nxd_D, &uar, &uref)) {
        return NULL;
    }
    if (!nx_get(o, nxd_D, &oar, &oref)) {
        KS_NDECREF(uref);
        return NULL;
    }
    nx_t ns = nx_getshape(shape);
    if (ns.rank < 0) return NULL;

    nx_array res = nx_array_newc(nxt_array, NULL, nxd_D, ns.rank, ns.shape, NULL);
    if (!nxrand_normal(self, res->val, uar, oar)) {
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

        {"seed",                   ksf_wrap(T_seed_, T_NAME ".seed(self, seed)", "Re-seed the random state")},
        
        {"randb",                  ksf_wrap(T_randb_, T_NAME ".randb(self, num=1)", "Generate 'num' random bytes")},
        {"randf",                  ksf_wrap(T_randf_, T_NAME ".randf(self, shape=none)", "Generate random floats")},
        {"normal",                 ksf_wrap(T_normal_, T_NAME ".normal(self, u=0.0, o=1.0, shape=none)", "Generate random floats in the normal distribution")},

    ));
}
