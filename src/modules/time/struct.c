/* struct.c - implementation of the 'time.struct' type
 *
 * 
 * @author: Cade Brown <brown.cade@gmail.com>
 */

#include "ks/impl.h"
#include <ks/time.h>

#define T_NAME "time.struct"


/* Internals */


kstime_struct kstime_wrap(struct tm tc) {
    kstime_struct res = KSO_NEW(kstime_struct, kstimet_struct);

    res->year = 1900 + tc.tm_year;

    res->month = tc.tm_mon;

    res->hour = tc.tm_hour;
    res->min = tc.tm_min;
    res->sec = tc.tm_sec;

    res->day = tc.tm_mday - 1;

    res->w_day = tc.tm_wday;
    res->y_day = tc.tm_yday;

    res->is_dst = tc.tm_isdst; /* TODO: check for -1 */

    /* technically plaform specific */
    #ifdef KS_HAVE_struct_tm_tm_gmtoff
    res->off_gmt = tc.tm_gmtoff;
    #else
    res->off_gmt = 0;
    #endif

    #ifdef KS_HAVE_struct_tm_tm_zone
    res->zone = tc.tm_zone ? ks_str_new(-1, tc.tm_zone) : NULL;
    #else
    res->zone = NULL;
    #endif

    return res;
}

/* Convert object to C-style structure */
struct tm kstime_unwrap(kstime_struct ts) {
    struct tm res;

    res.tm_year = ts->year - 1900; /* relative to 1900 */

    res.tm_mon = ts->month;

    res.tm_hour = ts->hour;
    res.tm_min = ts->min;
    res.tm_sec = ts->sec;

    res.tm_mday = ts->day + 1; /* 1-based */

    res.tm_wday = ts->w_day;
    res.tm_yday = ts->y_day;

    res.tm_isdst = ts->is_dst;

    /* technically plaform specific */
    #ifdef KS_HAVE_struct_tm_tm_gmtoff
    res.tm_gmtoff = ts->off_gmt;
    #endif

    #ifdef KS_HAVE_struct_tm_tm_zone
    res.tm_zone = ts->zone ? ts->zone->data : NULL;
    #endif

    return res;
}


/* C-API */

kstime_struct kstime_struct_new_utc(ks_cfloat tse) {
    #ifdef KS_HAVE_gmtime

    time_t _c = (time_t)tse;
    return kstime_wrap(*gmtime(&_c));
    
    #else
    
    KS_THROW(kst_PlatformWarning, "Failed to create 'time.struct' in UTC, because the platform did not have 'gmtime()' function");
    return NULL;
    
    #endif
}

kstime_struct kstime_struct_new_local(ks_cfloat tse) {
    #ifdef KS_HAVE_localtime
    
    time_t _c = (time_t)tse;
    return kstime_wrap(*localtime(&_c));

    #else
    
    KS_THROW(kst_PlatformWarning, "Failed to create 'time.struct' in localtime, because the platform did not have 'localtime()' function");
    return NULL;
    
    #endif
}


/* Type Functions */


static KS_TFUNC(T, free) {
    kstime_struct self;
    KS_ARGS("self:*", &self, kstimet_struct);

    KS_NDECREF(self->zone);

    KSO_DEL(self);

    return KSO_NONE;
}
static KS_TFUNC(T, str) {
    kstime_struct self;
    KS_ARGS("self:*", &self, kstimet_struct);

    ks_str fmt = kstime_fmt("%Y-%m-%dT%H:%M:%SZ%z", self);
    if (!fmt) return NULL;

    ks_str res = ks_fmt("<%T %R>", self, fmt);
    KS_DECREF(fmt);
    return (kso)res;
}
static KS_TFUNC(T, new) {
    ks_type tp;
    kso obj = KSO_NONE;
    KS_ARGS("tp:* ?obj", &tp, kst_type, &obj);


    KS_THROW(kst_Error, "Failed to convert '%T' object to time structure; use 'time.struct.local()' or 'time.struct.utc()'", obj);
    return NULL;
}
static KS_TFUNC(T, getattr) {
    kstime_struct self;
    ks_str attr;
    KS_ARGS("self:* attr:*", &self, kstimet_struct, &attr, kst_str);

    if (ks_str_eq_c(attr, "zone", 4)) {
        return KS_NEWREF(self->zone);
    }

    KS_THROW_ATTR(self, attr);
    return NULL;
}


static KS_TFUNC(T, local) {
    kso obj = KSO_NONE;
    KS_ARGS("?obj", &obj);

    if (kso_isinst(obj, kstimet_struct)) {
        /* convert struct to localtime */
    } else if (kso_is_float(obj) || kso_is_int(obj)) {
        ks_cfloat tse;
        if (!kso_get_cf(obj, &tse)) {
            return NULL;
        }
        return (kso)kstime_struct_new_local(tse);
    } else if (obj == KSO_NONE) {
        return (kso)kstime_struct_new_local(kstime_time());
    }

    KS_THROW(kst_Error, "Failed to convert '%T' object to local time", obj);
    return NULL;
}

static KS_TFUNC(T, utc) {
    kso obj = KSO_NONE;
    KS_ARGS("?obj", &obj);

    if (kso_isinst(obj, kstimet_struct)) {
        /* convert struct to localtime */
    } else if (kso_is_float(obj) || kso_is_int(obj)) {
        ks_cfloat tse;
        if (!kso_get_cf(obj, &tse)) {
            return NULL;
        }
        return (kso)kstime_struct_new_utc(tse);
    } else if (obj == KSO_NONE) {
        return (kso)kstime_struct_new_utc(kstime_time());
    }

    KS_THROW(kst_Error, "Failed to convert '%T' object to utc time", obj);
    return NULL;
}


/* Export */

static struct ks_type_s tp;
ks_type kstimet_struct = &tp;

void _ksi_time_struct() {
    _ksinit(kstimet_struct, kst_object, T_NAME, sizeof(struct kstime_struct_s), -1, "Structure describing a point in time", KS_IKV(
        {"__free",               ksf_wrap(T_free_, T_NAME ".__free(self)", "")},
        {"__new",                ksf_wrap(T_new_, T_NAME ".__new(tp, obj=none)", "")},
        {"__repr",               ksf_wrap(T_str_, T_NAME ".__repr(self)", "")},
        {"__str",                ksf_wrap(T_str_, T_NAME ".__str(self)", "")},
        {"__getattr",            ksf_wrap(T_getattr_, T_NAME ".__getattr(self, attr)", "")},

        {"local",                ksf_wrap(T_local_, T_NAME ".local(obj=none)", "Create a new structure describing a time in local time")},
        {"utc",                  ksf_wrap(T_utc_, T_NAME ".local(obj=none)", "Create a new structure describing a time in UTC time")},

    ));

}

