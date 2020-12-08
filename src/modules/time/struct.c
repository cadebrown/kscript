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

static KS_TFUNC(T, str) {
    kstime_struct self;
    KS_ARGS("self:*", &self, kstimet_struct);

    ks_str fmt = kstime_fmt("%Y-%m-%dT%H:%M:%SZ%z", self);
    if (!fmt) return NULL;

    ks_str res = ks_fmt("<%T %R>", self, fmt);
    KS_DECREF(fmt);
    return (kso)res;
}


/* Export */

static struct ks_type_s tp;
ks_type kstimet_struct = &tp;

void _ksi_time_struct() {
    _ksinit(kstimet_struct, kst_object, T_NAME, sizeof(struct kstime_struct_s), -1, "Structure describing a point in time", KS_IKV(
        {"__repr",               ksf_wrap(T_str_, T_NAME ".__repr(self)", "")},
        {"__str",                ksf_wrap(T_str_, T_NAME ".__str(self)", "")},

    ));

}

