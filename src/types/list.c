/* types/list.c - 'list' type
 *
 *
 * @author: Cade Brown <cade@kscript.org>
 */
#include <ks/impl.h>

#define T_NAME "list"



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

/* Export */

static struct ks_type_s tp;
ks_type kst_list = &tp;

void _ksi_list() {
    _ksinit(kst_list, kst_object, T_NAME, sizeof(struct ks_list_s), -1, KS_IKV(
        {"__free",               ksf_wrap(T_free_, T_NAME ".__free(self)", "")},
    ));
}
