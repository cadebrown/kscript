/* util/queue.c - 'util.Queue' type
 *
 * @author: Cade Brown <cade@kscript.org>
 */
#include <ks/impl.h>

#define T_NAME "util.Queue"
#define TI_NAME "util.Queue.__iter"


/* Internals */


static struct ks_queue_item *make_item(kso obj) {
    struct ks_queue_item *it = ks_malloc(sizeof(*it));
    assert(it != NULL);

    it->prev = it->next = NULL;

    KS_INCREF(obj);
    it->val = obj;

    return it;
}


/* C-API */

ks_queue ks_queue_new(ks_type tp) {
    ks_queue self = KSO_NEW(ks_queue, tp);

    self->first = self->last = NULL;

    return self;
}

bool ks_queue_push(ks_queue self, kso obj) {
    struct ks_queue_item *it = make_item(obj);
    if (!it) return false;

    if (ks_queue_empty(self)) {
        self->first = self->last = it;
    } else {
        self->last->next = it;
        it->prev = self->last;
        self->last = it;
    }

    return true;
}

kso ks_queue_pop(ks_queue self) {
    struct ks_queue_item *it = self->first;
    if (!it) {
        KS_THROW(kst_SizeError, "Cannot pop from empty queue");
        return NULL;
    }

    self->first = it->next;
    self->first->prev = NULL;

    kso rr = it->val;
    ks_free(it);

    return rr;
}

bool ks_queue_empty(ks_queue self) {
    return !self->first;
}

/* Type Functions */

static KS_TFUNC(T, free) {
    ks_queue self;
    KS_ARGS("self:*", &self, kst_queue);

    struct ks_queue_item *it = self->first;
    while (it) {
        KS_DECREF(it->val);
        it = it->next;
    }

    KSO_DEL(self);

    return KSO_NONE;
}


static KS_TFUNC(T, init) {
    ks_queue self;
    kso objs = KSO_NONE;
    KS_ARGS("self:* ?objs", &self, kst_queue, &objs);

    self->first = self->last = NULL;

    if (objs != KSO_NONE) {

        ks_cit it = ks_cit_make(objs);
        kso ob;
        while ((ob = ks_cit_next(&it)) != NULL) {
            if (!ks_queue_push(self, ob)) {
                it.exc = true;
            }
            KS_DECREF(ob);
        }

        ks_cit_done(&it);
        if (it.exc) {
            return NULL;
        }
    }

    return KSO_NONE;
}


static KS_TFUNC(T, str) {
    ks_queue self;
    KS_ARGS("self:*", &self, kst_queue);

    return (kso)ks_fmt("%O", self);
}

static KS_TFUNC(T, bool) {
    ks_queue self;
    KS_ARGS("self:*", &self, kst_queue);

    return KSO_BOOL(!ks_queue_empty(self));
}

static KS_TFUNC(T, len) {
    ks_queue self;
    KS_ARGS("self:*", &self, kst_queue);

    ks_cint ct = 0;
    struct ks_queue_item *it = self->first;
    while (it) {
        ct++;
        it = it->next;
    }

    return (kso)ks_int_newu(ct);
}

static KS_TFUNC(T, push) {
    ks_queue self;
    int nargs;
    kso* args;
    KS_ARGS("self:* *args", &self, kst_queue, &nargs, &args);

    int i;
    for (i = 0; i < nargs; ++i) {
        if (!ks_queue_push(self, args[i])) {
            return NULL;
        }
    }

    return KSO_NONE;
}

static KS_TFUNC(T, pop) {
    ks_queue self;
    ks_cint num = 1;
    KS_ARGS("self:* ?num:cint", &self, kst_queue, &num);

    if (_nargs == 1) {
        return ks_queue_pop(self);
    } else {
        ks_list res = ks_list_new(0, NULL);
        ks_cint i;
        for (i = 0; i < num; ++i) {
            kso ob = ks_queue_pop(self);
            if (!ob) {
                KS_DECREF(res);
                return NULL;
            }

            if (!ks_list_push(res, ob)) {
                KS_DECREF(res);
                KS_DECREF(ob);
                return NULL;
            }
            KS_DECREF(ob);
        }
        return (kso)res;
    }
}

/* Iterator Type */

static KS_TFUNC(TI, free) {
    ks_queue_iter self;
    KS_ARGS("self:*", &self, kst_queue_iter);

    KS_DECREF(self->of);

    KSO_DEL(self);

    return KSO_NONE;
}

static KS_TFUNC(TI, init) {
    ks_queue_iter self;
    ks_queue of;
    KS_ARGS("self:* of:*", &self, kst_queue_iter, &of, kst_queue);

    KS_INCREF(of);
    self->of = of;

    self->cur = of->first;

    return KSO_NONE;
}

static KS_TFUNC(TI, next) {
    ks_queue_iter self;
    KS_ARGS("self:*", &self, kst_queue_iter);

    if (!self->cur) {
        KS_OUTOFITER();
        return NULL;
    }

    kso ob = self->cur->val;
    self->cur = self->cur->next;
    KS_INCREF(ob);
    return ob;
}

static KS_TFUNC(TI, prev) {
    ks_queue_iter self;
    KS_ARGS("self:*", &self, kst_queue_iter);

    if (!self->cur) {
        KS_OUTOFITER();
        return NULL;
    }

    kso ob = self->cur->val;
    self->cur = self->cur->prev;
    KS_INCREF(ob);
    return ob;
}


/* Export */

static struct ks_type_s tp;
ks_type kst_queue = &tp;

static struct ks_type_s tpi;
ks_type kst_queue_iter = &tpi;

void _ksi_queue() {

    _ksinit(kst_queue_iter, kst_object, TI_NAME, sizeof(struct ks_queue_iter_s), -1, "", KS_IKV(
        {"__free",                 ksf_wrap(TI_free_, TI_NAME ".__free(self)", "")},
        {"__init",                 ksf_wrap(TI_init_, TI_NAME ".__init(self, of)", "")},
        {"__next",                 ksf_wrap(TI_next_, TI_NAME ".__next(self)", "")},
        {"__prev",                 ksf_wrap(TI_prev_, TI_NAME ".__prev(self)", "")},

    ));

    _ksinit(kst_queue, kst_object, T_NAME, sizeof(struct ks_queue_s), -1, "Queues are efficient ways of making lists which are poppable from both sides", KS_IKV(
        {"__free",                 ksf_wrap(T_free_, T_NAME ".__free(self)", "")},
        {"__init",                 ksf_wrap(T_init_, T_NAME ".__init(self, objs=none)", "")},
        {"__repr",                 ksf_wrap(T_str_, T_NAME ".__repr(self)", "")},
        {"__str",                  ksf_wrap(T_str_, T_NAME ".__str(self)", "")},
        {"__bool",                 ksf_wrap(T_bool_, T_NAME ".__bool(self)", "")},
        {"__len",                  ksf_wrap(T_len_, T_NAME ".__len(self)", "")},
        {"__iter",                 KS_NEWREF(kst_queue_iter)},


        {"push",                   ksf_wrap(T_push_, T_NAME ".push(self, *args)", "Pushes all arguments to the back of the queue")},
        {"pop",                    ksf_wrap(T_pop_, T_NAME ".pop(self)", "Pops off the front of the queue")},

    ));


}
