/* main.c - implementation of the 'time' module
 * 
 * @author:    Cade Brown <cade@kscript.org>
 */
#include <ks/impl.h>
#include <ks/time.h>

#define M_NAME "time"


/* C-API */

ks_cint kstime_extrananos(ks_cfloat tse) {
    return fmod(tse, 1) * KSTIME_NANO_PER_SEC;
}


ks_cfloat kstime_time() {
#ifdef KS_HAVE_clock_gettime
    struct timespec tp;

  #ifdef KS_HAVE_CLOCK_REALTIME
      clock_gettime(CLOCK_REALTIME, &tp);
  #elif defined(KS_HAVE_CLOCK_REALTIME_COARSE)
      clock_gettime(CLOCK_REALTIME_COARSE, &tp);
  #else
      ks_warn("time", "time() requested, but no 'CLOCK_REALTIME' or 'CLOCK_REALTIME_COARSE' was found in the C library");
      return -1.0;
  #endif
    return tp.tv_sec + (ks_cfloat)tp.tv_nsec / KSTIME_NANO_PER_SEC;
#elif defined(KS_HAVE_time)
    return (ks_cfloat)time(NULL);
#else
    ks_warn("time", "time() requested, but no 'clock_gettime()' or 'time()' function were found on this system");
    return -1.0;
#endif
}

ks_cfloat kstime_clock() {
#ifdef KS_HAVE_clock
    clock_t cres = clock();
    return (ks_cfloat)cres / CLOCKS_PER_SEC;
#else
    ks_warn("time", "clock() requested, but no 'clock()' function was found on this system");
    return -1.0;
#endif
}

void kstime_sleep(ks_cfloat dur) {
#if defined(KS_HAVE_nanosleep)
    struct timespec req = {}, rem = {};
    req.tv_sec = (int)dur;
    req.tv_nsec = kstime_extrananos(dur - req.tv_sec);
    nanosleep(&req, &rem);
#elif defined(KS_HAVE_usleep)
    usleep(dur * 1e6);
#elif defined(KS_HAVE_sleep)
    sleep(dur);
#else
    ks_warn("time", "sleep() requested, but no 'nanosleep()', 'usleep()', or 'sleep()' functions were found");
#endif
}

ks_str kstime_asc(kstime_DateTime ts) {
    /* SEE: https://pubs.opengroup.org/onlinepubs/009695399/functions/asctime.html */
    static char wday_name[7][3] = {
        "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"
    };
    static char mon_name[12][3] = {
        "Jan", "Feb", "Mar", "Apr", "May", "Jun",
        "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
    };
    char result[256];

    snprintf(result, sizeof(result) - 1, "%.3s %.3s%3d %.2d:%.2d:%.2d %d",
        wday_name[ts->w_day],
        mon_name[ts->month], (int)ts->day + 1, 
        (int)ts->hour, (int)ts->min, (int)ts->sec,
        (int)ts->year
    );

    return ks_str_new(-1, result);
}

ks_str kstime_format(const char* fmt, kstime_DateTime ts) {
    /* TODO: implement this from scratch */

#ifdef KS_HAVE_strftime
    /* Try with a fixed size quick buffer */
    char qb[256];
    struct tm ctm = kstime_unwrap(ts);
    size_t sz = strftime(qb, sizeof(qb) - 1, fmt, &ctm);
    if (sz == 0) {
        /* Failed to fit, so dynamically allocate */
        ks_ssize_t len_buf = 512;
        char* buf = NULL;
        do {
            len_buf = ks_nextsize(len_buf, 2 * len_buf);
            buf = ks_realloc(buf, len_buf + 1);

            sz = strftime(buf, len_buf, fmt, &ctm);

        } while (sz == 0);

        ks_str res = ks_str_new(-1, buf);
        ks_free(buf);
        return res;

    } else {
        return ks_str_new(-1, qb);
    }
#else
    KS_THROW(kst_PlatformWarning, "Failed to format time, the platform did not have 'strftime()' function");
    return NULL;
#endif
}

kstime_DateTime kstime_parse(const char* fmt, const char* str) {
#ifdef KS_HAVE_strptime
    struct tm ctm = {};
    char* endp = strptime(str, fmt, &ctm);
    if (!endp) {
        KS_THROW(kst_Error, "Failed to parse time string '%s' as format '%s'", str, fmt);
        return NULL;
    }

    return kstime_wrap(ctm, 0);
#else
    KS_THROW(kst_PlatformWarning, "Failed to parse time, the platform did not have 'strptime()' function");
    return NULL;
#endif
}


/* Module Functions */

static KS_TFUNC(M, now) {
    KS_ARGS("");

    return (kso)kstime_new_utc(kstime_time());
}

static KS_TFUNC(M, localnow) {
    KS_ARGS("");

    return (kso)kstime_new_local(kstime_time());
}

static KS_TFUNC(M, time) {
    KS_ARGS("");

    return (kso)ks_float_new(kstime_time());
}

static KS_TFUNC(M, clock) {
    KS_ARGS("");

    return (kso)ks_float_new(kstime_clock());
}
static KS_TFUNC(M, sleep) {
    ks_cfloat dur;
    KS_ARGS("dur:cfloat", &dur);

    kstime_sleep(dur);

    return KSO_NONE;
}

static KS_TFUNC(M, format) {
    kso t = KSO_NONE;
    ks_str fstr = NULL;
    KS_ARGS("?val ?fmt:*", &t, &fstr, kst_str);

    if (t == KSO_NONE || kso_is_num(t)) {
        ks_cfloat tse = 0;
        if (t == KSO_NONE) {
            tse = kstime_time();
        } else {
            if (!kso_get_cf(t, &tse)) return NULL;
        }

        kstime_DateTime ts = kstime_new_utc(tse);
        if (!ts) return NULL;
        ks_str res = kstime_format(fstr?fstr->data:KSTIME_FMT_ISO8601, ts);
        KS_DECREF(ts);
        return (kso)res;
    } else if (kso_issub(t->type, kstimet_DateTime)) {
        return (kso)kstime_format(fstr?fstr->data:KSTIME_FMT_ISO8601, (kstime_DateTime)t);
    }

    KS_THROW(kst_ArgError, "Invalid type for 't', expected either numerical date, or a 'time.struct' object");
    return NULL;
}

static KS_TFUNC(M, parse) {
    ks_str val, fmt = NULL;
    KS_ARGS("val:* ?fmt:*", &val, kst_str, &fmt, kst_str);

    return (kso)kstime_parse(fmt?fmt->data:KSTIME_FMT_ISO8601, val->data);
}

/* Export */

ks_module _ksi_time() {

    _ksi_time_DateTime();

    ks_module res = ks_module_new(M_NAME, KS_BIMOD_SRC, "'time' - implementation of time, datetime, and related functionalities\n\n    This module defines time types and operations", KS_IKV(
        /* Constants */

        {"ISO8601",                (kso)ks_str_new(-1, KSTIME_FMT_ISO8601)},

        /* Types */

        {"DateTime",               KS_NEWREF(kstimet_DateTime)},

        /* Functions */

        {"now",                    ksf_wrap(M_now_, M_NAME ".now()", "Get the current time as a 'time.DateTime' object, in UTC")},
        {"localnow",               ksf_wrap(M_localnow_, M_NAME ".localnow()", "Get the current time as a 'time.DateTime' object, in the local time zone")},
        {"time",                   ksf_wrap(M_time_, M_NAME ".time()", "Calculates the time (in seconds) since the epoch (normally 1970-01-01), not counting leap seconds\n\n    Sometimes referred to as 'Unix Time'")},
        {"clock",                  ksf_wrap(M_clock_, M_NAME ".clock()", "Calculates the time (in seconds) since the process start time")},
        {"sleep",                  ksf_wrap(M_sleep_, M_NAME ".sleep(dur)", "Sleep for a given duration (in seconds)")},

        {"format",                 ksf_wrap(M_format_, M_NAME ".format(val=none, fmt=time.ISO8601)", "Returns a time (default: now) formatted according to printf-like format specifiers\n\n   Specifiers:\n      %y: Year modulo 100 (00...99)\n      %Y: Year with century, in format '%-04i' (0001...9999)\n      %b: Month in locale (abbreviated)\n      %B: Month in locale (full)\n      %m: Month as zero-padded number (01...12)\n      %U: Week of the year as as a decimal number (Sunday==0)\n            (days before the first sunday are week 0)\n      %W: Week of the year as as a decimal number (Monday==0)\n            (days before the first monday are week 0)\n      %j: Yearday in '%03i' (001...366)\n      %d: Monthday as zero-padded number (01...31)\n      %a: Weekday in locale (abbreviated)\n      %A: Weekday in locale (full)\n      %w: Weekday as integer (0==Monday)\n      %H: Hour (in 24-hour clock) as zero-padded number (00...23)\n      %I: Hour (in 12 hour clock) as zero-padded number (00, 12)\n      %M: Minute as zero-padded number (00-59)\n      %S: Seconds as zero-padded decimal number (00-59)\n      %f: Microsecond as decimal number, formatted as '%-06i'\n      %z: Zone UTC offset in '(+|-)HHMM[SS.[ffffff]]'\n      %Z: Zone name (or empty if there was none)\n      %p: Locale's equiv of AM/PM\n      %c: Locale's full default date/time representation\n      %x: Locale's default date representation\n      %X: Locale's default time representation\n      %%: Literal '%'")},
        {"parse",                  ksf_wrap(M_parse_, M_NAME ".parse(val, fmt=time.ISO8601)", "Parses 'val' (which should be a string in the correct format) according to format string 'fmt'\n\n    See 'time.format()''s documentation for descriptions of the format string")},

    ));

    return res;
}
