/* io/BaseIO.c - 'io.BaseIO' type
 *
 * This is an abstract base type for all IO-like objects defined in the standard library
 * 
 * It has method stubs (which throw an error if ran), or which call derived types
 * 
 * 
 * @author: Cade Brown <cade@kscript.org>
 */
#include <ks/impl.h>

#define T_NAME "io.BaseIO"
#define TI_NAME T_NAME ".__iter"

/* C-API */

bool ksio_is_open(ksio_BaseIO self, bool* out) {
    if (kso_issub(self->type, ksiot_FileIO)) {
        *out = ((ksio_FileIO)self)->is_open;
        return true;
    } else if (kso_issub(self->type, ksiot_StringIO) || kso_issub(self->type, ksiot_BytesIO)) {
        *out = true;
        return true;
    } else {
        ks_str key = ks_str_new(-1, "isopen");
        kso ff = kso_getattr(self, key);
        KS_DECREF(key);
        if (!ff) return NULL;

        kso res = kso_call(ff, 0, NULL);
        KS_DECREF(ff);
        if (!res) return NULL;

        if (!kso_truthy(res, out)) {
            KS_DECREF(res);
            return false;
        }
        KS_DECREF(res);
        return true;
    }

    KS_THROW(kst_TypeError, "Failed to determine whether '%T' object was open", self);
    return false;
}

bool ksio_is_r(ksio_BaseIO self, bool* out) {
    if (kso_issub(self->type, ksiot_FileIO)) {
        *out = ((ksio_FileIO)self)->is_r;
        return true;
    } else if (kso_issub(self->type, ksiot_StringIO) || kso_issub(self->type, ksiot_BaseIO)) {
        *out = true;
        return true;
    }

    KS_THROW(kst_TypeError, "Failed to determine whether '%T' object was readable", self);
    return false;
}

bool ksio_is_w(ksio_BaseIO self, bool* out) {
    if (kso_issub(self->type, ksiot_FileIO)) {
        *out = ((ksio_FileIO)self)->is_w;
        return true;
    } else if (kso_issub(self->type, ksiot_StringIO) || kso_issub(self->type, ksiot_BaseIO)) {
        *out = true;
        return true;
    }


    KS_THROW(kst_TypeError, "Failed to determine whether '%T' object was writeable", self);
    return false;
}

ks_type ksio_rtype(ksio_BaseIO self) {
    if (kso_issub(self->type, ksiot_FileIO)) {
        if (((ksio_FileIO)self)->is_bin) {
            KS_INCREF(kst_bytes);
            return kst_bytes;
        } else {
            KS_INCREF(kst_str);
            return kst_str;
        }
    } else if (kso_issub(self->type, ksiot_StringIO)) {
        KS_INCREF(kst_str);
        return kst_str;
    } else if (kso_issub(self->type, ksiot_BytesIO)) {
        KS_INCREF(kst_bytes);
        return kst_bytes;
    } else {
        ks_str key = ks_str_new(-1, "rtype");
        kso ff = kso_getattr(self, key);
        KS_DECREF(key);
        if (!ff) return NULL;

        ks_type res = (ks_type)kso_call(ff, 0, NULL);
        KS_DECREF(ff);
        if (!res) return NULL;

        if (!kso_issub(res->type, kst_type)) {
            KS_THROW(kst_TypeError, "'%T.rtype()' returned non-type object of type '%T'", res);
            KS_DECREF(res);
            return NULL;
        }

        return res;
    }
    KS_THROW(kst_TypeError, "Failed to determine the IO result type of '%T' object", self);
    return NULL;
}

bool ksio_is_bytes(ksio_BaseIO self, bool* out) {
    ks_type rt = ksio_rtype(self);
    if (!rt) return false;

    *out = kso_issub(rt, kst_bytes);
    KS_DECREF(rt);
    return true;
}

bool ksio_is_str(ksio_BaseIO self, bool* out) {
    ks_type rt = ksio_rtype(self);
    if (!rt) return false;

    *out = kso_issub(rt, kst_str);
    KS_DECREF(rt);
    return true;
}

ks_ssize_t ksio_readb(ksio_BaseIO self, ks_ssize_t sz_b, void* data) {
    bool good;
    if (!ksio_is_open(self, &good)) return -1;
    if (!good) {
        KS_THROW(kst_IOError, "'%T' object is not open", self);
        return -1;
    }
    if (!ksio_is_r(self, &good)) return -1;
    if (!good) {
        KS_THROW(kst_IOError, "'%T' object is not readable", self);
        return -1;
    }
    if (!ksio_is_bytes(self, &good)) return -1;
    if (!good) {
    //    KS_THROW(kst_IOError, "'%T' object is not a bytes-oriented IO", self);
    //    return -1;
    }

    /* Read string based on the type of IO */

    if (kso_issub(self->type, ksiot_FileIO)) {
        ksio_FileIO fio = (ksio_FileIO)self;

        /* Unlock GIL for multithreading */
        KS_GIL_UNLOCK();
        ks_ssize_t real_sz = fread(data, 1, sz_b, fio->fp);
        KS_GIL_LOCK();

        /* Update state variables */
        fio->sz_r += real_sz;

        return real_sz;
    } else if (kso_issub(self->type, ksiot_BytesIO)) {
        ksio_StringIO sio = (ksio_StringIO)self;

        return sz_b;
    } else {
        ks_str key = ks_str_new(-1, "read");
        kso rf = kso_getattr(self, key);
        KS_DECREF(key);
        if (!rf) return -1;

        ks_int obsz = ks_int_new(sz_b);
        ks_bytes bio = (ks_bytes)kso_call(rf, 1, (kso[]){ (kso)obsz });
        KS_DECREF(obsz);
        KS_DECREF(rf);

        if (!bio) {
            return -1;
        } else if (!kso_issub(bio->type, kst_bytes)) {
            KS_THROW(kst_TypeError, "'%T.read()' returned non-bytes object of type '%T'", bio);
            KS_DECREF(bio);
            return -1;
        }

        ks_ssize_t real_sz = bio->len_b;
        memcpy(data, bio->data, bio->len_b);
        KS_DECREF(bio);

        return real_sz;
    }

    KS_THROW(kst_IOError, "Don't know how to read bytes from '%T' object", self);
    return -1;
}

ks_ssize_t ksio_reads(ksio_BaseIO self, ks_ssize_t sz_c, void* data, ks_ssize_t* num_c) {
    bool good;
    if (!ksio_is_open(self, &good)) return -1;
    if (!good) {
        KS_THROW(kst_IOError, "'%T' object is not open", self);
        return -1;
    }
    if (!ksio_is_r(self, &good)) return -1;
    if (!good) {
        KS_THROW(kst_IOError, "'%T' object is not readable", self);
        return -1;
    }
    if (!ksio_is_str(self, &good)) return -1;
    if (!good) {
    //    KS_THROW(kst_IOError, "'%T' object is not a str-oriented IO", self);
    //    return -1;
    }

    if (kso_issub(self->type, ksiot_FileIO)) {
        ksio_FileIO fio = (ksio_FileIO)self;

        ks_ssize_t sz_b = 0;
        char utf8[5];
        *num_c = 0;
        bool keep_going = true;
        while (*num_c < sz_c && keep_going) {
            /* Read another character */

            /* Assume UTF8 
             * TODO: handle other encodings?
             */
            int i;
            for (i = 0; i < 4; ++i) {
                int c = fgetc(fio->fp);
                if (c == EOF) {
                    keep_going = false;
                    break;
                }

                fio->sz_r++;
                utf8[i] = c;

                /* Check if it is NOT a continuation byte */
                if ((c & 0x80) == 0x0) {
                    i++;
                    break;
                }
            }
            if (i == 0) break;
            utf8[i] = '\0';

            /* Now, add to destination */
            memcpy(((char*)data) + sz_b, utf8, i);
            sz_b += i;
            (*num_c)++;
        }

        return sz_b;
    } else if (kso_issub(self->type, ksiot_BytesIO) || kso_issub(self->type, ksiot_StringIO)) {
        ksio_StringIO sio = (ksio_StringIO)self;

        ks_ssize_t sz_b = 0;
        char utf8[5];
        *num_c = 0;
        bool keep_going = true;
        while (*num_c < sz_c && keep_going) {
            /* Read another character */

            /* Assume UTF8 
             * TODO: handle other encodings?
             */
            int i;
            for (i = 0; i < 4; ++i) {
                if (sio->pos_b >= sio->len_b) {
                    keep_going = false;
                    break;
                }
                int c = sio->data[sio->pos_b++];

                sio->sz_r++;
                utf8[i] = c;

                /* Check if it is NOT a continuation byte */
                if ((c & 0x80) == 0x0) {
                    i++;
                    break;
                }
            }
            if (i == 0) break;
            utf8[i] = '\0';

            /* Now, add to destination */
            memcpy(((char*)data) + sz_b, utf8, i);
            sz_b += i;
            (*num_c)++;
        }
        return sz_b;
    } else {
        ks_str key = ks_str_new(-1, "read");
        kso rf = kso_getattr(self, key);
        KS_DECREF(key);
        if (!rf) return -1;

        ks_int obsz = ks_int_new(sz_c);
        ks_str bio = (ks_str)kso_call(rf, 1, (kso[]){ (kso)obsz });
        KS_DECREF(obsz);
        KS_DECREF(rf);

        if (!bio) {
            return -1;
        } else if (!kso_issub(bio->type, kst_str)) {
            KS_THROW(kst_TypeError, "'%T.read()' returned non-str object of type '%T'", bio);
            KS_DECREF(bio);
            return -1;
        }

        ks_ssize_t real_sz = bio->len_b;
        memcpy(data, bio->data, bio->len_b);
        KS_DECREF(bio);

        (*num_c) = bio->len_c;
        return real_sz;
    }


    KS_THROW(kst_IOError, "Don't know how to read str from '%T' object", self);
    return -1;
}

bool ksio_writeb(ksio_BaseIO self, ks_ssize_t sz_b, const void* data) {
    bool good;
    if (!ksio_is_open(self, &good)) return -1;
    if (!good) {
        KS_THROW(kst_IOError, "'%T' object is not open", self);
        return false;
    }
    if (!ksio_is_w(self, &good)) return -1;
    if (!good) {
        KS_THROW(kst_IOError, "'%T' object is not writeable", self);
        return false;
    }
    if (!ksio_is_bytes(self, &good)) return -1;
    if (!good) {
    //    KS_THROW(kst_IOError, "'%T' object is not a bytes-oriented IO", self);
    //    return -1;
    }


    if (kso_issub(self->type, ksiot_FileIO)) {
        ksio_FileIO fio = (ksio_FileIO)self;

        /* Unlock GIL for multithreading */
        KS_GIL_UNLOCK();
        size_t real_sz = fwrite(data, 1, sz_b, fio->fp);
        KS_GIL_LOCK();
    
        if (sz_b != real_sz) {
            KS_THROW(kst_IOError, "Failed to write to file: output was truncated (attempted %l bytes, only wrote %l)", (ks_cint)sz_b, (ks_cint)real_sz);
            return false;
        }

        fio->sz_w += real_sz;
        return true;

    } else if (kso_issub(self->type, ksiot_BytesIO) || kso_issub(self->type, ksiot_StringIO)) {
        ksio_StringIO sio = (ksio_StringIO)self;

        /* We always write to the end */
        if (sio->len_b + sz_b >= sio->max_len_b) {
            sio->max_len_b = ks_nextsize(sio->max_len_b, sio->len_b + sz_b);
            sio->data = ks_realloc(sio->data, sio->max_len_b);
        }

        /* Copy the memory to the end */
        memcpy(sio->data + sio->len_b, data, sz_b);

        /* Restore the position */
        sio->len_b += sz_b;
        sio->len_c += sz_b;

        return true;
    } else {
        ks_str key = ks_str_new(-1, "write");
        kso rf = kso_getattr(self, key);
        KS_DECREF(key);
        if (!rf) return false;

        ks_bytes bio = ks_bytes_new(sz_b, data);
        kso rr = kso_call(rf, 1, (kso[]){ (kso)bio });
        KS_DECREF(bio);
        KS_DECREF(rf);
        if (!rr) {
            return false;
        }
        KS_DECREF(rr);
        return true;
    }

    KS_THROW(kst_IOError, "Don't know how to write bytes to '%T' object", self);
    return false;
}

bool ksio_writes(ksio_BaseIO self, ks_ssize_t sz_b, const void* data) {
    if (sz_b < 0) sz_b = strlen(data);
    bool good;

    if (!ksio_is_open(self, &good)) {    
        return -1;
    }
    if (!good) {

        KS_THROW(kst_IOError, "'%T' object is not open", self);
        return -1;
    }
    if (!ksio_is_w(self, &good)) return -1;
    if (!good) {
        KS_THROW(kst_IOError, "'%T' object is not writeable", self);
        return -1;
    }
    if (!ksio_is_str(self, &good)) return -1;
    if (!good) {
    //    KS_THROW(kst_IOError, "'%T' object is not a str-oriented IO", self);
    //    return -1;
    }

    if (kso_issub(self->type, ksiot_FileIO)) {
        ksio_FileIO fio = (ksio_FileIO)self;

        /* Unlock GIL for multithreading */
        KS_GIL_UNLOCK();
        /* Assume UTF-8 */
        size_t real_sz = fwrite(data, 1, sz_b, fio->fp);
        KS_GIL_LOCK();
    
        if (sz_b != real_sz) {
            KS_THROW(kst_IOError, "Failed to write to file: output was truncated (attempted %l bytes, only wrote %l)", (ks_cint)sz_b, (ks_cint)real_sz);
            return -1;
        }

        fio->sz_w += real_sz;
        return real_sz;

    } else if (kso_issub(self->type, ksiot_BytesIO) || kso_issub(self->type, ksiot_StringIO)) {
        ksio_StringIO sio = (ksio_StringIO)self;

        /* We always write to the end */
        if (sio->len_b + sz_b >= sio->max_len_b) {
            sio->max_len_b = ks_nextsize(sio->max_len_b, sio->len_b + sz_b);
            sio->data = ks_realloc(sio->data, sio->max_len_b);
        }

        /* Copy the memory to the end */
        memcpy(sio->data + sio->len_b, data, sz_b);

        /* Restore the position */
        sio->len_b += sz_b;
        sio->len_c += ks_str_lenc(sz_b, data);

        return sz_b;
    } else {
        ks_str key = ks_str_new(-1, "write");
        kso rf = kso_getattr(self, key);
        KS_DECREF(key);
        if (!rf) return false;

        ks_str bio = ks_str_new(sz_b, data);
        kso rr = kso_call(rf, 1, (kso[]){ (kso)bio });
        KS_DECREF(bio);
        KS_DECREF(rf);
        if (!rr) {
            return false;
        }
        KS_DECREF(rr);
        return true;
    }

    KS_THROW(kst_IOError, "Don't know how to write str to '%T' object", self);
    return false;
}


/* Type Functions */

static KS_TFUNC(T, bool) {
    ksio_BaseIO self;
    KS_ARGS("self:*", &self, ksiot_BaseIO);

    bool g;
    if (!ksio_is_open(self, &g)) return NULL;

    return KSO_BOOL(g);
}

static KS_TFUNC(T, close) {
    ksio_BaseIO self;
    KS_ARGS("self:*", &self, ksiot_BaseIO);


    KS_THROW(kst_TypeError, "'%T' did not implement the 'close()' method of 'io.BaseIO'", self);
    return NULL;
}

static KS_TFUNC(T, read) {
    ksio_BaseIO self;
    ks_cint sz = KS_CINT_MAX;
    KS_ARGS("self:* ?sz:cint", &self, ksiot_BaseIO, &sz);

    KS_THROW(kst_TypeError, "'%T' did not implement the 'read()' method of 'io.BaseIO'", self);
    return NULL;
}

static KS_TFUNC(T, write) {
    ksio_BaseIO self;
    kso msg;
    KS_ARGS("self:* msg", &self, ksiot_BaseIO, &msg);

    KS_THROW(kst_TypeError, "'%T' did not implement the 'write()' method of 'io.BaseIO'", self);
    return NULL;
}

/** Iterable type **/

typedef struct _iter_s {
    KSO_BASE

    ksio_BaseIO of;

}* _iter;

static struct ks_type_s tpi;
static ks_type ksiot_BaseIO_iter = &tpi;

static KS_TFUNC(TI, free) {
    _iter self;
    KS_ARGS("self:*", &self, ksiot_BaseIO_iter);

    KS_DECREF(self->of);

    KSO_DEL(self);

    return KSO_NONE;
}

static KS_TFUNC(TI, init) {
    _iter self;
    ksio_BaseIO of;
    KS_ARGS("self:* of:*", &self, ksiot_BaseIO_iter, &of, ksiot_BaseIO);

    KS_INCREF(of);
    self->of = of;

    return KSO_NONE;
}

static KS_TFUNC(TI, next) {
    _iter self;
    KS_ARGS("self:*", &self, ksiot_BaseIO_iter);

    bool g;
    if (!ksio_is_open(self->of, &g)) return NULL;
    if (!g) {
        KS_OUTOFITER();
        return NULL;
    }

    bool isbin;
    if (!ksio_is_bytes(self->of, &isbin)) return NULL;

    char* data = NULL;
    ks_ssize_t num_c, rsz = 0;

    while (true) {
        data = ks_realloc(data, rsz + 4);
        ks_ssize_t csz = 0;
        if (isbin) {
            csz = ksio_readb(self->of, 1, data + rsz);
        } else {
            csz = ksio_reads(self->of, 1, data + rsz, &num_c);
        }
        rsz += csz;
        if (csz < 0) {
            ks_free(data);
            return NULL;
        } else if (csz == 0) {
            ks_str res = ks_str_new(rsz, data);
            ks_free(data);
            return (kso)res;
        } else if (data[rsz - csz] == '\n') {
            rsz--;
            if (rsz > 0 && data[rsz - 1] == '\r') {
                rsz--;
            }
            kso res = NULL;
            if (isbin) {
                res = (kso)ks_bytes_new(rsz, data);
            } else {
                res = (kso)ks_str_new(rsz, data);
            }
            ks_free(data);
            return (kso)res;
        }
    }
    assert(false);
    return NULL;
}



/* Export */


static struct ks_type_s tp;
ks_type ksiot_BaseIO = &tp;


void _ksi_io_BaseIO() {
    _ksinit(ksiot_BaseIO_iter, kst_object, TI_NAME, sizeof(struct _iter_s), -1, "", KS_IKV(
        {"__free",                 ksf_wrap(TI_free_, TI_NAME ".__free(self)", "")},
        {"__init",                 ksf_wrap(TI_init_, TI_NAME ".__init(self, of)", "")},
        {"__next",                 ksf_wrap(TI_next_, TI_NAME ".__next(self)", "")},

    ));

    _ksinit(ksiot_BaseIO, kst_object, T_NAME, sizeof(struct kso_s), -1, "Abstract base type of other IO objects", KS_IKV(
        {"__bool",                 ksf_wrap(T_bool_, T_NAME ".__bool(self)", "")},
        {"__iter",                 KS_NEWREF(ksiot_BaseIO_iter)},

        {"read",                   ksf_wrap(T_read_, T_NAME ".read(self, sz=-1)", "Reads a message from the stream")},
        {"write",                  ksf_wrap(T_write_, T_NAME ".write(self, msg)", "Writes a messate to the stream")},
        {"close",                  ksf_wrap(T_close_, T_NAME ".close(self)", "Closes the stream")},
    ));
}
