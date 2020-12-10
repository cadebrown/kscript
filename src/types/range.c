/* types/range.c - 'range' type
 *
 * @author: Cade Brown <cade@kscript.org>
 */
#include <ks/impl.h>

#define T_NAME "range"
#define TI_NAME T_NAME ".__iter"


/* C-API */

ks_range ks_range_new(ks_type tp, ks_int start, ks_int end, ks_int step) {
    ks_range self = KSO_NEW(ks_range, tp);

    KS_INCREF(start);
    self->start = start;
    KS_INCREF(end);
    self->end = end;
    KS_INCREF(step);
    self->step = step;

    return self;
}


/* Type Functions */

static KS_TFUNC(T, new) {
    ks_type tp;
    int n_args;
    kso* args;
    KS_ARGS("tp:* *args", &tp, kst_type, &n_args, &args);

    if (n_args == 1) {
        ks_int end = kso_int(args[0]);
        if (!end) return NULL;
        ks_range res = ks_range_new(tp, _ksint_0, end, _ksint_1);
        KS_DECREF(end);
        return (kso)res;
    } else if (n_args == 2) {
        ks_int start = kso_int(args[0]), end = kso_int(args[1]);
        ks_range res = ks_range_new(tp, start, end, _ksint_1);
        KS_DECREF(start);
        KS_DECREF(end);
        return (kso)res;
    } else if (n_args == 3) {
        ks_int start = kso_int(args[0]), end = kso_int(args[1]), step = kso_int(args[2]);
        ks_range res = ks_range_new(tp, start, end, step);
        KS_DECREF(start);
        KS_DECREF(end);
        KS_DECREF(step);
        return (kso)res;
    }

    KS_THROW(kst_ArgError, "Invalid argument combination, must have 'range(end)', 'range(start, end)', or 'range(start, end, step)'");
    return NULL;
}

static KS_TFUNC(T, free) {
    ks_range self;
    KS_ARGS("self:*", &self, kst_range);

    KS_DECREF(self->start);
    KS_DECREF(self->end);
    KS_DECREF(self->step);

    KSO_DEL(self);

    return KSO_NONE;
}

static KS_TFUNC(T, str) {
    ks_range self;
    KS_ARGS("self:*", &self, kst_range);

    if (self->step == _ksint_1) {
        if (self->start == _ksint_0) {
            return (kso)ks_fmt("%T(%R)", self, self->end);
        } else {
            return (kso)ks_fmt("%T(%R, %R)", self, self->start, self->end);
        }
    } else {
        return (kso)ks_fmt("%T(%R, %R, %R)", self, self->start, self->end, self->step);
    }
}

static KS_TFUNC(T, len) {
    ks_range self;
    KS_ARGS("self:*", &self, kst_range);

    if (ks_int_cmp_c(self->step, 0) == 0) {
        return (kso)ks_int_new(0);
    }

    ks_int tmp = (ks_int)ks_bop_sub((kso)self->end, (kso)self->start);
    if (!tmp) return NULL;
    ks_int res = (ks_int)ks_bop_floordiv((kso)tmp, (kso)self->step);
    KS_DECREF(tmp);
    if (!res) return NULL;

    if (ks_int_cmp_c(res, 0) < 0) {
        KS_DECREF(res);
        return (kso)ks_int_new(0);
    } else {
        return (kso)res;
    }
}

static KS_TFUNC(T, contains) {
    ks_range self;
    kso obj;
    KS_ARGS("self:* obj", &self, kst_range, &obj);
    
    ks_int iv = kso_int(obj);
    if (!iv) return NULL;

    int sc = ks_int_cmp_c(self->step, 0);
    if (sc == 0) {
        KS_DECREF(iv);
        return KSO_FALSE;
    } else if (ks_int_cmp_c(self->step, 1) == 0) {
        bool res = ks_int_cmp(iv, self->start) >= 0 && ks_int_cmp(iv, self->end) < 0;
        KS_DECREF(iv);
        return KSO_BOOL(res);
    } else {
        bool res = ((sc > 0 && ks_int_cmp(iv, self->start) >= 0 && ks_int_cmp(iv, self->end) < 0) || (sc < 0 && ks_int_cmp(iv, self->start) <= 0 && ks_int_cmp(iv, self->end) > 0));
        if (!res) {
            KS_DECREF(iv);
            return KSO_FALSE;
        }

        /* Must also test congruence */
        ks_int tmp = (ks_int)ks_bop_sub((kso)iv, (kso)self->start);
        ks_int mv = (ks_int)ks_bop_mod((kso)tmp, (kso)self->step);
        KS_DECREF(tmp);

        res = ks_int_cmp_c(mv, 0) == 0;
        KS_DECREF(mv);
        return KSO_BOOL(res);
    }
}

/* Iterator type */

static KS_TFUNC(TI, free) {
    ks_range_iter self;
    KS_ARGS("self:*", &self, kst_range_iter);

    KS_DECREF(self->of);
    KS_NDECREF(self->cur);

    KSO_DEL(self);

    return KSO_NONE;
}

static KS_TFUNC(TI, init) {
    ks_range_iter self;
    ks_range of;
    KS_ARGS("self:* of:*", &self, kst_range_iter, &of, kst_range);

    KS_INCREF(of);
    self->of = of;
    self->cur = NULL;
    self->done = false;
    self->use_ci = false;
    self->cmp_step_0 = ks_int_cmp_c(self->of->step, 0);

    if (!kso_get_ci((kso)self->of->start, &self->_ci.start) || !kso_get_ci((kso)self->of->end, &self->_ci.end) || !kso_get_ci((kso)self->of->step, &self->_ci.step)) {
        kso_catch_ignore();
    } else {
        /* We can use C-style integers */
        self->use_ci = true;
        self->_ci.cur = self->_ci.start;
    }

    return KSO_NONE;
}

/* actual next is in 'kso_next()', for optimizations */

/* Export */

static struct ks_type_s tp;
ks_type kst_range = &tp;

static struct ks_type_s tpi;
ks_type kst_range_iter = &tpi;


void _ksi_range() {
    _ksinit(kst_range_iter, kst_object, T_NAME, sizeof(struct ks_range_iter_s), -1, "", KS_IKV(
        {"__free",                 ksf_wrap(TI_free_, TI_NAME ".__free(self)", "")},
        {"__init",                 ksf_wrap(TI_init_, TI_NAME ".__init(self, of)", "")},
       // {"__next",                 ksf_wrap(TI_next_, TI_NAME ".__next(self)", "")},
    ));
    _ksinit(kst_range, kst_object, T_NAME, sizeof(struct ks_range_s), -1, "Range of integral values, with an optional step between", KS_IKV(
        {"__free",                 ksf_wrap(T_free_, T_NAME ".__free(self)", "")},
        {"__new",                  ksf_wrap(T_new_, T_NAME ".__new(tp, *args)", "")},
        {"__repr",                 ksf_wrap(T_str_, T_NAME ".__repr(self)", "")},
        {"__str",                  ksf_wrap(T_str_, T_NAME ".__str(self)", "")},

        {"__len",                  ksf_wrap(T_len_, T_NAME ".__len(self)", "")},
        {"__contains",             ksf_wrap(T_contains_, T_NAME ".__contains(self, obj)", "")},

        {"__iter",                 KS_NEWREF(kst_range_iter)},

    ));
}
