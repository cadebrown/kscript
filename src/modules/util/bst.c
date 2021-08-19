/* util/bst.c - 'util.BST' type
 *
 * @author: Cade Brown <cade@kscript.org>
 */
#include <ks/impl.h>

#define T_NAME "util.BST"
#define TI_NAME "util.BST.__iter"


/* Internals */

/* Utility function to allocate item */
static struct ks_bst_item* make_item(kso key, kso val) {
    struct ks_bst_item* it = ks_malloc(sizeof(*it));

    it->left = it->right = NULL;

    KS_INCREF(key);
    it->key = key;
    KS_INCREF(val);
    it->val = val;

    return it;
}



/* Utility function to determine whether the binary search tree contains a key */
static bool bst_has(ks_bst self, struct ks_bst_item* it, kso key, bool* out) {
    if (!it) {
        *out = false;
        return true;
    }

    int cmp;
    if (!kso_cmp(it->key, key, &cmp)) {
        return false;
    }

    if (cmp == 0) {
        *out = true;
        return true;
    } else if (cmp > 0) {
        return bst_has(self, it->left, key, out);
    } else {
        return bst_has(self, it->right, key, out);
    }
}


static bool bst_get(ks_bst self, struct ks_bst_item* it, kso key, kso* out) {
    if (!it) {
        KS_THROW_KEY(self, key);
        return false;
    }

    int cmp;
    if (!kso_cmp(it->key, key, &cmp)) {
        return false;
    }

    if (cmp == 0) {
        KS_INCREF(it->val);
        *out = it->val;
        return true;
    } else if (cmp < 0) {
        return bst_get(self, it->left, key, out);
    } else {
        return bst_get(self, it->right, key, out);
    }
}


/* C-API */

ks_bst ks_bst_new(ks_type tp) {
    ks_bst self = KSO_NEW(ks_bst, tp);

    self->root = NULL;

    return self;
}


bool ks_bst_has(ks_bst self, kso key, bool* out) {
    return bst_has(self, self->root, key, out);
}


kso ks_bst_get(ks_bst self, kso key) {
    kso res;
    if (!bst_get(self, self->root, key, &res)) return NULL;
    return res;
}


static bool bst_set(struct ks_bst_item** it, kso key, kso val) {
    if (!*it) {
        *it = make_item(key, val);
        return true;
    }

    int cmp;
    if (!kso_cmp((*it)->key, key, &cmp)) {
        return false;
    }

    if (cmp == 0) {
        KS_INCREF(val);
        KS_DECREF((*it)->val);
        (*it)->val = val;
        return true;
    } else if (cmp < 0) {
        return bst_set(&(*it)->right, key, val);
    } else {
        return bst_set(&(*it)->left, key, val);
    }

}

bool ks_bst_set(ks_bst self, kso key, kso val) {
    return bst_set(&self->root, key, val);
}



/* Type Functions */

static KS_TFUNC(T, free) {
    ks_bst self;
    KS_ARGS("self:*", &self, kst_bst);

    KSO_DEL(self);

    return KSO_NONE;
}


static KS_TFUNC(T, new) {
    ks_type tp;
    kso objs = KSO_NONE;
    KS_ARGS("tp:* ?objs", &tp, kst_type, &objs);

    ks_bst self = ks_bst_new(tp);

    if (objs == KSO_NONE) {

    } else {
        ks_dict elems = (ks_dict)kso_call((kso)kst_dict, 1, &objs);
        if (!elems) {
            KS_DECREF(self);
            return NULL;
        }

        ks_cint i;
        for (i = 0; i < elems->len_ents; ++i) {
            if (elems->ents[i].key) {
                if (!ks_bst_set(self, elems->ents[i].key, elems->ents[i].val)) {
                    KS_DECREF(self);
                    KS_DECREF(elems);
                    return NULL;
                }
            }
        }
        KS_DECREF(elems);
    }

    return (kso)self;
}

static bool bst_fill_io(ks_bst self, struct ks_bst_item* it, ksio_BaseIO to, bool* has) {
    if (!it) return true;
    if (!bst_fill_io(self, it->left, to, has)) {
        return false;
    }

    if (!*has) {
        *has = true;
    } else {
        ksio_add(to, ", ");
    }

    if (!ksio_add(to, "%R: %R", it->key, it->val)) {
        return false;
    }

    if (!bst_fill_io(self, it->right, to, has)) {
        return false;
    } 

    return true;
}
static KS_TFUNC(T, str) {
    ks_bst self;
    KS_ARGS("self:*", &self, kst_bst);

    if (!self->root) return (kso)ks_fmt("%T()", self);

    ksio_StringIO sio = ksio_StringIO_new();
    bool has = false;

    ksio_add(sio, "%T({", self);

    if (!bst_fill_io(self, self->root, (ksio_BaseIO)sio, &has)) {
        KS_DECREF(sio);
        return NULL;
    }
    ksio_add(sio, "})");

    return (kso)ksio_StringIO_getf(sio);
}

static KS_TFUNC(T, bool) {
    ks_bst self;
    KS_ARGS("self:*", &self, kst_bst);

    return KSO_BOOL(self->root != NULL);
}

static bool bst_fill_dict(ks_bst self, struct ks_bst_item* it, ks_dict to) {
    if (!it) return true;
    if (!bst_fill_dict(self, it->left, to)) {
        return false;
    } 
    if (!ks_dict_set(to, it->key, it->val)) {
        return false;
    }

    if (!bst_fill_dict(self, it->right, to)) {
        return false;
    } 

    return true;
}

static KS_TFUNC(T, dict) {
    ks_bst self;
    KS_ARGS("self:*", &self, kst_bst);

    ks_dict res = ks_dict_new(NULL);
    if (!bst_fill_dict(self, self->root, res)) {
        KS_DECREF(res);
        return NULL;
    }
    return (kso)res;
}

static ks_cint bst_len(struct ks_bst_item* it) {
    if (!it) return 0;

    ks_cint sublen = 0;
    if (it->left) {
        sublen = bst_len(it->left);
    }

    if (it->right) {
        ks_cint rl = bst_len(it->right);
        if (rl > sublen) sublen = rl;
    }

    return 1 + sublen;
}

static KS_TFUNC(T, len) {
    ks_bst self;
    KS_ARGS("self:*", &self, kst_bst);

    return (kso)ks_int_new(bst_len(self->root));
}

static KS_TFUNC(T, contains) {
    ks_bst self;
    kso key;
    KS_ARGS("self:* key", &self, kst_bst, &key);
    bool res;
    if (!ks_bst_has(self, key, &res)) return NULL;
    return KSO_BOOL(res);
}

static KS_TFUNC(T, getelem) {
    ks_bst self;
    kso key;
    KS_ARGS("self:* key", &self, kst_bst, &key);

    return (kso)ks_bst_get(self, key);
}

static KS_TFUNC(T, setelem) {
    ks_bst self;
    kso key, val;
    KS_ARGS("self:* key val", &self, kst_bst, &key, &val);

    if (!ks_bst_set(self, key, val)) {
        return NULL;
    }

    return KSO_NONE;
}


/* Iterator Type */

static KS_TFUNC(TI, free) {
    ks_bst_iter self;
    KS_ARGS("self:*", &self, kst_bst_iter);

    KS_DECREF(self->of);

    KSO_DEL(self);

    return KSO_NONE;
}

static KS_TFUNC(TI, init) {
    ks_bst_iter self;
    ks_bst of;
    KS_ARGS("self:* of:*", &self, kst_bst_iter, &of, kst_bst);

    KS_INCREF(of);
    self->of = of;

    self->cur = of->root;

    self->nstk = 0;
    self->stk = NULL;

    return KSO_NONE;
}

static KS_TFUNC(TI, next) {
    ks_bst_iter self;
    KS_ARGS("self:*", &self, kst_bst_iter);
    
    ti_next_restart: ;

    if (self->cur) {
        int i = self->nstk++;
        self->stk = ks_zrealloc(self->stk, sizeof(*self->stk), self->nstk);
        self->stk[i] = self->cur;
        self->cur = self->cur->left;

        goto ti_next_restart;

    } else if (self->nstk > 0) {

        self->cur = self->stk[--self->nstk];

        kso r = self->cur->key;
        KS_INCREF(r);

        self->cur = self->cur->right;
        return r;
    }

    /* Nothing in the iterator */
    KS_OUTOFITER();
    return NULL;
}



/* Export */

static struct ks_type_s tp;
ks_type kst_bst = &tp;

static struct ks_type_s tpi;
ks_type kst_bst_iter = &tpi;

void _ksi_bst() {

    _ksinit(kst_bst_iter, kst_object, TI_NAME, sizeof(struct ks_bst_iter_s), -1, "", KS_IKV(
        {"__free",                 ksf_wrap(TI_free_, TI_NAME ".__free(self)", "")},
        {"__init",                 ksf_wrap(TI_init_, TI_NAME ".__init(self, of)", "")},
        {"__next",                 ksf_wrap(TI_next_, TI_NAME ".__next(self)", "")},

    ));

    _ksinit(kst_bst, kst_object, T_NAME, sizeof(struct ks_bst_s), -1, "Binary search trees are mapping objects that have sorted keys", KS_IKV(
        {"__free",                 ksf_wrap(T_free_, T_NAME ".__free(self)", "")},
        {"__new",                  ksf_wrap(T_new_, T_NAME ".__new(tp, objs=none)", "")},
        {"__repr",                 ksf_wrap(T_str_, T_NAME ".__repr(self)", "")},
        {"__str",                  ksf_wrap(T_str_, T_NAME ".__str(self)", "")},
        {"__bool",                 ksf_wrap(T_bool_, T_NAME ".__bool(self)", "")},
        {"__dict",                 ksf_wrap(T_dict_, T_NAME ".__dict(self)", "")},
        {"__getelem",              ksf_wrap(T_getelem_, T_NAME ".__getelem(self, key)", "")},
        {"__setelem",              ksf_wrap(T_setelem_, T_NAME ".__setelem(self, key, val)", "")},
        {"__contains",             ksf_wrap(T_contains_, T_NAME ".__contains(self, key)", "")},
        {"__len",                  ksf_wrap(T_len_, T_NAME ".__len(self)", "")},

        {"__iter",                 KS_NEWREF(kst_bst_iter)},


        /*
        {"__integral",             ksf_wrap(T_int_, T_NAME ".__integral(self)", "")},
        */

    ));


}
