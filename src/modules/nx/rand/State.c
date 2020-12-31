/* State.c - implementation of the 'nx.rand.State' type
 * 
 *
 * SEE: https://www.mcs.anl.gov/~kazutomo/hugepage-old/twister.c
 * 
 * @author:    Cade Brown <cade@kscript.org>
 */
#include <ks/impl.h>
#include <ks/nx.h>

#define T_NAME "nx.rand.State"


/* Internals */


/* Mask all but highest bit of 'u' */
#define hi_bit(u)       ((u) & 0x80000000U)
/* Mask all but lowest bit of 'u' */
#define lo_bit(u)       ((u) & 0x00000001U)
#define lo_bits(u)      ((u) & 0x7FFFFFFFU)
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

/* Reload the internal buffer of random bits (self->state) */
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

/* Generate random bytes */
static void mt_randb(nxrand_State self, int nout, unsigned char* out) {
    int i = 0;
    while (i < nout) {
        if (self-> pos >= NXRAND_MT_N) mt_reload(self);

        /* Maximum number of state numbers to use */
        int mx = NXRAND_MT_N - self->pos;
        if (nout - i < mx) mx = nout - i;

        /* TODO: perhaps use 4 bytes per state variable */
        int j;
        for (j = 0; j < mx; ++j) {
            out[i + j] = self->state[self->pos + j];
        }

        self->pos += mx;
        i += mx;
    }
}

static ks_uint32_t mt_randi(nxrand_State self) {

    if (self->pos >= NXRAND_MT_N) mt_reload(self);

    ks_uint32_t y = self->state[self->pos++];
    y ^= (y >> 11);
    y ^= (y <<  7) & 0x9D2C5680U;
    y ^= (y << 15) & 0xEFC60000U;

    return (y ^ (y >> 18));
}


/* C-API */

nxrand_State nxrand_State_new(ks_uint seed) {
    nxrand_State self = KSO_NEW(nxrand_State, nxrandt_State);

    mt_seed(self, seed);

    return self;
}

bool nxrand_get_a(nxrand_State self, nxar_t A) {

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

/* Export */

static struct ks_type_s tp;
ks_type nxrandt_State = &tp;

void _ksi_nxrand_State() {
    
    _ksinit(nxrandt_State, kst_object, T_NAME, sizeof(struct nxrand_State_s), -1, "Random number state", KS_IKV(
        {"__new",                  ksf_wrap(T_new_, T_NAME ".__new(tp, seed=none)", "")},

        {"__next",                 ksf_wrap(T_next_, T_NAME ".__next(self)", "")},

        {"randb",                  ksf_wrap(T_randb_, T_NAME ".randb(self, num=1)", "Generate 'num' random bytes")},

    ));
}
