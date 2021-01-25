/* DateTime.c - implementation of the 'time.DateTime' type
 *
 * 
 * @author: Cade Brown <brown.cade@gmail.com>
 */

#include "ks/impl.h"
#include <ks/time.h>

#define T_NAME "time.DateTime"


/* Internals */


kstime_DateTime kstime_wrap(struct tm tc, ks_cint nano) {
    return kstime_wrapt(kstimet_DateTime, tc, nano);
}

kstime_DateTime kstime_wrapt(ks_type tp, struct tm tc, ks_cint nano) {
    kstime_DateTime res = KSO_NEW(kstime_DateTime, tp);

    res->year = 1900 + tc.tm_year;

    res->month = tc.tm_mon + 1;
    res->day = tc.tm_mday;

    res->hour = tc.tm_hour;
    res->min = tc.tm_min;
    res->sec = tc.tm_sec;
    res->nano = nano;

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
struct tm kstime_unwrap(kstime_DateTime ts) {
    struct tm res;

    res.tm_year = ts->year - 1900; /* relative to 1900 */

    res.tm_mon = ts->month - 1;
    res.tm_mday = ts->day; /* 1-based */

    res.tm_hour = ts->hour;
    res.tm_min = ts->min;
    res.tm_sec = ts->sec;

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

ks_cfloat kstime_DateTime_tse(kstime_DateTime self) {
    struct tm val = kstime_unwrap(self);

#ifdef KS_HAVE_mktime
    ks_cfloat tse_c = mktime(&val);

#ifdef KS_HAVE_struct_tm_tm_gmtoff
    tse_c += val.tm_gmtoff - self->off_gmt;
#else
    tse_c -= self->off_gmt;
#endif
    return tse_c + (ks_cfloat)self->nano / KSTIME_NANO_PER_SEC;

#else
    ks_warn("time", "Attempted to convert datetime into time-since-epoch, but had no 'mktime()' function");
    return -1;
#endif

}


/* C-API */

kstime_DateTime kstime_new_utc(ks_cfloat tse) {
#if defined(WIN32)
	time_t tse_c = (time_t)tse;
	return kstime_wrap(*gmtime(&tse_c), kstime_extrananos(tse));
#elif defined(KS_HAVE_gmtime_r)
    time_t tse_c = (time_t)tse;
    struct tm val;
    gmtime_r(&tse_c, &val);
    return kstime_wrap(val, kstime_extrananos(tse));
#elif defined(KS_HAVE_gmtime)
    time_t tse_c = (time_t)tse;
    return kstime_wrap(*gmtime(&tse_c), kstime_extrananos(tse));
#else
    KS_THROW(kst_PlatformWarning, "Failed to create 'time.DateTime' in UTC, because the platform did not have 'gmtime()' function");
    return NULL;
    
#endif
}

kstime_DateTime kstime_new_local(ks_cfloat tse) {
#if defined(KS_HAVE_localtime_r)
    time_t tse_c = (time_t)tse;
    struct tm val;
    localtime_r(&tse_c, &val);
    return kstime_wrap(val, kstime_extrananos(tse));
#elif defined(KS_HAVE_localtime)
    time_t tse_c = (time_t)tse;
    return kstime_wrap(*localtime(&tse_c), kstime_extrananos(tse));
#else
    KS_THROW(kst_PlatformWarning, "Failed to create 'time.DateTime' in localtime, because the platform did not have 'localtime()' function");
    return NULL;
#endif
}


kstime_DateTime kstime_new_tz(ks_cfloat tse, kso tz) {
    if (tz == KSO_NONE) {
        return kstime_new_utc(tse);
    }

    if (!kso_issub(tz->type, kst_str)) {
        KS_THROW(kst_TypeError, "Expected timezone to be of type 'str' or 'none', but got '%T' object", tz);
        return NULL;
    }

    if (ks_str_eq_c((ks_str)tz, "local", 5) || ks_str_eq_c((ks_str)tz, "LOCAL", 5)) {
        return kstime_new_local(tse);
    } else if (ks_str_eq_c((ks_str)tz, "utc", 3) || ks_str_eq_c((ks_str)tz, "UTC", 3)) {
        return kstime_new_utc(tse);
    }

    /* First, capture the old timezone */
    ks_str TZs = ks_str_new(-1, "TZ");
    kso old_TZ = ksos_getenv(TZs, KSO_NONE);
    if (!old_TZ) {
        KS_DECREF(TZs);
        return NULL;
    }
    /* Set the timezone temporarily */
    if (!ksos_setenv(TZs, (ks_str)tz)) {
        KS_DECREF(TZs);
        KS_DECREF(old_TZ);
        return NULL;
    }

    /* Create new datetime in that timezone */
    kstime_DateTime res = kstime_new_local(tse);

    /* Restore the timezone */
    if (old_TZ == KSO_NONE) {
        if (!ksos_delenv(TZs)) {
            KS_NDECREF(res);
            KS_DECREF(old_TZ);
            return NULL;
        }
    } else {
        if (!ksos_setenv(TZs, (ks_str)old_TZ)) {
            KS_DECREF(TZs);
            KS_DECREF(old_TZ);
            KS_NDECREF(res);
            return NULL;
        }
    }

    KS_DECREF(TZs);
    KS_DECREF(old_TZ);
    if (!res) return NULL;

    return res;
}

/* Type Functions */

static KS_TFUNC(T, free) {
    kstime_DateTime self;
    KS_ARGS("self:*", &self, kstimet_DateTime);

    KS_NDECREF(self->zone);

    KSO_DEL(self);

    return KSO_NONE;
}

static KS_TFUNC(T, new) {
    ks_type tp;
    kso obj = KSO_NONE;
    kso tz = KSO_NONE;
    KS_ARGS("tp:* ?obj ?tz", &tp, kst_type, &obj, &tz);

    if (kso_isinst(obj, kstimet_DateTime)) {
        /* Convert from a date time */
        ks_cfloat tse = kstime_DateTime_tse((kstime_DateTime)obj);
        return (kso)kstime_new_tz(tse, tz);

    } else if (kso_is_float(obj) || kso_is_int(obj)) {
        /* Convert a time-since-epoch */
        ks_cfloat tse;
        if (!kso_get_cf(obj, &tse)) return NULL;
        return (kso)kstime_new_tz(tse, tz);
    } else if (obj == KSO_NONE) {
        return (kso)kstime_new_tz(kstime_time(), tz);
    }


    KS_THROW(kst_TypeError, "Failed to convert '%T' object to '%R': Expected either a 'time.DateTime' object, 'none', or a floating point number", obj, tp);
    return NULL;
}

static KS_TFUNC(T, str) {
    kstime_DateTime self;
    KS_ARGS("self:*", &self, kstimet_DateTime);

/*
    return (kso)ks_fmt("<%T year=%i, month=%i, day=%i, hour=%i, min=%i, sec=%i, nano=%i>", self, 
        (int)self->year, 
        (int)self->month, 
        (int)self->day, 
        (int)self->hour, 
        (int)self->min, 
        (int)self->sec,
        (int)self->nano
    );
*/
    ks_str fmt = kstime_format(KSTIME_FMT_ISO8601, self);
    if (!fmt) return NULL;

    ks_str res = ks_fmt("<%T %R>", self, fmt);
    KS_DECREF(fmt);
    return (kso)res;
}

static KS_TFUNC(T, getattr) {
    kstime_DateTime self;
    ks_str attr;
    KS_ARGS("self:* attr:*", &self, kstimet_DateTime, &attr, kst_str);

    if (ks_str_eq_c(attr, "tz", 4)) {
        return KS_NEWREF(self->zone ? (kso)self->zone : KSO_NONE);
    } else if (ks_str_eq_c(attr, "year", 4)) {
        return (kso)ks_int_new(self->year);
    } else if (ks_str_eq_c(attr, "month", 5)) {
        return (kso)ks_int_new(self->month);
    } else if (ks_str_eq_c(attr, "day", 3)) {
        return (kso)ks_int_new(self->day);
    } else if (ks_str_eq_c(attr, "hour", 4)) {
        return (kso)ks_int_new(self->hour);
    } else if (ks_str_eq_c(attr, "min", 3)) {
        return (kso)ks_int_new(self->min);
    } else if (ks_str_eq_c(attr, "sec", 3)) {
        return (kso)ks_int_new(self->sec);
    } else if (ks_str_eq_c(attr, "nano", 4)) {
        return (kso)ks_int_new(self->nano);
    } else if (ks_str_eq_c(attr, "isdst", 5)) {
        return KSO_BOOL(self->is_dst);
    } else if (ks_str_eq_c(attr, "tse", 3)) {
        return (kso)ks_float_new(kstime_DateTime_tse(self));
    }

    KS_THROW_ATTR(self, attr);
    return NULL;
}
static KS_TFUNC(T, float) {
    kstime_DateTime self;
    KS_ARGS("self:*", &self, kstimet_DateTime);

    return (kso)ks_float_new(kstime_DateTime_tse(self));
}




/* Export */

static struct ks_type_s tp;
ks_type kstimet_DateTime = &tp;

void _ksi_time_DateTime() {
    _ksinit(kstimet_DateTime, kst_number, T_NAME, sizeof(struct kstime_DateTime_s), -1, "Structure describing a point in time", KS_IKV(
        {"__free",               ksf_wrap(T_free_, T_NAME ".__free(self)", "")},
        {"__new",                ksf_wrap(T_new_, T_NAME ".__new(tp, obj=none, tz=none)", "")},
        {"__repr",               ksf_wrap(T_str_, T_NAME ".__repr(self)", "")},
        {"__str",                ksf_wrap(T_str_, T_NAME ".__str(self)", "")},
        {"__float",              ksf_wrap(T_float_, T_NAME ".__float(self)", "")},
        {"__getattr",            ksf_wrap(T_getattr_, T_NAME ".__getattr(self, attr)", "")},

    ));

}

