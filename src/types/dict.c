/* types/dict.c - 'dict' type
 *
 * @author: Cade Brown <cade@kscript.org>
 */
#include <ks/impl.h>

#define T_NAME "dict"
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
 *   to the relevant one for a given dictionary and length of entries
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
static double s_load(ks_dict self) {
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
static bool s_search(ks_dict self, kso key, ks_hash_t hash, ks_ssize_t* rb, ks_ssize_t* re) {
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
static bool s_fill_holes(ks_dict self) {

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
static bool s_resize(ks_dict self, ks_size_t new_len_buckets) {
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
    for (i = 0; i < self->len_ents; ++i) {
        struct ks_dict_ent* ent = &self->ents[i];
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

ks_dict ks_dict_newt(ks_type tp, struct ks_ikv* ikv) {
    ks_dict self = ks_zmalloc(1, sizeof(*self));
    KS_INCREF(kst_dict);
    self->type = tp;
    self->refs = 1;

    self->len_buckets = self->len_ents = self->len_real = 0;
    self->_max_len_buckets_b = self->_max_len_ents = 0;

    self->ents = NULL;
    self->buckets_s8 = NULL;

    /* Initialize elements */
    if (ikv) {
        struct ks_ikv* p = ikv;
        while (p->key) {
            ks_str k = ks_str_new(-1, p->key);
            ks_dict_set_h(self, (kso)k, k->v_hash, p->val);
            KS_DECREF(k);
            p++;
        }
    }

    return self;
}

ks_dict ks_dict_new(struct ks_ikv* ikv) {
    ks_dict self = ks_zmalloc(1, sizeof(*self));
    KS_INCREF(kst_dict);
    self->type = kst_dict;
    self->refs = 1;

    self->len_buckets = self->len_ents = self->len_real = 0;
    self->_max_len_buckets_b = self->_max_len_ents = 0;

    self->ents = NULL;
    self->buckets_s8 = NULL;

    /* Initialize elements */
    if (ikv) {
        struct ks_ikv* p = ikv;
        while (p->key) {
            ks_str k = ks_str_new(-1, p->key);
            ks_dict_set_h(self, (kso)k, k->v_hash, p->val);
            KS_DECREF(k);
            p++;
        }
    }

    return self;
}
ks_dict ks_dict_newn(struct ks_ikv* ikv) {
    ks_dict res = ks_dict_new(ikv);
    if (ikv) while (ikv->key) {
        /* Absorbs references */
        KS_DECREF(ikv->val);
        ikv++;
    }
    return res;
}

ks_dict ks_dict_newkv(int nargs, kso* args) {
    ks_dict res = ks_dict_new(NULL);

    assert(nargs % 2 == 0);
    int i;
    for (i = 0; i < nargs; i += 2) {
        if (!ks_dict_set(res, args[i], args[i+1])) {
            KS_DECREF(res);
            return NULL;
        }
    }

    return res;
}


void ks_dict_clear(ks_dict self) {

    ks_cint i;
    for (i = 0; i < self->len_ents; ++i) {
        if (self->ents[i].key) {
            KS_DECREF(self->ents[i].key);
            KS_DECREF(self->ents[i].val);
        }
    }

    self->len_ents = 0;
    self->len_buckets = 0;
}



bool ks_dict_merge(ks_dict self, ks_dict from) {
    ks_size_t i;
    for (i = 0; i < from->len_ents; ++i) {
        if (from->ents[i].key) {
            if (!ks_dict_set_h(self, from->ents[i].key, from->ents[i].hash, from->ents[i].val)) return false;
        }
    }

    return true;
}
bool ks_dict_merge_ikv(ks_dict self, struct ks_ikv* ikv) {
    assert(kso_issub(self->type, kst_dict));
    
    bool res = true;
    /* Iterate until given 'NULL' as a key */
    struct ks_ikv* it = ikv;
    if (it) while (it->key) {

        /* If there was a valid dictionary to modify. Otherwise, keep going */
        if (res) {
            ks_str key = ks_str_new(-1, it->key);
            assert(key != NULL);
            kso val = it->val;
            assert(val != NULL);
            bool had_err = !ks_dict_set_h(self, (kso)key, key->v_hash, val);

            /* This function works by taking the references from the list of elements */
            KS_DECREF(key);
            KS_DECREF(val);

            if (had_err) {
                res = false;
            }
        } else {
            /* Still have to get rid of the reference */
            KS_DECREF(it->val);
        }

        it++;
    }

    return res;
}

kso ks_dict_get(ks_dict self, kso key) {
    ks_hash_t hash;
    if (!kso_hash(key, &hash)) return NULL;
    return ks_dict_get_h(self, key, hash);
}

kso ks_dict_get_h(ks_dict self, kso key, ks_hash_t hash) {
    ks_ssize_t rb, re;
    if (!s_search(self, key, hash, &rb, &re)) return NULL;

    if (re < 0) {
        /* Not found */
        KS_THROW_KEY(self, key);
        return NULL;
    } else {
        /* Found */
        return KS_NEWREF(self->ents[re].val);
    }
}
kso ks_dict_get_ih(ks_dict self, kso key, ks_hash_t hash) {
    ks_ssize_t rb, re;
    if (!s_search(self, key, hash, &rb, &re)) return NULL;

    if (re < 0) {
        /* Not found */
        return NULL;
    } else {
        /* Found */
        return KS_NEWREF(self->ents[re].val);
    }
}

kso ks_dict_get_c(ks_dict self, const char* ckey) {
    ks_str key = ks_str_new(-1, ckey);
    kso res = ks_dict_get_h(self, (kso)key, key->v_hash);
    KS_DECREF(key);
    return res;
}

bool ks_dict_set(ks_dict self, kso key, kso val) {
    ks_hash_t hash;
    if (!kso_hash(key, &hash)) return false;
    return ks_dict_set_h(self, key, hash, val);
}
bool ks_dict_set_c1(ks_dict self, const char* ckey, kso val) {
    ks_str key = ks_str_new(-1, ckey);
    bool res = ks_dict_set_h(self, (kso)key, key->v_hash, val);
    KS_DECREF(key);
    KS_DECREF(val);
    return res;
}
bool ks_dict_set_c(ks_dict self, const char* ckey, kso val) {
    ks_str key = ks_str_new(-1, ckey);
    bool res = ks_dict_set_h(self, (kso)key, key->v_hash, val);
    KS_DECREF(key);
    return res;
}
bool ks_dict_set_h(ks_dict self, kso key, ks_hash_t hash, kso val) {
    ks_ssize_t rb, re;

    /* Resize if needed */
    if (s_load(self) > S_LOAD_MAX || self->len_buckets == 0)
        if (!s_resize(self, (ks_size_t)(self->len_ents / S_LOAD_NEW)))
            return false;

    if (!s_search(self, key, hash, &rb, &re)) return false;

    if (re < 0) {
        /* Not found, so add to the dictionary*/
        
        re = self->len_ents++;
        if (self->len_ents > self->_max_len_ents) {
            self->_max_len_ents = ks_nextsize(self->_max_len_ents, self->len_ents);
            self->ents = ks_zrealloc(self->ents, sizeof(*self->ents), self->_max_len_ents);
        }

        /* Add to entries */
        self->len_real++;
        
        KS_INCREF(key);
        KS_INCREF(val);
        
        self->ents[re].hash = hash;
        self->ents[re].key = key;
        self->ents[re].val = val;

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
        /* Found, so replace value*/
        KS_INCREF(val);
        KS_DECREF(self->ents[re].val);
        self->ents[re].val = val;
        return true;
    }
}

bool ks_dict_has(ks_dict self, kso key, bool* exists) {
    ks_hash_t hash;
    if (!kso_hash(key, &hash)) return false;
    return ks_dict_has_h(self, key, hash, exists);
}

bool ks_dict_has_h(ks_dict self, kso key, ks_hash_t hash, bool* exists) {
    ks_ssize_t rb, re;
    if (!s_search(self, key, hash, &rb, &re)) return false;

    *exists = re >= 0;
    return true;
}
bool ks_dict_has_c(ks_dict self, const char* key, bool* exists) {
    ks_str o = ks_str_new(-1, key);
    bool res = ks_dict_has_h(self, (kso)o, o->v_hash, exists);
    KS_DECREF(o);
    return res;
}


bool ks_dict_del(ks_dict self, kso key, bool* existed) {
    ks_hash_t hash;
    if (!kso_hash(key, &hash)) return false;
    return ks_dict_del_h(self, key, hash, existed);
}

bool ks_dict_del_h(ks_dict self, kso key, ks_hash_t hash, bool* existed) {
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

ks_list ks_dict_calc_ents(ks_dict self) {
    ks_list res = ks_list_new(0, NULL);

    ks_ssize_t i;
    for (i = 0; i < self->len_ents; ++i) {
        struct ks_dict_ent* ent = &self->ents[i];
        if (ent->key) {
            ks_int h = ks_int_newu(ent->hash);;
            ks_tuple e = ks_tuple_new(3, (kso[]){
                ent->key,
                (kso)h,
                ent->val
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

ks_list ks_dict_calc_buckets(ks_dict self) {
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
    ks_dict self;
    KS_ARGS("self:*", &self, kst_dict);

    ks_size_t i;
    for (i = 0; i < self->len_ents; ++i) {
        if (self->ents[i].key != NULL) {
            KS_DECREF(self->ents[i].key);
            KS_DECREF(self->ents[i].val);
        }
    }

    ks_free(self->ents);
    ks_free(self->buckets_s8);

    KSO_DEL(self);

    return KSO_NONE;
}


static KS_TFUNC(T, new) {
    ks_type tp;
    kso objs = KSO_NONE;
    KS_ARGS("tp:* ?objs", &tp, kst_type, &objs);

    if (objs == KSO_NONE) {
        return (kso)ks_dict_newt(tp, NULL);
    } else if (kso_issub(objs->type, kst_dict)) {
        ks_dict rr = (ks_dict)objs;
        ks_dict r = ks_dict_newt(tp, NULL);
        ks_cint i;
        for (i = 0; i < rr->len_ents; ++i) {
            if (rr->ents[i].key) {
                if (!ks_dict_set_h(r, rr->ents[i].key, rr->ents[i].hash, rr->ents[i].val)) {
                    KS_DECREF(r);
                    return NULL;
                }
            }
        }
        return (kso)r;
    } else if (objs->type->i__dict) {
        ks_dict rr = (ks_dict)kso_call(objs->type->i__dict, 1, &objs);
        if (!rr) {
            return NULL;
        }
        if (!kso_issub(rr->type, kst_dict)) {
            KS_THROW(kst_TypeError, "'%T.__dict' returned non-dict object of type %T", rr);
            KS_DECREF(rr);
            return NULL;
        }
        if (kso_issub(rr->type, tp)) {
            return (kso)rr;
        } else {
            ks_dict r = ks_dict_newt(tp, NULL);
            ks_cint i;
            for (i = 0; i < rr->len_ents; ++i) {
                if (rr->ents[i].key) {
                    if (!ks_dict_set_h(r, rr->ents[i].key, rr->ents[i].hash, rr->ents[i].val)) {
                        KS_DECREF(r);
                        KS_DECREF(rr);
                        return NULL;
                    }
                }
            }
            KS_DECREF(rr);
            return (kso)r;
        }
    }


    KS_THROW_CONV(objs->type, tp);
    return NULL;
}


static KS_TFUNC(T, bool) {
    ks_dict self;
    KS_ARGS("self:*", &self, kst_dict);

    return (kso)KSO_BOOL(self->len_real != 0);
}

static KS_TFUNC(T, len) {
    ks_dict self;
    KS_ARGS("self:*", &self, kst_dict);

    return (kso)ks_int_newu(self->len_real);
}


static KS_TFUNC(T, contains) {
    ks_dict self;
    kso key;
    KS_ARGS("self:* key", &self, kst_dict, &key);

    bool g;
    if (!ks_dict_has(self, key, &g)) return NULL;


    return KSO_BOOL(g);
}


/** Iterator **/

static KS_TFUNC(TI, free) {
    ks_dict_iter self;
    KS_ARGS("self:*", &self, kst_dict_iter);

    KS_DECREF(self->of);
    KSO_DEL(self);

    return KSO_NONE;
}

static KS_TFUNC(TI, new) {
    ks_type tp;
    ks_dict of;
    KS_ARGS("tp:* of:*", &tp, kst_type, &of, kst_dict);

    ks_dict_iter self = KSO_NEW(ks_dict_iter, tp);

    KS_INCREF(of);
    self->of = of;

    self->pos = 0;

    return (kso)self;
}

/* Export */

static struct ks_type_s tp;
ks_type kst_dict = &tp;

static struct ks_type_s tp_iter;
ks_type kst_dict_iter = &tp_iter;

void _ksi_dict() {

    _ksinit(kst_dict_iter, kst_object, TI_NAME, sizeof(struct ks_dict_s), -1, "", KS_IKV(
        {"__free",               ksf_wrap(TI_free_, TI_NAME ".__free(self)", "")},
        {"__new",                ksf_wrap(TI_new_, TI_NAME ".__new(tp, of)", "")},
    ));

    _ksinit(kst_dict, kst_object, T_NAME, sizeof(struct ks_dict_s), -1, "Dictionaries, sometimes called associative arrays, are mappings between keys and values. The keys and values may be any objects, the only requirement is that keys are hashable. And, for keys which hash equally and compare equally, there is only one key stored\n\n    Entries are ordered by first insertion of the key, which is reset upon deletion\n\n    SEE: https://en.wikipedia.org/wiki/Associative_array", KS_IKV(
        {"__free",               ksf_wrap(T_free_, T_NAME ".__free(self)", "")},
        {"__new",                ksf_wrap(T_new_, T_NAME ".__new(tp, objs)", "")},
        {"__bool",                 ksf_wrap(T_bool_, T_NAME ".__bool(self)", "")},
        {"__len",                  ksf_wrap(T_len_, T_NAME ".__len(self)", "")},
        {"__contains",             ksf_wrap(T_contains_, T_NAME ".__contains(self, key)", "")},

        {"__iter",               KS_NEWREF(kst_dict_iter)},
        
    ));
    
    kst_dict->i__hash = NULL;
}
