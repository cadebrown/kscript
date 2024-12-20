/* args.c - implementation of C-style function signature argument parsing
 *
 * @author: Cade Brown <cade@kscript.org>
 */
#include <ks/impl.h>

bool kso_parse(int nargs, kso* args, const char* fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    bool res = _ks_argsv(1, nargs, args, fmt, ap);
    va_end(ap);
    return res;
}

bool _ks_args(int nargs, kso* args, const char* fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    bool res = _ks_argsv(0, nargs, args, fmt, ap);
    va_end(ap);
    return res;
}

bool _ks_argsv(int kk, int nargs, kso* args, const char* fmt, va_list ap) {
    const char* o_fmt = fmt;

    bool res = true;

    /* Current argument index being consumed */
    int cai = 0;

    while (*fmt) {
        const char* cfmt = fmt;
        while (*fmt && *fmt == ' ') fmt++;

        /* '*name' -> absorb all remaining into this name  
         * '?name' -> optional argument
         */
        bool is_vararg = *fmt == '*', is_opt = *fmt == '?'; 
        if (is_vararg || is_opt) fmt++;

        /* Spaces are allowed between the entries */
        while (*fmt == ' ') fmt++;

        /* Parse argument name */
        const char* an = fmt;
        int an_c = 0;
        while (*fmt && *fmt != ':' && *fmt != ' ') {
            an_c++;
            fmt++;
        }

        if (is_vararg) {
            /* Store the rest in these two */

            int* to_nargs = va_arg(ap, int*);
            kso** to_args = va_arg(ap, kso**);

            *to_nargs = nargs - cai;
            *to_args = &args[cai];

            cai += *to_nargs;

            /* Done, we consumed the rest */
            break;

        } else {

            if (cai >= nargs) {
                if (is_opt) break;
                if (kk == 0) {
                    KS_THROW(kst_ArgError, "Missing arguments, only given %i", nargs);
                } else {
                    KS_THROW(kst_ArgError, "Missing values for value string '%s'", o_fmt);
                }
                res = false;
                break;
            }
            /* Consume one more argument */
            kso cargin = args[cai++];
            kso* cargto = va_arg(ap, kso*);

            if (*fmt == ':') {
                fmt++;

                if (*fmt == '*') {
                    fmt++;
                    ks_type req = va_arg(ap, ks_type);
                    assert(req->type == kst_type);

                    if (!kso_issub(cargin->type, req)) {
                        if (kk == 0) {
                            KS_THROW(kst_ArgError, "Expected argument '%.*s' to be of type %R, but was of type '%T'", an_c, an, req->i__fullname, cargin);
                        } else {
                            KS_THROW(kst_ArgError, "Expected value '%.*s' to be of type %R, but was of type '%T' (in value string '%s')", an_c, an, req->i__fullname, cargin, o_fmt);
                        }
                        res = false;
                        break;
                    }


                } else if (strncmp(fmt, "cint", 4) == 0) {
                    fmt += 4;

                    if (!kso_get_ci(cargin, (ks_cint*)cargto)) {
                        kso_catch_ignore();
                        if (kk == 0) {
                            KS_THROW(kst_Error, "Argument '%.*s' (of type '%T') could not be converted to a C-style int", an_c, an, cargin);
                        } else {
                            KS_THROW(kst_Error, "Value '%.*s' (of type '%T') could not be converted to a C-style int", an_c, an, cargin);
                        }
                        res = false;
                        break;
                    }
                    continue;
                } else if (strncmp(fmt, "cfloat", 6) == 0) {
                    fmt += 6;

                    if (!kso_get_cf(cargin, (ks_cfloat*)cargto)) {
                        kso_catch_ignore();
                        if (kk == 0) {
                            KS_THROW(kst_Error, "Argument '%.*s' (of type '%T') could not be converted to a C-style float", an_c, an, cargin);
                        } else {
                            KS_THROW(kst_Error, "Value '%.*s' (of type '%T') could not be converted to a C-style float", an_c, an, cargin);
                        }
                        res = false;
                        break;
                    }
                    continue;
                } else if (strncmp(fmt, "bool", 4) == 0) {
                    fmt += 4;

                    if (!kso_truthy(cargin, (bool*)cargto)) {
                        res = false;
                        break;
                    }
                    continue;

                } else {
                    assert(false && "'KS_ARGS'/similar was given a bad C-style format string");
                }
            }

            *cargto = cargin;
        }
    }

    if (res && cai != nargs) {
        KS_THROW(kst_ArgError, "Given extra arguments, only expected %i, but given %i", cai, nargs);
        res = false;
    }
    /* Success/Fail */
    return res;
}




