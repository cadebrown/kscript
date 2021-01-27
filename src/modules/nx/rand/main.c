/* rand/main.c - 'nx.rand' module
 *
 * 
 * @author:    Cade Brown <cade@kscript.org>
 */
#include <ks/impl.h>
#include <ks/nx.h>
#include <ks/time.h>

#define M_NAME "nx.rand"

/* C-API */


/* Module Functions */

static KS_TFUNC(M, seed) {
    ks_cint seed;
    KS_ARGS("seed:*cint", &seed);

    nxrand_State_seed(nxrand_State_default, seed);

    return KSO_NONE;
}

static KS_TFUNC(M, randb) {
    ks_cint num = 1;
    KS_ARGS("?num:cint", &num);
    
    unsigned char* data = ks_malloc(num);
    nxrand_randb(nxrand_State_default, num, data);
    return (kso)ks_bytes_newn(num, data);
}

static KS_TFUNC(M, randf) {
    int nS;
    kso* S;
    KS_ARGS("*shape", &nS, &S);

    int rank;
    ks_size_t shape[NX_MAXRANK];
    if (!nx_getshapev(nS, S, &rank, shape)) {
        return NULL;
    }

    nx_array res = nx_array_newc(nxt_array, NULL, nxd_D, rank, shape, NULL);
    if (!nxrand_randf(nxrand_State_default, res->val)) {
        KS_DECREF(res);
        return NULL;
    }

    return (kso)res;
}

static KS_TFUNC(M, normal) {
    int nS;
    kso* S;
    KS_ARGS("*shape", &nS, &S);

    int rank;
    ks_size_t shape[NX_MAXRANK];
    if (!nx_getshapev(nS, S, &rank, shape)) {
        return NULL;
    }

    nx_array res = nx_array_newc(nxt_array, NULL, nxd_D, rank, shape, NULL);
    if (!nxrand_normal(nxrand_State_default, res->val)) {
        KS_DECREF(res);
        return NULL;
    }

    return (kso)res;
}



/* Export */

/* Default generator state */
nxrand_State nxrand_State_default = NULL;


ks_module _ksi_nxrand() {
    _ksi_nxrand_State();

#ifdef KS_HAVE_time
    nxrand_State_default = nxrand_State_new(time(NULL));
#else
    nxrand_State_default = nxrand_State_new(42);
#endif


    ks_module res = ks_module_new(M_NAME, KS_BIMOD_SRC, "Random module", KS_IKV(

        /* Types */

        {"State",                  (kso)nxrandt_State},

        /* Functions */

        {"seed",                   ksf_wrap(M_seed_, M_NAME ".seed(seed)", "Re-seed the default random number generator")},
        
        {"randb",                  ksf_wrap(M_randb_, M_NAME ".randb(num=1)", "Generate 'num' random bytes")},
        {"randf",                  ksf_wrap(M_randf_, M_NAME ".randf(*shape)", "Generate random floats with a given shape (default: scalar)")},
        {"normal",                 ksf_wrap(M_normal_, M_NAME ".normal(*shape)", "Generate random floats within the 'normal' distribution (mean=0, stddev=1)")},
    
    ));

    return res;
}
