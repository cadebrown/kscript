/* types/list.c - 'list' type
 *
 *
 * @author: Cade Brown <cade@kscript.org>
 */
#include <ks/impl.h>

#define T_NAME "list"
#define TI_NAME T_NAME ".__iter"


/* C-API */

ks_list ks_list_new(ks_ssize_t len, kso* elems) {
    ks_list self = KSO_NEW(ks_list, kst_list);

    self->len = len;
    self->_max_len = len;
    self->elems = ks_zmalloc(sizeof(*self->elems), len);

    ks_ssize_t i;
    for (i = 0; i < len; ++i) {
        KS_INCREF(elems[i]);
        self->elems[i] = elems[i];
    }

    return self;
}

ks_list ks_list_newn(ks_ssize_t len, kso* elems) {
    ks_list self = KSO_NEW(ks_list, kst_list);

    self->len = len;
    self->_max_len = len;
    self->elems = ks_zmalloc(sizeof(*self->elems), len);

    ks_ssize_t i;
    for (i = 0; i < len; ++i) {
        self->elems[i] = elems[i];
    }

    return self;
}


ks_list ks_list_newit(ks_type tp, kso objs) {
    ks_list res = KSO_NEW(ks_list, tp);

    /* Build the tuple */
    res->len = 0;
    res->elems = NULL;
    res->_max_len = 0;

    ks_cit it = ks_cit_make(objs);
    kso ob;
    while ((ob = ks_cit_next(&it)) != NULL) {
        ks_ssize_t idx = res->len++;
        if (res->len > res->_max_len) {
            res->_max_len = ks_nextsize(res->len, res->_max_len);
            res->elems = ks_zrealloc(res->elems, sizeof(*res->elems), res->_max_len);
        }

        /* Absorb reference */
        res->elems[idx] = ob;
    }

    ks_cit_done(&it);
    if (it.exc) {
        KS_DECREF(res);
        return NULL;
    }

    return res;
}
ks_list ks_list_newi(kso objs) {
    return ks_list_newit(kst_list, objs);
}
bool ks_list_reserve(ks_list self, int cap) {
    if (cap > self->_max_len) {
        self->_max_len = ks_nextsize(self->_max_len, cap);
        self->elems = ks_zrealloc(self->elems, sizeof(*self->elems), self->_max_len);
    }
    return true;
}

void ks_list_clear(ks_list self) {
    ks_size_t i;
    for (i = 0; i < self->len; ++i) {
        KS_DECREF(self->elems[i]);
    }
    self->len = 0;
}

bool ks_list_push(ks_list self, kso ob) {
    bool res = ks_list_pushu(self, ob);
    if (res) KS_INCREF(ob);
    return res;
}

bool ks_list_pushu(ks_list self, kso ob) {
    ks_size_t i = self->len++;
    if (self->len > self->_max_len) {
        self->_max_len = ks_nextsize(self->len, self->_max_len);
        self->elems = ks_zrealloc(self->elems, sizeof(*self->elems), self->_max_len);
    }
    self->elems[i] = ob;
    return true;
}
bool ks_list_pushan(ks_list self, ks_cint len, kso* objs) {
    ks_ssize_t i = self->len;
    self->len += len;
    if (self->len > self->_max_len) {
        self->_max_len = ks_nextsize(self->_max_len, self->len);
        self->elems = ks_zrealloc(self->elems, sizeof(*self->elems), self->_max_len);
    }

    memcpy(self->elems + i, objs, len * sizeof(*objs));
    return true;
}


bool ks_list_pusha(ks_list self, ks_cint len, kso* objs) {
    ks_ssize_t i = self->len;
    self->len += len;
    if (self->len > self->_max_len) {
        self->_max_len = ks_nextsize(self->_max_len, self->len);
        self->elems = ks_zrealloc(self->elems, sizeof(*self->elems), self->_max_len);
    }

    memcpy(self->elems + i, objs, len * sizeof(*objs));
    for (i = 0; i < len; ++i) {
        KS_INCREF(objs[i]);
    }

    return true;
}


bool ks_list_pushall(ks_list self, kso objs) {
    if (kso_issub(objs->type, kst_list)) {
        return ks_list_pusha(self, ((ks_list)objs)->len, ((ks_list)objs)->elems);
    } else if (kso_issub(objs->type, kst_tuple)) {
        return ks_list_pusha(self, ((ks_tuple)objs)->len, ((ks_tuple)objs)->elems);
    } else {
        ks_cit it = ks_cit_make(objs);
        kso ob;
        while (ob = ks_cit_next(&it)) {
            if (!ks_list_push(self, ob)) {
                KS_DECREF(ob);
                ks_cit_done(&it);
                return false;
            }
            KS_DECREF(ob);
        }
        ks_cit_done(&it);
        if (it.exc) return false;

        return true;
    }
}
bool ks_list_insert(ks_list self, ks_cint idx, kso ob) {
    bool res = ks_list_insertu(self, idx, ob);
    KS_INCREF(ob);
    return res;
}


bool ks_list_insertu(ks_list self, ks_cint idx, kso ob) {
    self->len++;
    if (self->len > self->_max_len) {
        self->_max_len = ks_nextsize(self->len, self->_max_len);
        self->elems = ks_zrealloc(self->elems, sizeof(*self->elems), self->_max_len);
    }

    ks_ssize_t i;
    for (i = self->len - 1; i > idx; --i) {
        self->elems[i] = self->elems[i - 1];
    }
    self->elems[idx] = ob;
    return true;
}



kso ks_list_pop(ks_list self) {
    return self->elems[--self->len];
}

void ks_list_popu(ks_list self) {
    kso el = self->elems[--self->len];
    KS_DECREF(el);
}


bool ks_list_del(ks_list self, ks_cint idx) {
    if (idx < 0) idx += self->len;

    KS_DECREF(self->elems[idx]);

    ks_cint i;
    for (i = idx; i < self->len - 1; ++i) {
        self->elems[i] = self->elems[i + 1];
    }

    self->len--;

    return true;
}

/* Type Functions */

static KS_TFUNC(T, free) {
    ks_list self;
    KS_ARGS("self:*", &self, kst_list);

    ks_size_t i;
    for (i = 0; i < self->len; ++i) {
        KS_DECREF(self->elems[i]);
    }
    ks_free(self->elems);

    KSO_DEL(self);

    return KSO_NONE;
}

static KS_TFUNC(T, new) {
    ks_type tp;
    int nargs;
    kso* args;
    KS_ARGS("tp:* *args", &tp, kst_type, &nargs, &args);

    ks_list self = KSO_NEW(ks_list, tp);

    self->len = self->_max_len = 0;
    self->elems = NULL;

    return (kso)self;
}

static KS_TFUNC(T, init) {
    ks_list self;
    kso objs = KSO_NONE;
    KS_ARGS("self:* ?objs", &self, kst_list, &objs);

    ks_list_clear(self);

    if (!ks_list_pushall(self, objs)) {
        return NULL;
    }

    return KSO_NONE;
}

static KS_TFUNC(T, bool) {
    ks_list self;
    KS_ARGS("self:*", &self, kst_list);

    return (kso)KSO_BOOL(self->len != 0);
}


static KS_TFUNC(T, len) {
    ks_list self;
    KS_ARGS("self:*", &self, kst_list);

    return (kso)ks_int_newu(self->len);
}


static KS_TFUNC(T, eq) {
    kso L, R;
    KS_ARGS("L R", &L, &R);

    if (kso_issub(L->type, kst_list) && kso_issub(R->type, kst_list)) {
        ks_list lL = (ks_list)L, lR = (ks_list)R;

        if (lL->len != lR->len) return KSO_FALSE;

        ks_cint i;
        for (i = 0; i < lL->len; ++i) {
            kso a = lL->elems[i], b = lR->elems[i];
            if (a == b) continue;

            bool g;
            if (!kso_eq(a, b, &g)) {
                return NULL;
            }

            if (!g) return KSO_FALSE;
        }

        return KSO_TRUE;
    }

    return KSO_UNDEFINED;
}

static KS_TFUNC(T, add) {
    kso L, R;
    KS_ARGS("L R", &L, &R);

    ks_list res = ks_list_new(0, NULL);

    if (!ks_list_pushall(res, L)) {
        KS_DECREF(res);
        return NULL;
    }

    if (!ks_list_pushall(res, R)) {
        KS_DECREF(res);
        return NULL;
    }
    return (kso)res;
}

static KS_TFUNC(T, mul) {
    kso L, R;
    KS_ARGS("L R", &L, &R);

    if (kso_issub(L->type, kst_list) && kso_is_int(R)) {
        ks_cint ct;
        if (!kso_get_ci(R, &ct)) {
            return NULL;
        }
        ks_list res = ks_list_new(0, NULL);
        while (ct > 0) {
            ks_list_pusha(res, ((ks_list)L)->len, ((ks_list)L)->elems);
            ct--;
        }

        return (kso)res;

    } else if (kso_issub(R->type, kst_list) && kso_is_int(L)) {
        ks_cint ct;
        if (!kso_get_ci(L, &ct)) {
            return NULL;
        }
        ks_list res = ks_list_new(0, NULL);
        while (ct > 0) {
            ks_list_pusha(res, ((ks_list)R)->len, ((ks_list)R)->elems);
            ct--;
        }

        return (kso)res;
    }

    return KSO_UNDEFINED;
}

static KS_TFUNC(T, push) {
    ks_list self;
    int nargs;
    kso* args;
    KS_ARGS("self:* *args", &self, kst_list, &nargs, &args);

    if (!ks_list_pusha(self, nargs, args)) return NULL;

    return KSO_NONE;
}

static KS_TFUNC(T, pop) {
    ks_list self;
    ks_cint num = 1;
    KS_ARGS("self:* ?num:cint", &self, kst_list, &num);

    if (num > self->len) {
        KS_THROW(kst_Error, "Attempted to pop more items than existed in list");
        return NULL;
    }

    if (num < 0) {
        KS_THROW(kst_Error, "'num' must be positive or 0");
        return NULL;
    }

    if (_nargs == 1) {
        return (kso)ks_list_pop(self);
    } else {
        ks_list res = ks_list_newn(num, self->elems + self->len - num);
        self->len -= num;
        return (kso)res;
    }
}
static KS_TFUNC(T, index) {
    ks_list self;
    kso elem;
    KS_ARGS("self:* elem", &self, kst_list, &elem);

    ks_cint i;
    for (i = 0; i < self->len; ++i) {
        kso ob = self->elems[i];
        if (ob == elem) {
            return (kso)ks_int_new(i);
        }

        bool eq;
        if (!kso_eq(elem, ob, &eq)) {
            return NULL;
        } else if (eq) {
            return (kso)ks_int_new(i);
        }

    }


    KS_THROW_VAL(self, elem);
    return NULL;
}

static KS_TFUNC(T, sort) {
    ks_list self;
    KS_ARGS("self:*", &self, kst_list);

    if (!ks_sort(self->len, self->elems, self->elems, NULL)) {
        return NULL;
    }

    return KSO_NONE;
}

/** Iterator **/

static KS_TFUNC(TI, free) {
    ks_list_iter self;
    KS_ARGS("self:*", &self, kst_list_iter);

    KS_DECREF(self->of);
    KSO_DEL(self);

    return KSO_NONE;
}

static KS_TFUNC(TI, new) {
    ks_type tp;
    ks_list of;
    KS_ARGS("tp:* of:*", &tp, kst_type, &of, kst_list);

    ks_list_iter self = KSO_NEW(ks_list_iter, tp);

    KS_INCREF(of);
    self->of = of;

    self->pos = 0;

    return (kso)self;
}


/* Export */

static struct ks_type_s tp;
ks_type kst_list = &tp;

static struct ks_type_s tp_iter;
ks_type kst_list_iter = &tp_iter;

void _ksi_list() {

    _ksinit(kst_list_iter, kst_object, TI_NAME, sizeof(struct ks_list_iter_s), -1, "", KS_IKV(
        {"__free",               ksf_wrap(TI_free_, TI_NAME ".__free(self)", "")},
        {"__new",                ksf_wrap(TI_new_, TI_NAME ".__new(tp, of)", "")},
    ));

    _ksinit(kst_list, kst_object, T_NAME, sizeof(struct ks_list_s), -1, "List of references to other objects, which is mutable\n\n    Internally, a 'list' is not a linked-list-like data structure, but closer to an array. Specifically, it is an array of references, so children are not copied or duplicated, only a reference is made to them", KS_IKV(
        {"__free",               ksf_wrap(T_free_, T_NAME ".__free(self)", "")},
        {"__new",                ksf_wrap(T_new_, T_NAME ".__new(tp, *args)", "")},
        {"__init",               ksf_wrap(T_init_, T_NAME ".__init(self, objs=none)", "")},
        {"__bool",               ksf_wrap(T_bool_, T_NAME ".__bool(self)", "")},
        {"__len",                ksf_wrap(T_len_, T_NAME ".__len(self)", "")},
        {"__iter",               KS_NEWREF(kst_list_iter)},
        {"__eq",                 ksf_wrap(T_eq_, T_NAME ".__eq(L, R)", "")},
        {"__add",                ksf_wrap(T_add_, T_NAME ".__add(L, R)", "")},
        {"__mul",                ksf_wrap(T_mul_, T_NAME ".__mul(L, R)", "")},

        {"push",                 ksf_wrap(T_push_, T_NAME ".push(self, *args)", "Pushes any number of arguments on to the end of the list")},
        {"pop",                  ksf_wrap(T_pop_, T_NAME ".pop(self, num=1)", "Pops the given number of arguments off of the end of the list")},
        {"index",                ksf_wrap(T_index_, T_NAME ".index(self, elem)", "Calculates the index of an element")},

        {"sort",                 ksf_wrap(T_sort_, T_NAME ".sort(self)", "Sorts, in place, the list")},

    ));

    kst_list->i__hash = NULL;
}
