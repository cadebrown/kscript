/* util.c - Utility functions
 *
 * 
 * @author: Cade Brown <cade@kscript.org>
 */
#include <ks/impl.h>

ks_str ks_fmt(const char* fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    ks_str res = ks_fmtv(fmt, ap);
    va_end(ap);
    return res;
}

ks_str ks_fmtv(const char* fmt, va_list ap) {
    ksio_StringIO sio = ksio_StringIO_new();

    if (!ksio_addv((ksio_AnyIO)sio, fmt, ap)) {
        KS_DECREF(sio);
        return NULL;
    }

    return ksio_StringIO_getf(sio);
}

