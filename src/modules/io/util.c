/* io/util.c - 'io' module stream formatting utilities
 *
 * This file defines the standard IO of common types (their individual '__str' and
 *   '__repr' methods don't do anything, it's all done here)
 * 
 */
#include <ks/impl.h>
#include <ks/io.h>


/** Internals **/




/* Digits used for base conversions */
static const char numeric_digits[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";

/* Byte strings to be used for particiular values */
static const char* byte_strs[] = {
    "\\x00", "\\x01", "\\x02", "\\x03", "\\x04", "\\x05", "\\x06", "\\x07", "\\x08", "\\x09", "\\x0A", "\\x0B", "\\x0C", "\\x0D", "\\x0E", "\\x0F",
    "\\x10", "\\x11", "\\x12", "\\x13", "\\x14", "\\x15", "\\x16", "\\x17", "\\x18", "\\x19", "\\x1A", "\\x1B", "\\x1C", "\\x1D", "\\x1E", "\\x1F",
    "\\x20", "\\x21", "\\x22", "\\x23", "\\x24", "\\x25", "\\x26", "\\x27", "\\x28", "\\x29", "\\x2A", "\\x2B", "\\x2C", "\\x2D", "\\x2E", "\\x2F",
    "\\x30", "\\x31", "\\x32", "\\x33", "\\x34", "\\x35", "\\x36", "\\x37", "\\x38", "\\x39", "\\x3A", "\\x3B", "\\x3C", "\\x3D", "\\x3E", "\\x3F",
    "\\x40", "\\x41", "\\x42", "\\x43", "\\x44", "\\x45", "\\x46", "\\x47", "\\x48", "\\x49", "\\x4A", "\\x4B", "\\x4C", "\\x4D", "\\x4E", "\\x4F",
    "\\x50", "\\x51", "\\x52", "\\x53", "\\x54", "\\x55", "\\x56", "\\x57", "\\x58", "\\x59", "\\x5A", "\\x5B", "\\x5C", "\\x5D", "\\x5E", "\\x5F",
    "\\x60", "\\x61", "\\x62", "\\x63", "\\x64", "\\x65", "\\x66", "\\x67", "\\x68", "\\x69", "\\x6A", "\\x6B", "\\x6C", "\\x6D", "\\x6E", "\\x6F",
    "\\x70", "\\x71", "\\x72", "\\x73", "\\x74", "\\x75", "\\x76", "\\x77", "\\x78", "\\x79", "\\x7A", "\\x7B", "\\x7C", "\\x7D", "\\x7E", "\\x7F",
    "\\x80", "\\x81", "\\x82", "\\x83", "\\x84", "\\x85", "\\x86", "\\x87", "\\x88", "\\x89", "\\x8A", "\\x8B", "\\x8C", "\\x8D", "\\x8E", "\\x8F",
    "\\x90", "\\x91", "\\x92", "\\x93", "\\x94", "\\x95", "\\x96", "\\x97", "\\x98", "\\x99", "\\x9A", "\\x9B", "\\x9C", "\\x9D", "\\x9E", "\\x9F",
    "\\xA0", "\\xA1", "\\xA2", "\\xA3", "\\xA4", "\\xA5", "\\xA6", "\\xA7", "\\xA8", "\\xA9", "\\xAA", "\\xAB", "\\xAC", "\\xAD", "\\xAE", "\\xAF",
    "\\xB0", "\\xB1", "\\xB2", "\\xB3", "\\xB4", "\\xB5", "\\xB6", "\\xB7", "\\xB8", "\\xB9", "\\xBA", "\\xBB", "\\xBC", "\\xBD", "\\xBE", "\\xBF",
    "\\xC0", "\\xC1", "\\xC2", "\\xC3", "\\xC4", "\\xC5", "\\xC6", "\\xC7", "\\xC8", "\\xC9", "\\xCA", "\\xCB", "\\xCC", "\\xCD", "\\xCE", "\\xCF",
    "\\xD0", "\\xD1", "\\xD2", "\\xD3", "\\xD4", "\\xD5", "\\xD6", "\\xD7", "\\xD8", "\\xD9", "\\xDA", "\\xDB", "\\xDC", "\\xDD", "\\xDE", "\\xDF",
    "\\xE0", "\\xE1", "\\xE2", "\\xE3", "\\xE4", "\\xE5", "\\xE6", "\\xE7", "\\xE8", "\\xE9", "\\xEA", "\\xEB", "\\xEC", "\\xED", "\\xEE", "\\xEF",
    "\\xF0", "\\xF1", "\\xF2", "\\xF3", "\\xF4", "\\xF5", "\\xF6", "\\xF7", "\\xF8", "\\xF9", "\\xFA", "\\xFB", "\\xFC", "\\xFD", "\\xFE", "\\xFF",
};

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

/* Forward declarations */
static bool add_str(ksio_BaseIO self, kso obj);
static bool add_repr(ksio_BaseIO self, kso obj);


/* Base method to add a C-style integer to the output */
static bool add_int(ksio_BaseIO self, ks_cint val, int base, struct sbfield sbf) {

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
static bool add_uint(ksio_BaseIO self, ks_uint val, int base, struct sbfield sbf) {
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
static bool add_float(ksio_BaseIO self, ks_cfloat val, int base, struct sbfield sbf) {
    char tmp[256];
    int i = 0;
    if (sbf.p < 0) sbf.p = 11;
    int req_sz = ks_cfloat_to_str(tmp, sizeof(tmp)-2, val, false, sbf.p, base);

    /* TODO: reallocate */
    assert (req_sz < 256);

    ksio_addbuf(self, req_sz, tmp);

    return true;
}

/* Add generic object */
static bool add_O(ksio_BaseIO self, kso obj) {
    return 
        ksio_addbuf(self, 2, "<'") &&
        ksio_addbuf(self, obj->type->i__fullname->len_b, obj->type->i__fullname->data) &&
        ksio_addbuf(self, 6, "' @ 0x") &&
        add_uint(self, (uintptr_t)obj, 16, SBFIELD_DEFAULT) &&
        ksio_addbuf(self, 1, ">");
}


/** Builtin object types **/


/* Add integer type */
static bool add_O_int(ksio_BaseIO self, ks_int val) {
    int base = 10;
    ks_size_t mlb = 16 + mpz_sizeinbase(val->val, base);
    char* buf = ks_malloc(mlb);
    mpz_get_str(buf, base, val->val);

    /* Translate to upper case */
    int i = 0;
    while (buf[i]) {
        if ('a' <= buf[i] && buf[i] <= 'f') {
            buf[i] += 'A' - 'a';
        }
        i++;
    }

    bool res = ksio_addbuf(self, i, buf);
    ks_free(buf);

    return res;
}


/* Add integer type */

static bool add_O_list(ksio_BaseIO self, ks_list val) {
    if (!ksio_addbuf(self, 1, "[")) return false;

    if (kso_inrepr((kso)val)) {
        ksio_addbuf(self, 3, "...");
    } else {
        ks_size_t i;
        for (i = 0; i < val->len; ++i) {
            if (i > 0) ksio_addbuf(self, 2, ", ");
            if (!add_repr(self, val->elems[i])) return false;
        }
        kso_outrepr();
    }


    if (!ksio_addbuf(self, 1, "]")) return false;
    return true;
}

static bool add_O_tuple(ksio_BaseIO self, ks_tuple val) {
    if (!ksio_addbuf(self, 1, "(")) return false;

    if (kso_inrepr((kso)val)) {
        ksio_addbuf(self, 3, "...");
    } else {
        ks_size_t i;
        for (i = 0; i < val->len; ++i) {
            if (i > 0) ksio_addbuf(self, 2, ", ");
            if (!add_repr(self, val->elems[i])) return false;
        }
        if (val->len == 1) ksio_addbuf(self, 1, ",");
        kso_outrepr();
    }

    if (!ksio_addbuf(self, 1, ")")) return false;
    return true;
}

static bool add_O_set(ksio_BaseIO self, ks_set val) {
    if (!ksio_addbuf(self, 1, "{")) return false;

    if (kso_inrepr((kso)val)) {
        ksio_addbuf(self, 3, "...");
    } else {
        ks_size_t i, ct = 0;
        for (i = 0; i < val->len_ents; ++i) {
            if (val->ents[i].key) {
                if (ct > 0) ksio_addbuf(self, 2, ", ");
                if (!add_repr(self, val->ents[i].key)) return false;
                ct++;
            }
        }
        kso_outrepr();
    }

    if (!ksio_addbuf(self, 1, "}")) return false;
    return true;
}

static bool add_O_dict(ksio_BaseIO self, ks_dict val) {
    if (!ksio_addbuf(self, 1, "{")) return false;
    if (kso_inrepr((kso)val)) {
        ksio_addbuf(self, 3, "...");
    } else {
        ks_size_t i, ct = 0;
        for (i = 0; i < val->len_ents; ++i) {
            if (val->ents[i].key) {
                if (ct > 0) ksio_addbuf(self, 2, ", ");
                if (!add_repr(self, val->ents[i].key)) return false;
                ksio_addbuf(self, 2, ": ");
                if (!add_repr(self, val->ents[i].val)) return false;
                ct++;
            }
        }
        kso_outrepr();
    }

    if (!ksio_addbuf(self, 1, "}")) return false;
    return true;
}

static bool add_O_path(ksio_BaseIO self, ksos_path val) {
    if (val->str_ != NULL) {
        return add_str(self, (kso)val->str_);
    }

    /* Have stringIO so we are appending, then capture the string at the end to cache for later */
    ksio_StringIO sio = NULL;
    if (kso_issub(self->type, ksiot_StringIO)) {
        sio = (ksio_StringIO)self;
        KS_INCREF(sio);
    } else {
        sio = ksio_StringIO_new();
    }

    /* Where it began in the stream */
    ks_cint pos = sio->len_b;

    if (val->root == KSO_NONE && val->parts->len == 0) {
        if (!ksio_add((ksio_BaseIO)sio, ".")) {
            KS_DECREF(sio);
            return false;

        }
    } else {
        if (val->root != KSO_NONE) if (!ksio_add((ksio_BaseIO)sio, "%S", val->root)) {
            KS_DECREF(sio);
            return false;
        }

        ks_size_t i;
        for (i = 0; i < val->parts->len; ++i) {
            if (i > 0) ksio_add((ksio_BaseIO)sio, "%s", KS_PLATFORM_PATHSEP);
            if (!ksio_add((ksio_BaseIO)sio, "%S", val->parts->elems[i])) {
                KS_DECREF(sio);
                return false;
            }
        }
    }

    val->str_ = ks_str_new(sio->len_b - pos, sio->data);
    if ((ksio_BaseIO)sio != self) {
        ksio_addbuf(self, val->str_->len_b, val->str_->data);
    }
    KS_DECREF(sio);

    return true; //ksio_addbuf(self, val->str_->len_b, val->str_->data);
}

/* Add 'str(obj)' */
static bool add_str(ksio_BaseIO self, kso obj) {
    if (kso_issub(obj->type, kst_str) && obj->type->i__str == kst_str->i__str) {
        ks_str sobj = (ks_str)obj;
        return ksio_addbuf(self, sobj->len_b, sobj->data);
    } else if (kso_issub(obj->type, kst_bytes) && obj->type->i__str == kst_bytes->i__str) {
        return add_repr(self, obj);
    } else if (obj->type == kst_type) {
        return add_str(self, (kso)((ks_type)obj)->i__fullname);
    } else if (obj == KSO_NONE) {
        return ksio_addbuf(self, 4, "none");
    } else if (obj->type == kst_bool) {
        return ksio_add(self, obj == KSO_TRUE ? "true" : "false");
    } else if (kso_isinst(obj, kst_enum) && obj->type->i__str == kst_enum->i__str) {
        ks_enum v = (ks_enum)obj;
        if (!add_str(self, (kso)v->s_int.type->i__fullname)) return false;
        if (!ksio_addbuf(self, 1, ".")) return false;
        if (!add_str(self, (kso)v->name)) return false;
        return true;
    } else if (kso_isinst(obj, kst_int) && obj->type->i__str == kst_int->i__str) {
        return add_O_int(self, (ks_int)obj);
    } else if (kso_isinst(obj, kst_float) && obj->type->i__str == kst_float->i__str) {
        return add_float(self, ((ks_float)obj)->val, 10, SBFIELD_DEFAULT);
    } else if (kso_isinst(obj, kst_list) && obj->type->i__str == kst_list->i__str) {
        return add_O_list(self, (ks_list)obj);
    } else if (kso_isinst(obj, kst_tuple) && obj->type->i__str == kst_tuple->i__str) {
        return add_O_tuple(self, (ks_tuple)obj);
    } else if (kso_isinst(obj, kst_set) && obj->type->i__str == kst_set->i__str) {
        return add_O_set(self, (ks_set)obj);
    } else if (kso_isinst(obj, kst_dict) && obj->type->i__str == kst_dict->i__str) {
        return add_O_dict(self, (ks_dict)obj);

    } else if (kso_isinst(obj, ksost_path) && obj->type->i__str == ksost_path->i__str) {
        return add_O_path(self, (ksos_path)obj);
    
    } else if (obj->type->i__str != kst_object->i__str) {
        ks_str sobj = (ks_str)kso_call(obj->type->i__str, 1, &obj);
        if (!sobj) return false;
        if (!kso_issub(sobj->type, kst_str)) {
            KS_THROW(kst_Error, "'%T.__str' method returned non-str object of type '%T'", obj, sobj);
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
static bool add_repr(ksio_BaseIO self, kso obj) {
    if (kso_issub(obj->type, kst_str) && obj->type->i__str == kst_str->i__str) {
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
    } else if (kso_issub(obj->type, kst_bytes) && obj->type->i__str == kst_str->i__bytes) {
        ks_bytes bobj = (ks_bytes)obj;

        /* TODO: include ASCII by default? */

        ksio_addbuf(self, 2, "b'");

        ks_cint i;
        for (i = 0; i < bobj->len_b; ++i) {
            unsigned char c = bobj->data[i];
            ksio_addbuf(self, 4, byte_strs[c]);
        }

        return ksio_addbuf(self, 1, "'");
    } else if (obj->type == kst_type) {
        return add_str(self, (kso)((ks_type)obj)->i__fullname);
    } else if (obj == KSO_NONE) {
        return ksio_addbuf(self, 4, "none");
    } else if (obj->type == kst_bool) {
        return ksio_add(self, obj == KSO_TRUE ? "true" : "false");
    } else if (kso_isinst(obj, kst_enum) && obj->type->i__str == kst_enum->i__str) {
        ks_enum v = (ks_enum)obj;
        if (!add_str(self, (kso)v->s_int.type->i__fullname)) return false;
        if (!ksio_addbuf(self, 1, ".")) return false;
        if (!add_str(self, (kso)v->name)) return false;
        return true;
    } else if (kso_isinst(obj, kst_int) && obj->type->i__repr == kst_int->i__repr) {
        return add_O_int(self, (ks_int)obj);
    } else if (kso_isinst(obj, kst_float) && obj->type->i__str == kst_float->i__str) {
        return add_float(self, ((ks_float)obj)->val, 10, SBFIELD_DEFAULT);
    } else if (kso_isinst(obj, kst_list) && obj->type->i__str == kst_list->i__str) {
        return add_O_list(self, (ks_list)obj);
    } else if (kso_isinst(obj, kst_tuple) && obj->type->i__str == kst_tuple->i__str) {
        return add_O_tuple(self, (ks_tuple)obj);
    } else if (kso_isinst(obj, kst_set) && obj->type->i__str == kst_set->i__str) {
        return add_O_set(self, (ks_set)obj);
    } else if (kso_isinst(obj, kst_dict) && obj->type->i__str == kst_dict->i__str) {
        return add_O_dict(self, (ks_dict)obj);


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


bool ksio_fmtv(ksio_BaseIO self, const char* fmt, va_list ap) {
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
                kso* objs;
                ks_cint vi = 0;
                ks_uint vu = 0;
                ks_cfloat vf = 0;
                char* ss = NULL;
                ks_ucp c_v = 0;
                char c_v_utf8[5];
                int k;

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
                 
                case 'J':
                    ss = va_arg(ap, char*);
                    vi = va_arg(ap, int);
                    objs = va_arg(ap, kso*);
                    int sl = strlen(ss);

                    for (k = 0; k < vi; ++k) {
                        if (k > 0) if (!ksio_addbuf(self, sl, ss)) return false;
                        if (!add_str(self, objs[k])) return false;
                    }
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

    return true;
}

bool ksio_fmt(ksio_BaseIO self, const char* fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    bool res = ksio_fmtv(self, fmt, ap);
    va_end(ap);
    return res;
}

ks_str ksio_readall(ks_str fname) {
    FILE* fp = fopen(fname->data, "r");
    if (!fp) {
        KS_THROW(kst_IOError, "Failed to open %R: %s", fname, strerror(errno));
        return NULL;
    }
    ksio_FileIO fio = ksio_FileIO_wrap(ksiot_FileIO, fp, true, true, false, false, fname);

    /* Buffered read, and append */
    ks_ssize_t bsz = KSIO_BUFSIZ, rsz = 0, num_c;
    void* dest = NULL;
    while (true) {
        dest = ks_realloc(dest, rsz + bsz * 4);
        ks_ssize_t csz = ksio_reads((ksio_BaseIO)fio, bsz, ((char*)dest) + rsz, &num_c);
        if (csz < 0) {
            ks_free(dest);
            KS_DECREF(fio);
            return NULL;
        }
        rsz += csz;
        if (csz == 0) break;
    }

    /* Construct string and return it */
    ks_str res = ks_str_new(rsz, dest);
    ks_free(dest);
    KS_DECREF(fio);
    return res;
}

/* Adapted from: https://gist.github.com/jstaursky/84cf1ddf91716d31558d6f0b5afc3feb */
ks_ssize_t ksu_getline(char** lineptr, ks_ssize_t* n, FILE* fp) {
    char *buffer_tmp, *position;
    ks_ssize_t block, offset;
    int c;

    if (!fp || !lineptr || !n) {
        return -1;

    } else if (*lineptr == NULL || *n <= 1) {
        // Minimum length for strings is 2 bytes
        *n = 128;
        if (!(*lineptr = ks_malloc(*n))) {
            return -1;
        }
    }

    block = *n;
    position = *lineptr;

    // keep reading characters until newline is hit
    /* Read until newline (or EOF) */
    while ((c = fgetc(fp)) != EOF && (*position++ = c) != '\n') {
        offset = position - *lineptr;

        if( offset >= *n ) {
            buffer_tmp = ks_realloc(*lineptr, *n += block);
            
            /* Do not free. Return *lineptr. */
            if (!buffer_tmp) {
                return -1;
            }

            *lineptr = buffer_tmp;
            position = *lineptr + offset;
        }
    }
    /* Terminate */
    *position = '\0';
    return (position - *lineptr - 1);
}
