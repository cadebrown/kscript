/* types/slice.c - 'slice' type
 *
 * @author: Cade Brown <cade@kscript.org>
 */
#include <ks/impl.h>

#define T_NAME "slice"


/* C-API */

ks_slice ks_slice_new(ks_type tp, kso start, kso end, kso step) {
    ks_slice self = KSO_NEW(ks_slice, tp);

    KS_INCREF(start);
    self->start = start;
    KS_INCREF(end);
    self->end = end;
    KS_INCREF(step);
    self->step = step;

    return self;
}

bool ks_slice_get_citer_c(ks_cint start, ks_cint end, ks_cint step, ks_cint len, ks_cint* first, ks_cint* last, ks_cint* delta) {
    if (step == 0) {
        KS_THROW(kst_ValError, "Slices cannot have step==0");
        return false;
    }

    if (start < 0) start += len;
    if (start < 0) start = 0;
    else if (start > len) start = len;

    if (end < 0) end += len;
    if (end < 0) end = 0;
    else if (end > len) end = len;

    /* Check for quick exits */
    if ((step < 0 && start <= step) || (step > 0 && start >= end)) {
        *first = *last = 0;
        *delta = 1;
        return true;
    }

    /* Elements between them */
    *first = start;
    *last = end;

    if (step >= len || step <= -len) {
        /* 1 iteration */
        *delta = 1;
        *last = *first + *delta; 

        return true;
    } else if ((step > 0 && *last < *first) || (step < 0 && *last > *first)) {
        *delta = 1;
        *last = *first;

    } else {
        *delta = step;
    }

    ks_cint diff = *last - *first;

    if (diff % *delta != 0) {
        /* Nudge index */
        if (*delta > 0) *last = *first + *delta * (diff / *delta + 1);
        else  *last = *first + -*delta * (diff / -*delta + 1);
    }

    return true;
}

bool ks_slice_get_citer(ks_slice self, ks_cint len, ks_cint* first, ks_cint* last, ks_cint* delta) {

    /* Translate to integers */
    ks_cint start, end, step;

    if (self->step == KSO_NONE) {
        step = 1;
    } else {
        if (!kso_get_ci(self->step, &step)) return false;
    }

    if (self->start == KSO_NONE) {
        start = 0;
    } else {
        if (!kso_get_ci(self->start, &start)) return false;
    }

    if (self->end == KSO_NONE) {
        end = len;
    } else {
        if (!kso_get_ci(self->end, &end)) return false;
    }

    if (step == 0) {
        KS_THROW(kst_ValError, "Slices cannot have step==0");
        return false;
    }

    // bounds check them
    if (start < 0) start += len;
    if (start < 0) start = 0;
    else if (start > len) start = len;

    if (end < 0) end += len;
    if (end < 0) end = 0;
    else if (end > len) end = len;

    // special cases
    if (step < 0 && self->start == KSO_NONE) {
        start = len-1;
    }

    if (step < 0 && self->end == KSO_NONE) {
        end = -1;
    }

    *first = start;
    *last = end;
    *delta = step;


    ks_cint diff = *last - *first;

    if (diff % *delta != 0) {
        /* Nudge the index so 'i != last' works in iteration */
        *last = *first + *delta * (diff / *delta + 1);
    }

    return true;
}

/* Type Functions */

static KS_TFUNC(T, new) {
    ks_type tp;
    kso start, end, step = KSO_NONE;
    KS_ARGS("tp:* start end >step", &tp, kst_type, &start, &end, &step);

    return (kso)ks_slice_new(tp, start, end, step);
}

static KS_TFUNC(T, free) {
    ks_slice self;
    KS_ARGS("self:*", &self, kst_slice);

    KS_DECREF(self->start);
    KS_DECREF(self->end);
    KS_DECREF(self->step);

    KSO_DEL(self);

    return KSO_NONE;
}

static KS_TFUNC(T, str) {
    ks_slice self;
    KS_ARGS("self:*", &self, kst_slice);

    return (kso)ks_fmt("%T(%R, %R, %R)", self, self->start, self->end, self->step);
}

/* Export */

static struct ks_type_s tp;
ks_type kst_slice = &tp;


void _ksi_slice() {
    _ksinit(kst_slice, kst_object, T_NAME, sizeof(struct ks_slice_s), -1, "Slice indexer", KS_IKV(
        {"__free",                 ksf_wrap(T_free_, T_NAME ".__free(self)", "")},
        {"__new",                  ksf_wrap(T_new_, T_NAME ".__new(tp, start, end, step=none)", "")},
        {"__repr",                 ksf_wrap(T_str_, T_NAME ".__repr(self)", "")},
        {"__str",                  ksf_wrap(T_str_, T_NAME ".__str(self)", "")},

    ));
}
