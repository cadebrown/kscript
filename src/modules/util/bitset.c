/* util/bitset.c - 'util.Bitset' type
 *
 * @author: Cade Brown <cade@kscript.org>
 */
#include <ks/impl.h>

#define T_NAME "util.Bitset"
#define TI_NAME "util.Bitset.__iter"


/* Internals */

/* Initialized to the value returned when a search for a bit fails */
static ks_cint my_gmp_bitnotfound = 0;


/* C-API */

ks_bitset ks_bitset_new(ks_type tp) {
    ks_bitset self = KSO_NEW(ks_bitset, tp);

    mpz_init(self->val);

    return self;
}

bool ks_bitset_add(ks_bitset self, ks_cint elem) {
    mpz_setbit(self->val, elem);
    return true;
}

bool ks_bitset_del(ks_bitset self, ks_cint elem) {
    mpz_clrbit(self->val, elem);
    return true;
}

bool ks_bitset_has(ks_bitset self, ks_cint elem) {
    return mpz_tstbit(self->val, elem);
}



/* Type Functions */

static KS_TFUNC(T, free) {
    ks_bitset self;
    KS_ARGS("self:*", &self, kst_bitset);

    mpz_clear(self->val);
    KSO_DEL(self);

    return KSO_NONE;
}


static KS_TFUNC(T, new) {
    ks_type tp;
    kso objs = KSO_NONE;
    KS_ARGS("tp:* ?objs", &tp, kst_type, &objs);

    ks_bitset self = ks_bitset_new(tp);
    if (!self) return NULL;

    if (objs == KSO_NONE) {

    } else if (kso_is_int(objs)) {
        ks_int vv = kso_int(objs);
        if (!vv) {
            KS_DECREF(self);
            return NULL;
        }
        mpz_set(self->val, vv->val);
    } else {
        /* add all elements */
        ks_cit it = ks_cit_make(objs);
        kso ob;
        while ((ob = ks_cit_next(&it)) != NULL) {
            ks_cint v;
            if (!kso_get_ci(ob, &v)) {
                it.exc = true;
            } else {
                if (!ks_bitset_add(self, v)) {
                    it.exc = true;
                }
            }
            KS_DECREF(ob);
        }

        ks_cit_done(&it);
        if (it.exc) {
            return NULL;
        }
    }

    return (kso)self;
}

static KS_TFUNC(T, str) {
    ks_bitset self;
    KS_ARGS("self:*", &self, kst_bitset);

    char* buf = ks_malloc(mpz_sizeinbase(self->val, 2));
    mpz_get_str(buf, 2, self->val);

    ks_str res = ks_fmt("%T(0b%s)", self, buf);
    ks_free(buf);

    return (kso)res;
}

static KS_TFUNC(T, bool) {
    ks_bitset self;
    KS_ARGS("self:*", &self, kst_bitset);

    return KSO_BOOL(mpz_cmp_si(self->val, 0) != 0);
}

static KS_TFUNC(T, int) {
    ks_bitset self;
    KS_ARGS("self:*", &self, kst_bitset);

    return (kso)ks_int_newz(self->val);
}

static KS_TFUNC(T, len) {
    ks_bitset self;
    KS_ARGS("self:*", &self, kst_bitset);

    return (kso)ks_int_new(mpz_popcount(self->val));
}

static KS_TFUNC(T, add) {
    ks_bitset self;
    int nargs;
    kso* args;
    KS_ARGS("self:* *args", &self, kst_bitset, &nargs, &args);

    int i;
    for (i = 0; i < nargs; ++i) {
        ks_cint elem;
        if (!kso_get_ci(args[i], &elem)) {
            return NULL;
        }
        if (!ks_bitset_add(self, elem)) {
            return NULL;
        }
    }

    return KSO_NONE;
}

static KS_TFUNC(T, rem) {
    ks_bitset self;
    int nargs;
    kso* args;
    KS_ARGS("self:* *args", &self, kst_bitset, &nargs, &args);

    int i;
    for (i = 0; i < nargs; ++i) {
        ks_cint elem;
        if (!kso_get_ci(args[i], &elem)) {
            return NULL;
        }
        if (!ks_bitset_del(self, elem)) {
            return NULL;
        }
    }

    return KSO_NONE;
}

/* Iterator Type */

static KS_TFUNC(TI, free) {
    ks_bitset_iter self;
    KS_ARGS("self:*", &self, kst_bitset_iter);

    KS_DECREF(self->of);

    KSO_DEL(self);

    return KSO_NONE;
}

static KS_TFUNC(TI, init) {
    ks_bitset_iter self;
    ks_bitset of;
    KS_ARGS("self:* of:*", &self, kst_bitset_iter, &of, kst_bitset);

    KS_INCREF(of);
    self->of = of;

    self->pos = 0;

    return KSO_NONE;
}

static KS_TFUNC(TI, next) {
    ks_bitset_iter self;
    KS_ARGS("self:*", &self, kst_bitset_iter);

    ks_cint np = mpz_scan1(self->of->val, self->pos);
    if (np == my_gmp_bitnotfound) {
        KS_OUTOFITER();
        return NULL;
    }

    self->pos = np + 1;

    return (kso)ks_int_new(np);
}




/* Export */

static struct ks_type_s tp;
ks_type kst_bitset = &tp;

static struct ks_type_s tpi;
ks_type kst_bitset_iter = &tpi;

void _ksi_bitset() {

    _ksinit(kst_bitset_iter, kst_object, TI_NAME, sizeof(struct ks_bitset_iter_s), -1, "", KS_IKV(
        {"__free",                 ksf_wrap(TI_free_, TI_NAME ".__free(self)", "")},
        {"__init",                 ksf_wrap(TI_init_, TI_NAME ".__init(self, of)", "")},
        {"__next",                 ksf_wrap(TI_next_, TI_NAME ".__next(self)", "")},

    ));

    _ksinit(kst_bitset, kst_object, T_NAME, sizeof(struct ks_bitset_s), -1, "Bitsets are efficient ways to store sets of small integers", KS_IKV(
        {"__free",                 ksf_wrap(T_free_, T_NAME ".__free(self)", "")},
        {"__new",                  ksf_wrap(T_new_, T_NAME ".__new(tp, objs=none)", "")},
        {"__repr",                 ksf_wrap(T_str_, T_NAME ".__repr(self)", "")},
        {"__str",                  ksf_wrap(T_str_, T_NAME ".__str(self)", "")},
        {"__bool",                 ksf_wrap(T_bool_, T_NAME ".__bool(self)", "")},
        {"__integral",             ksf_wrap(T_int_, T_NAME ".__integral(self)", "")},
        {"__len",                  ksf_wrap(T_len_, T_NAME ".__len(self)", "")},
        {"__iter",                 KS_NEWREF(kst_bitset_iter)},

        {"add",                    ksf_wrap(T_add_, T_NAME ".add(self, *args)", "Adds all arguments to the bitset")},
        {"rem",                    ksf_wrap(T_rem_, T_NAME ".rem(self, *args)", "Removes all arguments from the bitset")},
    //    {"pop",                    ksf_wrap(T_pop_, T_NAME ".pop(self)", "Pops off the front of the queue")},

    ));

    ks_int tmp = ks_int_new(0);
    my_gmp_bitnotfound = mpz_scan1(tmp->val, 0);
    KS_DECREF(tmp);


}
