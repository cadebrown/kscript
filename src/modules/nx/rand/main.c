/* rand/main.c - 'nx.rand' module
 *
 * 
 * @author:    Cade Brown <cade@kscript.org>
 */
#include <ks/impl.h>
#include <ks/nx.h>

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
    kso shape = KSO_NONE;
    KS_ARGS("?shape", &shape);

    nx_t ns = nx_getshape(shape);
    if (ns.rank < 0) return NULL;

    nx_array res = nx_array_newc(nxt_array, NULL, nxd_D, ns.rank, ns.shape, NULL);

    if (!nxrand_randf(nxrand_State_default, res->val)) {
        KS_DECREF(res);
        return NULL;
    }

    return (kso)res;
}

static KS_TFUNC(M, normal) {
    kso u, o;
    kso shape = KSO_NONE;
    KS_ARGS("u o ?shape", &u, &o, &shape);

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
    if (!nxrand_normal(nxrand_State_default, res->val, uar, oar)) {
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
        {"randf",                  ksf_wrap(M_randf_, M_NAME ".randf(shape=none)", "Generate random floats with a given shape (default: scalar)")},
        {"normal",                 ksf_wrap(M_normal_, M_NAME ".normal(u=0.0, o=1.0, shape=none)", "Generate random floats in the normal distribution with a given shape (default: scalar)")},


    
    ));

    return res;
}
