/* types/set.c - 'set' type
 *
 * @author: Cade Brown <cade@kscript.org>
 */
#include <ks/impl.h>

#define T_NAME "set"
#define TI_NAME T_NAME ".__iter"

/* Internals */

/* Special bucket values signalling that it is vacant, or deleted */
#define B_EMPTY          (-1)
#define B_DELETED        (-2)


/* Maximum load factor that is tolerable. When this is exceeded, it is resized and rehashed */
#define S_LOAD_MAX       (0.6)

/* New/target load factor for rehashing */
#define S_LOAD_NEW       (0.3)

/* Maximum proportion of holes in the entries array. Once the ratio exceeds this, holes are filled */
#define S_HOLES_MAX      (0.5)


/* Template to conditionally execute different code based on size 
 * 
 * Since hash tables use different sized indices for different number of entries, this macro allows
 *   clean code that doesn't manually select the buckets array. You can use '__buckets' which will be set
 *   to the relevant one for a given set and length of entries
 * 
 * Note that this should be the length of the entries array (->len_ents), not the number of entries, which may be different
 *   
 * 
 */
#define S_T_SIZE(_self, _len_ents, ...) do { \
    /****/ if (_len_ents < KS_SINT8_MAX) {                 \
        ks_sint8_t* __buckets = self->buckets_s8;          \
        { __VA_ARGS__; };                                  \
    } else if (_len_ents < KS_SINT16_MAX) {                \
        ks_sint16_t* __buckets = self->buckets_s16;        \
        { __VA_ARGS__; };                                  \
    } else if (_len_ents < KS_SINT32_MAX) {                \
        ks_sint32_t* __buckets = self->buckets_s32;        \
        { __VA_ARGS__; };                                  \
    } else {                                               \
        ks_sint64_t* __buckets = self->buckets_s64;        \
        { __VA_ARGS__; };                                  \
    }                                                      \
} while (0)


/* Calculate real load factor */
static double s_load(ks_set self) {
    return self->len_buckets > 0 ? (double)self->len_ents / self->len_buckets : 0.0;
}


/* Search through the hash table for a given hash and key 
 *
 * Sets 'rb' to the bucket at which the key was located, OR the first bucket that was empty, OR
 *   -1 if the hash table was full
 * Sets 're' to the index in the arrays of entries, or -1 if it was not found
 * 
 * Returns true if the operation completed, false if an error was thrown
 */
static bool s_search(ks_set self, kso key, ks_hash_t hash, ks_ssize_t* rb, ks_ssize_t* re) {
    if (self->len_buckets <= 0) {
        /* no elements, but avoid modulo by 0 */
        *rb = *re = -1;
        return true;
    }

    /* Calculate bucket index based on hash */
    ks_size_t bi = hash % self->len_buckets;

    /* First bucket index, and number of tries thus far */
    ks_size_t bi0 = bi, tries = 0;

    do {
        /* Element index (>=0 means valid, < 0 means special case) */
        ks_ssize_t ei;
        S_T_SIZE(self, self->len_ents,
            ei = __buckets[bi];
        );


        /****/ if (ei == B_EMPTY) {
            /* Hit an empty bucket, so the key was not present. However, it can now be inserted here, so signal that */
            *rb = bi;
            *re = -1;
            return true;
        } else if (ei == B_DELETED) {
            /* Skip deleted entries, so that we don't break existing keys */
        } else if (self->ents[ei].hash == hash) {
            /* Hashes match exactly, now determine whether the objects are eqaully (identically, or via 'A == B') */
            bool is_eq = self->ents[ei].key == key;

            if (!is_eq) {
                /* Determine via comparison */
                if (!kso_eq(self->ents[ei].key, key, &is_eq)) return false;
            }

            if (is_eq) {
                /* Successfully found it */
                *rb = bi;
                *re = ei;
                return true;
            }
        }

        /* Probe for the next bucket in the hash table
         *
         * For now, we use linear probing
         */
        bi = (bi + 1) % self->len_buckets;

    } while (bi != bi0);

    /* Not found, and no room to insert, so signal that */
    *rb = *re = -1;
    return -1;
}

/* Fill holes in the entries array of 'self'
 *
 * Returns true if the operation completed, false if an error was thrown
 */
static bool s_fill_holes(ks_set self) {

    if (self->len_ents <= self->len_real || self->len_ents <= 0) return true;

    ks_ssize_t* em = ks_zmalloc(self->len_ents, sizeof(*em));
    if (!em) {
        //KS_THROW_OUTOFMEM();
        return false;
    }

    /* Calculate index translation map for the entries */
    ks_size_t i, j = 0;

    for (i = 0; i < self->len_ents; ++i) {
        em[i] = j;
        if (self->ents[i].key != NULL) j++;
    }


    /* Replace current indices with new indices */
    for (i = 0; i < self->len_ents; ++i) self->ents[em[i]] = self->ents[i];
    S_T_SIZE(self, self->len_ents,
        for (i = 0; i < self->len_buckets; ++i) if (__buckets[i] >= 0)
            __buckets[i] = em[__buckets[i]];
    );

    ks_free(em);
    return true;
}

/* Resize and rehash the hash table to hold at least 'new_len_buckets'
 *
 * Returns true if the operation completed, false if an error was thrown
 */
static bool s_resize(ks_set self, ks_size_t new_len_buckets) {
    new_len_buckets = ks_nextprime(new_len_buckets);
    if (self->len_buckets >= new_len_buckets) return true;

    ks_size_t i;

    /* Calculate required size of buckets array */
    ks_size_t new_bucket_sz = 0;
    S_T_SIZE(self, self->len_ents, 
        new_bucket_sz = sizeof(*__buckets) * new_len_buckets;
    );

    /* Reallocate buckets array if we need to */
    if (new_bucket_sz > self->_max_len_buckets_b) {
        self->_max_len_buckets_b = ks_nextsize(self->_max_len_buckets_b, new_bucket_sz);

        self->buckets_s8 = ks_realloc(self->buckets_s8, self->_max_len_buckets_b);
        //if (!new_buckets) {
            /* No memory */
        //}
    }

    /* Update and clear all buckets */
    self->len_buckets = new_len_buckets;
    S_T_SIZE(self, self->len_ents, 
        for (i = 0; i < self->len_buckets; ++i) __buckets[i] = B_EMPTY;
    );

    /* Now, rehash the buckets */
    ks_size_t ct = 0;
    while (self->len_ents > 0 && self->ents[self->len_ents - 1].key == NULL) {
        self->len_ents--;
        self->len_real--;
    }
    for (i = 0; i < self->len_ents; ++i) {
        struct ks_set_ent* ent = &self->ents[i];
        if (ent->key) {
            ks_ssize_t rb, re;
            if (!s_search(self, ent->key, ent->hash, &rb, &re)) return false;
            assert(re < 0 && rb >= 0);

            S_T_SIZE(self, self->len_ents, 
                __buckets[rb] = i;
            );

            ct++;
        }
    }
    
    assert(self->len_real == ct);
    return self->len_ents * S_HOLES_MAX >= self->len_real ? s_fill_holes(self) : true;
}


/* C-API */

ks_set ks_set_new(ks_cint len, kso* elems) {
    ks_set self = KSO_NEW(ks_set, kst_set);

    self->len_ents = self->len_real = self->_max_len_ents = 0;
    self->ents = NULL;

    self->len_buckets = self->_max_len_buckets_b = 0;
    self->buckets_s8 = NULL;

    ks_cint i;
    for (i = 0; i < len; ++i) {
        if (!ks_set_add(self, elems[i])) {
            KS_DECREF(self);
            return NULL;
        }
    }

    return self;
}

void ks_set_clear(ks_set self) {
    int i;
    for (i = 0; i < self->len_ents; ++i) {
        if (self->ents[i].key != NULL) {
            KS_DECREF(self->ents[i].key);
        }
    }
    S_T_SIZE(self, self->len_ents, 
        for (i = 0; i < self->len_buckets; ++i) __buckets[i] = B_EMPTY;
    );

    self->len_buckets = 0;
    self->len_real = self->len_ents = 0;
}

bool ks_set_add(ks_set self, kso key) {
    ks_hash_t hash;
    if (!kso_hash(key, &hash)) return false;
    return ks_set_add_h(self, key, hash);
}

bool ks_set_add_h(ks_set self, kso key, ks_hash_t hash) {
    ks_ssize_t rb, re;

    /* Resize if needed */
    if (s_load(self) > S_LOAD_MAX || self->len_buckets == 0)
        if (!s_resize(self, (ks_size_t)(self->len_ents / S_LOAD_NEW)))
            return false;

    if (!s_search(self, key, hash, &rb, &re)) return false;

    if (re < 0) {
        /* Not found, so add to the setionary*/
        
        re = self->len_ents++;
        if (self->len_ents > self->_max_len_ents) {
            self->_max_len_ents = ks_nextsize(self->_max_len_ents, self->len_ents);
            self->ents = ks_zrealloc(self->ents, sizeof(*self->ents), self->_max_len_ents);
        }

        /* Add to entries */
        self->len_real++;
        
        KS_INCREF(key);
        
        self->ents[re].hash = hash;
        self->ents[re].key = key;

        /* Add buckets if needed */
        if (rb < 0 || (self->len_ents == KS_SINT8_MAX || self->len_ents == KS_SINT16_MAX || self->len_ents == KS_SINT32_MAX)) {
            /* No bucket found, we need to resize/fill holes */
            if (!s_resize(self, (ks_size_t)(self->len_ents / S_LOAD_NEW)))
                return false;

            /* Now, refind it */
            ks_ssize_t new_re;
            if (!s_search(self, key, hash, &rb, &new_re)) return false;

            assert(rb >= 0 && re == new_re);
        }

        /* Set the bucket to point to it */
        S_T_SIZE(self, self->len_ents, 
            __buckets[rb] = re;
        );

        return true;
    } else {
        /* Found, so don't update */
        return true;
    }
}

bool ks_set_has(ks_set self, kso key, bool* exists) {
    ks_hash_t hash;
    if (!kso_hash(key, &hash)) return false;
    return ks_set_has_h(self, key, hash, exists);
}

bool ks_set_has_h(ks_set self, kso key, ks_hash_t hash, bool* exists) {
    ks_ssize_t rb, re;
    if (!s_search(self, key, hash, &rb, &re)) return false;

    *exists = re >= 0;
    return true;
}

bool ks_set_del(ks_set self, kso key, bool* existed) {
    ks_hash_t hash;
    if (!kso_hash(key, &hash)) return false;
    return ks_set_del_h(self, key, hash, existed);
}

bool ks_set_del_h(ks_set self, kso key, ks_hash_t hash, bool* existed) {
    ks_ssize_t rb, re;
    if (!s_search(self, key, hash, &rb, &re)) return false;

    *existed = re >= 0;

    if (*existed) {
        assert (rb >= 0);
        S_T_SIZE(self, self->len_ents,
            __buckets[rb] = B_DELETED;
        );
    }

    return true;
}

bool ks_set_addall(ks_set self, kso objs) {
    ks_cit it = ks_cit_make(objs);
    kso ob;
    while ((ob = ks_cit_next(&it)) != NULL) {
        if (!ks_set_add(self, ob)) it.exc = true;
        KS_DECREF(ob);
    }
    ks_cit_done(&it);
    return !it.exc;
}
bool ks_set_addn(ks_set self, ks_cint len, kso* objs) {
    ks_cint i;
    for (i = 0; i < len; ++i) {
        if (!ks_set_add(self, objs[i])) return false;
    }
    return true;
}


ks_list ks_set_calc_ents(ks_set self) {
    ks_list res = ks_list_new(0, NULL);

    ks_ssize_t i;
    for (i = 0; i < self->len_ents; ++i) {
        struct ks_set_ent* ent = &self->ents[i];
        if (ent->key) {
            ks_int h = ks_int_newu(ent->hash);;
            ks_tuple e = ks_tuple_new(2, (kso[]){
                ent->key,
                (kso)h
            });
            KS_DECREF(h);
            ks_list_push(res, (kso)e);
            KS_DECREF(e);
        } else {
            ks_list_push(res, KSO_NONE);
        }
    }

    return res;
}

ks_list ks_set_calc_buckets(ks_set self) {
    ks_list res = ks_list_new(0, NULL);

    ks_ssize_t i;
    for (i = 0; i < self->len_buckets; ++i) {
        ks_ssize_t b;
        S_T_SIZE(self, self->len_ents,
            b = __buckets[i];
        );

        ks_int v = ks_int_new(b);
        ks_list_push(res, (kso)v);
        KS_DECREF(v);
    }

    return res;
}

/* Type Functions */

static KS_TFUNC(T, free) {
    ks_set self;
    KS_ARGS("self:*", &self, kst_set);

    ks_cint i;
    for (i = 0; i < self->len_ents; ++i) {
        struct ks_set_ent* ent = &self->ents[i];
        if (ent->key) {
            KS_DECREF(ent->key);
        }
    }

    ks_free(self->ents);
    ks_free(self->buckets_s8);

    KSO_DEL(self);

    return KSO_NONE;
}

static KS_TFUNC(T, new) {
    ks_type tp;
    int nargs;
    kso* args;
    KS_ARGS("tp:* *args", &tp, kst_type, &nargs, &args);

    ks_set self = KSO_NEW(ks_set, tp);

    self->len_buckets = self->len_ents = self->len_real = 0;
    self->_max_len_buckets_b = self->_max_len_ents = 0;

    self->ents = NULL;
    self->buckets_s8 = NULL;

    return (kso)self;
}

static KS_TFUNC(T, init) {
    ks_set self;
    kso objs = KSO_NONE;
    KS_ARGS("self:* ?objs", &self, kst_set, &objs);

    ks_set_clear(self);

    if (objs != KSO_NONE) {
        if (!ks_set_addall(self, objs)) {
            return NULL;
        }
    }

    return KSO_NONE;
}

static KS_TFUNC(T, len) {
    ks_set self;
    KS_ARGS("self:*", &self, kst_set);

    return (kso)ks_int_newu(self->len_real);
}


/** Iterator **/

static KS_TFUNC(TI, free) {
    ks_set_iter self;
    KS_ARGS("self:*", &self, kst_set_iter);

    KS_DECREF(self->of);
    KSO_DEL(self);

    return KSO_NONE;
}

static KS_TFUNC(TI, new) {
    ks_type tp;
    ks_set of;
    KS_ARGS("tp:* of:*", &tp, kst_type, &of, kst_set);

    ks_set_iter self = KSO_NEW(ks_set_iter, tp);

    KS_INCREF(of);
    self->of = of;

    self->pos = 0;

    return (kso)self;
}

/* Export */

static struct ks_type_s tp;
ks_type kst_set = &tp;

static struct ks_type_s tp_iter;
ks_type kst_set_iter = &tp_iter;

void _ksi_set() {


    _ksinit(kst_set_iter, kst_object, TI_NAME, sizeof(struct ks_set_s), -1, "", KS_IKV(
        {"__free",               ksf_wrap(TI_free_, TI_NAME ".__free(self)", "")},
        {"__new",                ksf_wrap(TI_new_, TI_NAME ".__new(tp, of)", "")},
    ));


    _ksinit(kst_set, kst_object, T_NAME, sizeof(struct ks_set_s), -1, "A set of (unique) objects, which can be modified, ordered by first insertion order, resetting with deletion\n\n    Internally, it is a hash-set, which means only one object that hashes a certain way and compares equal with other keys may be contained. Therefore, you cannot store things like 'true' and '1' in the same hashset -- they will become the same item", KS_IKV(
        {"__free",                 ksf_wrap(T_free_, T_NAME ".__free(self)", "")},
        {"__new",                  ksf_wrap(T_new_, T_NAME ".__new(tp, *args)", "")},
        {"__init",                 ksf_wrap(T_init_, T_NAME ".__init(self, objs=none)", "")},

        {"__len",                  ksf_wrap(T_len_, T_NAME ".__len(self)", "")},
        {"__iter",                 KS_NEWREF(kst_set_iter)},
    ));

    kst_set->i__hash = NULL;
}
