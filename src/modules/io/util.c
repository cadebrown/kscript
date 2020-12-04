/* io/util.c - 'io' module stream formatting utilities
 *
 */
#include <ks/impl.h>
#include <ks/io.h>

/* Format specifier */
struct sbfield {

    /* Width and precision (-1==default,-2=='*' argument) */
    int w, p;

    /* Flags */
    enum sbf {
        SBF_NONE = 0,

        /* contains '0' */
        SBF_0    = 0x01,

        /* ' ','+','-' */
        SBF_SPACE= 0x02,
        SBF_POS  = 0x04,
        SBF_NEG  = 0x08,

    } f;

};

#define SBFIELD_DEFAULT ((struct sbfield){ .w = -1, .p = -1, .f = SBF_NONE })


/* Digits used for base conversions */
static const char numeric_digits[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";

/* Base method to add a C-style integer to the output */
static bool add_int(ksio_AnyIO self, ks_cint val, int base, struct sbfield sbf) {

    char tmp[256];
    int i = 0;
    if (val < 0) {
        tmp[i++] = '-';
        val = -val;
    }

    int _si = i;

    do {
        /* Calculate Digit */
        int this_dig = val % base;
        if (this_dig < 0) this_dig = -this_dig;

        /* Extract Digit */
        val /= base;
        tmp[i++] = numeric_digits[this_dig];

    } while (val != 0);

    /* Reverse buffer */
    int j, k;
    for (j = _si, k = i-1; j < k; j++, k--) {
        char c = tmp[j];
        tmp[j] = tmp[k];
        tmp[k] = c;
    }

    /* Shift off */
    if (sbf.w > 0) {
        int s = sbf.w - i;
        int j;
        for (j = 0; j < s; ++j) {
            ksio_addbuf(self, 1, (sbf.f & SBF_0) ? "0" : " ");
        }
    }

    return ksio_addbuf(self, i, tmp);
}

/* Add unsigned integer */
static bool add_uint(ksio_AnyIO self, ks_uint val, int base, struct sbfield sbf) {
    char tmp[256];
    int i = 0;
    bool is_neg = val < 0;
    if (is_neg) tmp[i++] = '-';

    do {
        /* Calculate Digit */
        int this_dig = val % base;
        if (this_dig < 0) this_dig = -this_dig;

        /* Extract Digit */
        val /= base;
        tmp[i++] = numeric_digits[this_dig];

    } while (val != 0);

    /* Reverse buffer */
    int j, k;
    for (j = is_neg?1:0, k = i-1; j < k; j++, k--) {
        char c = tmp[j];
        tmp[j] = tmp[k];
        tmp[k] = c;
    }

    /* Shift off */
    if (sbf.w > 0) {
        int s = sbf.w - i;
        int j;
        for (j = 0; j < s; ++j) {
            ksio_addbuf(self, 1, (sbf.f & SBF_0) ? "0" : " ");
        }
    }

    return ksio_addbuf(self, i, tmp);
}


/* Add floating point value */
static bool add_float(ksio_AnyIO self, ks_cfloat val, int base, struct sbfield sbf) {
    char tmp[256];
    int i = 0;
    int req_sz = ks_cfloat_to_str(tmp, sizeof(tmp)-2, val, false, 6, base);

    /* TODO: reallocate */
    assert (req_sz < 256);

    ksio_addbuf(self, req_sz, tmp);

    return true;
}


/* Add generic object */
static bool add_O(ksio_AnyIO self, kso obj) {
    return 
        ksio_addbuf(self, 2, "<'") &&
        ksio_addbuf(self, obj->type->i__fullname->len_b, obj->type->i__fullname->data) &&
        ksio_addbuf(self, 6, "' @ 0x") &&
        add_uint(self, (uintptr_t)obj, 16, SBFIELD_DEFAULT) &&
        ksio_addbuf(self, 1, ">");
}

/* Add 'str(obj)' */
static bool add_str(ksio_AnyIO self, kso obj) {

    if (kso_issub(obj->type, kst_str)) {
        ks_str sobj = (ks_str)obj;
        return ksio_addbuf(self, sobj->len_b, sobj->data);
    } else if (obj == KSO_NONE) {
        return ksio_addbuf(self, 4, "none");
    } else if (obj->type == kst_bool) {
        return ksio_add(self, obj == KSO_TRUE ? "true" : "false");
    } else if (obj->type->i__str != kst_object->i__str) {
        ks_str sobj = (ks_str)kso_call(obj->type->i__str, 1, &obj);
        if (!sobj) return false;
        if (!kso_issub(sobj->type, kst_str)) {
            //KS_THROW_EXC(kst_Error, "'__str' method returned non-str");
            KS_DECREF(sobj);
            return false;
        }

        bool res = add_str(self, (kso)sobj);
        KS_DECREF(sobj);

        return res;
    }



    /* not found, so do generically */
    return add_O(self, obj);
}

/* Add 'repr(obj)' */
static bool add_repr(ksio_AnyIO self, kso obj) {

    if (kso_issub(obj->type, kst_str)) {
        ks_str sobj = (ks_str)obj;

        /* TODO: escape unicode? */

        ksio_addbuf(self, 1, "'");

        ks_cint i;
        for (i = 0; i < sobj->len_b; ++i) {
            char c = sobj->data[i];

            /**/ if (c == '\\') ksio_addbuf(self, 2, "\\\\");
            else if (c == '\n') ksio_addbuf(self, 2, "\\n");
            else if (c == '\r') ksio_addbuf(self, 2, "\\r");
            else if (c == '\t') ksio_addbuf(self, 2, "\\t");
            else if (c == '\a') ksio_addbuf(self, 2, "\\a");
            else if (c == '\b') ksio_addbuf(self, 2, "\\b");
            else if (c == '\f') ksio_addbuf(self, 2, "\\f");
            else if (c == '\v') ksio_addbuf(self, 2, "\\v");
            else if (c == '\'') ksio_addbuf(self, 2, "\\'");
            else {
                ksio_addbuf(self, 1, &c);
            }
        }

        return ksio_addbuf(self, 1, "'");
    } else if (obj == KSO_NONE) {
        return ksio_addbuf(self, 4, "none");
    } else if (obj->type == kst_bool) {
        return ksio_add(self, obj == KSO_TRUE ? "true" : "false");

    } else if (obj->type->i__repr != kst_object->i__repr) {
        ks_str sobj = (ks_str)kso_call(obj->type->i__repr, 1, &obj);
        if (!sobj) return false;
        if (!kso_issub(sobj->type, kst_str)) {
            //KS_THROW_EXC(kst_Error, "'__repr' method returned non-str");
            KS_DECREF(sobj);
            return false;
        }

        bool res = add_str(self, (kso)sobj);
        KS_DECREF(sobj);

        return res;
    }

    /* not found, so do generically */
    return add_O(self, obj);
}

bool ksio_addbuf(ksio_AnyIO self, ks_ssize_t sz, const char* data) {
    if (kso_issub(self->type, ksiot_FileIO)) {
        ksio_FileIO fio = (ksio_FileIO)self;
        return ksio_FileIO_writes(fio, sz, data);
    } else if (kso_issub(self->type, ksiot_StringIO)) {
        ksio_StringIO sio = (ksio_StringIO)self;
        ks_ssize_t pos = sio->len_b;

        /* Determine number of characters */
        sio->len_b += sz;
        sio->len_c += ks_str_lenc(sz, data);

        sio->data = ks_zrealloc(sio->data, 1, 1 + sio->len_b);
        memcpy(sio->data + pos, data, sz);
        sio->data[sio->len_b] = '\0';

        return true;
    } else {
        return false;
    }
}



bool ksio_addv(ksio_AnyIO self, const char* fmt, va_list ap) {
    const char* ofmt = fmt;
    struct sbfield sbf;

    if (fmt) while (*fmt) {
        const char* cfmt = fmt;
        while (*fmt && *fmt != '%') fmt++;

        /* Add literal bytes*/
        if (!ksio_addbuf(self, (ks_ssize_t)(fmt - cfmt), cfmt)) return false;

        char c;
        int i = 0;

        if (*fmt == '%') {
            /* Parse format specifier*/
            fmt++;
            sbf.w = sbf.p = -1;
            sbf.f = SBF_NONE;

            while ((c = *fmt) == '+' || c == '-' || c == ' ' || c == '0') {
                if (c == '+' || c == ' ' || c == '-') {
                    if (sbf.f & (SBF_POS | SBF_NEG | SBF_SPACE)) {
                        fprintf(stderr, "Invalid C-style format string, given sign multiple times (fmt='%s')\n", ofmt);
                    }
                    sbf.f |= c == '+' ? SBF_POS : (c == '-' ? SBF_NEG : SBF_SPACE);
                } else if (c == '0') {
                    if (sbf.f & SBF_0) {
                        fprintf(stderr, "Invalid C-style format string, given '0' multiple times (fmt='%s')\n", ofmt);
                    }
                    sbf.f |= SBF_0;
                }
                fmt++;
            }

            /* <w> */
            if (*fmt == '*') {
                sbf.w = -2;
                fmt++;
            } else if ('0' <= (c = *fmt) && c <= '9') {
                sbf.w = 0;
                while ((c = *fmt) && ('0' <= c && c <= '9')) {
                    sbf.w = 10 * sbf.w + (c - '0');
                    fmt++;
                }
            }

            /* .<p> */
            if ((c = *fmt) == '.') {
                fmt++;
                if ((c = *fmt) == '*') {
                    sbf.p = -2;
                    fmt++;
                } else if ('0' <= (c = *fmt) && c <= '9') {
                    sbf.p = 0;
                    while ((c = *fmt) && ('0' <= c && c <= '9')) {
                        sbf.p = 10 * sbf.p + (c - '0');
                        fmt++;
                    }
                }
            }

            if ((c = *fmt++) == '%') {
                /* %% -> % */
                if (!ksio_addbuf(self, 1, "%")) return false;
            } else {
                /* Otherwise, check 'c' */
                //if (!add_c(self, fmt, c, sbf, &sap)) return false;
                kso obj = NULL;
                ks_cint vi = 0;
                ks_uint vu = 0;
                ks_cfloat vf = 0;
                char* ss = NULL;
                ks_ucp c_v = 0;
                char c_v_utf8[5];

                switch (c)
                {
                case 'i':
                    vi = va_arg(ap, int);
                    if (!add_int(self, vi, 10, sbf)) return false;
                    break;

                case 'l':
                    vi = va_arg(ap, ks_cint);
                    if (!add_int(self, vi, 10, sbf)) return false;
                    break;
                case 'f':
                    vf = va_arg(ap, ks_cfloat);
                    if (!add_float(self, vf, 10, sbf)) return false;
                    break;

                case 'p':
                    vu = (ks_uint)va_arg(ap, void*);
                    if (!ksio_addbuf(self, 2, "0x")) return false;
                    if (!add_uint(self, vu, 16, sbf)) return false;
                    break;
                case 'u':
                    vu = va_arg(ap, ks_uint);
                    if (!add_uint(self, vu, 10, sbf)) return false;
                    break;

                case 'O':
                    obj = va_arg(ap, kso);
                    if (!add_O(self, obj)) return false;
                    break;
                
                case 'T':
                    obj = va_arg(ap, kso);
                    ksio_addbuf(self, obj->type->i__fullname->len_b, obj->type->i__fullname->data);
                    break;

                case 'S':
                    obj = va_arg(ap, kso);
                    if (!add_str(self, obj)) return false;
                    break;
                        
                case 'R':
                    obj = va_arg(ap, kso);
                    if (!add_repr(self, obj)) return false;
                    break;
                        
                case 'c':
                    if (sbf.w == -2) sbf.w = va_arg(ap, int);
                    if (sbf.p == -2) sbf.p = va_arg(ap, int);
                    c_v = (ks_ucp)va_arg(ap, int);
                    int nc = 0;
                    nc = 1;
                    c_v_utf8[0] = c_v;
                    //KS_UNICH_TO_UTF8(c_v_utf8, nc, c_v);
                    //c_v_utf8[nc] = '\0';
                    
                    if (sbf.p < 0) sbf.p = 1;

                    for (i = 0; i < sbf.p; ++i) {
                        if (!ksio_addbuf(self, nc, c_v_utf8)) return false;
                    }

                    break;
                case 's':
                    if (sbf.w == -2) sbf.w = va_arg(ap, int);
                    if (sbf.p == -2) sbf.p = va_arg(ap, int);
                    ss = va_arg(ap, char*);
                    if (sbf.p < 0) sbf.p = strlen(ss);
                    if (!ksio_addbuf(self, sbf.p, ss)) return false;

                    break;

                default:
                    fprintf(stderr, "Internal error: given bad C-format 'c': %c\n", c);
                    assert(false);
                    return false;
                    break;
                }
            }
        }
    }

}

bool ksio_add(ksio_AnyIO self, const char* fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    bool res = ksio_addv(self, fmt, ap);
    va_end(ap);
    return res;
}

