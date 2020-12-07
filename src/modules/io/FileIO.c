/* io/FileIO.c - 'io.FileIO' type
 *
 * 
 * @author: Cade Brown <cade@kscript.org>
 */
#include <ks/impl.h>

#define T_NAME "io.FileIO"


/* C-API */

ksio_FileIO ksio_FileIO_wrap(ks_type tp, FILE* fp, bool do_close, bool is_r, bool is_w, bool is_bin, ks_str src_name) {
    ksio_FileIO self = KSO_NEW(ksio_FileIO, tp);

    self->do_close = do_close;
    self->is_r = is_r;
    self->is_w = is_w;
    self->is_bin = is_bin;
    KS_INCREF(src_name);
    self->src_name = src_name;
    self->fp = fp;
    self->sz_r = self->sz_w = 0;
    self->is_open = true;

    return self;
}

ks_ssize_t ksio_FileIO_readb(ksio_FileIO self, ks_ssize_t sz_b, void* data) {
    if (!self->is_open) {
        KS_THROW(kst_IOError, "File is not open");
        return -1;
    }

    if (!self->is_r) {
        KS_THROW(kst_IOError, "File is not open for reading");
        return -1;
    }

    /* Unlock GIL for IO */
    KS_GIL_UNLOCK();
    size_t real_sz = fread(data, 1, sz_b, self->fp);
    self->sz_r += real_sz;
    KS_GIL_LOCK();

    return real_sz;
}

ks_ssize_t ksio_FileIO_reads(ksio_FileIO self, ks_ssize_t sz_c, void* data, ks_ssize_t* num_c) {
    if (!self->is_open) {
        KS_THROW(kst_IOError, "File is not open");
        return -1;
    }

    if (!self->is_r) {
        KS_THROW(kst_IOError, "File is not open for reading");
        return -1;
    }

    ks_ssize_t sz_b = 0;
    char utf8[5];
    *num_c = 0;
    bool keep_going = true;
    while (*num_c < sz_c && keep_going) {
        /* Read single character */


        /* Assume UTF8 */
        int i;
        for (i = 0; i < 4; ++i) {
            int c = fgetc(self->fp);
            if (c == EOF) {
                keep_going = false;
                break;
            }

            self->sz_r++;
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
}

bool ksio_FileIO_writeb(ksio_FileIO self, ks_ssize_t sz_b, const void* data) {
    if (!self->is_open) {
        KS_THROW(kst_IOError, "File is not open");
        return false;
    }

    if (!self->is_w) {
        KS_THROW(kst_IOError, "File is not open for writing");
        return false;
    }

    /* Unlock GIL for IO */
    KS_GIL_UNLOCK();
    size_t real_sz = fwrite(data, 1, sz_b, self->fp);
    self->sz_w += real_sz;
    //fflush(self->fp);
    KS_GIL_LOCK();

    if (sz_b != real_sz) {
        KS_THROW(kst_IOError, "Failed to write to file: output was truncated (attempted %l bytes, only wrote %l)", (ks_cint)sz_b, (ks_cint)real_sz);
        return false;
    }

    return true;
}

bool ksio_FileIO_writes(ksio_FileIO self, ks_ssize_t sz_b, const void* data) {

    /* Assume output is UTF8 */
    return ksio_FileIO_writeb(self, sz_b, data);
}


/* Type Functions */

static KS_TFUNC(T, init) {
    ksio_FileIO self;
    kso ssrc;
    ks_str mode = _ksv_r;
    KS_ARGS("self:* src ?mode:*", &self, ksiot_FileIO, &ssrc, &mode, kst_str);

    ks_str src = NULL;
    if (kso_issub(ssrc->type, kst_str)) {
        KS_INCREF(ssrc);
        src = (ks_str)ssrc;
    } else if (kso_issub(ssrc->type, ksost_path)) {
        src = ks_fmt("%S", ssrc);
        if (!src) return NULL;
    } else {
        KS_THROW(kst_Error, "Expected 'src' to be either 'str' or 'os.path', but got '%T' object", ssrc);
        return NULL;
    }

    bool is_r = false, is_w = false, is_bin = false;

    if (ks_str_eq_c(mode, "r", 1)) {
        is_r = true;
    } else if (ks_str_eq_c(mode, "w", 1)) {
        is_w = true;
    } else if (ks_str_eq_c(mode, "a", 1)) {
        is_w = true;
    } else if (ks_str_eq_c(mode, "r+", 2) || ks_str_eq_c(mode, "w+", 2) || ks_str_eq_c(mode, "a+", 2)) {
        is_r = true;
        is_w = true;
    } else if (ks_str_eq_c(mode, "rb", 2)) {
        is_r = true;
        is_bin = true;
    } else if (ks_str_eq_c(mode, "r+b", 3) || ks_str_eq_c(mode, "rb+", 3)) {
        is_r = true;
        is_w = true;
        is_bin = true;
    } else if (ks_str_eq_c(mode, "wb", 2)) {
        is_w = true;
        is_bin = true;
    } else if (ks_str_eq_c(mode, "w+b", 3) || ks_str_eq_c(mode, "wb+", 3)) {
        is_r = true;
        is_w = true;
        is_bin = true;
    } else if (ks_str_eq_c(mode, "ab", 2) || ks_str_eq_c(mode, "a+b", 3) || ks_str_eq_c(mode, "ab+", 3)) {
        is_w = true;
        is_bin = true;
    } else {
        KS_THROW(kst_Error, "Invalid mode: %R", mode);
        return NULL;
    }

    //ksos_path path = ksos_path_new_o(ssrc);
    //if (!path) return NULL;

    /*bool isdir;
    if (!ksos_path_isdir(path, &isdir)) {
        KS_DECREF(path);
        return NULL;
    }
    KS_DECREF(path);
    if (isdir) {
        KS_THROW(kst_IOError, "Failed to open %R: Is a directory", src);
        return NULL;
    }*/

    /* Attempt to open via the C library 
     * TODO: check filesystem encoding
     */
    FILE* fp = fopen(src->data, mode->data);
    if (!fp) {
        KS_THROW(kst_IOError, "Failed to open %R: %s", src, strerror(errno));
        return NULL;
    }

    KS_INCREF(src);
    self->src_name = src;
    self->is_r = is_r;
    self->is_w = is_w;
    self->is_bin = is_bin;
    self->do_close = true;
    self->sz_r = self->sz_w;
    self->fp = fp;
    self->is_open = true;

    return KSO_NONE;
}

static KS_TFUNC(T, free) {
    ksio_FileIO self;
    KS_ARGS("self:*", &self, ksiot_FileIO);

    KS_NDECREF(self->src_name);
    if (self->do_close && self->is_open && self->fp) {
        fclose(self->fp);
    }

    KSO_DEL(self);

    return KSO_NONE;
}
static KS_TFUNC(T, bool) {
    ksio_FileIO self;
    KS_ARGS("self:*", &self, ksiot_FileIO);

    return KSO_BOOL(!feof(self->fp));
}

static KS_TFUNC(T, repr) {
    ksio_FileIO self;
    KS_ARGS("self:*", &self, ksiot_FileIO);

    bool both = self->is_r && self->is_w;
    return (kso)ks_fmt("<'%T' @ %p (src=%R, mode='%s%s')>", self, self, self->src_name, both ? "r+" : self->is_r ? "r" : "w", self->is_bin ? "b" : "");
}

static KS_TFUNC(T, close) {
    ksio_FileIO self;
    KS_ARGS("self:*", &self, ksiot_FileIO);
    if (self->do_close && self->is_open && self->fp) {
        self->is_open = false;
        if (fclose(self->fp) != 0) {
            KS_THROW(kst_IOError, "Failed to close %R: %s", self, strerror(errno));
            return NULL;
        }
    }

    return KSO_NONE;
}

static KS_TFUNC(T, read) {
    ksio_FileIO self;
    ks_cint sz = KS_CINT_MAX;
    KS_ARGS("self:* ?sz:cint", &self, ksiot_FileIO, &sz);
    
    if (self->is_bin) {
        /* Read bytes */
        ks_ssize_t bsz = KSIO_BUFSIZ, rsz = 0;
        void* dest = NULL;
        while (rsz < sz) {
            dest = ks_realloc(dest, rsz + bsz);
            ks_ssize_t csz = ksio_FileIO_readb(self, bsz, ((char*)dest) + rsz);
            if (csz < 0) {
                ks_free(dest);
                return NULL;
            }
            rsz += csz;
            if (csz == 0) break;
        }

        ks_bytes res = ks_bytes_new(rsz, dest);
        ks_free(dest);
        return (kso)res;
    } else {
        /* Read string */
        ks_ssize_t bsz = KSIO_BUFSIZ, rsz = 0, num_c;
        void* dest = NULL;
        while (rsz < sz) {
            dest = ks_realloc(dest, rsz + bsz * 4);
            ks_ssize_t csz = ksio_FileIO_reads(self, bsz, ((char*)dest) + rsz, &num_c);
            if (csz < 0) {
                ks_free(dest);
                return NULL;
            }
            rsz += csz;
            if (csz == 0) break;
        }

        ks_str res = ks_str_new(rsz, dest);
        ks_free(dest);
        return (kso)res;
    }
}

static KS_TFUNC(T, write) {
    ksio_FileIO self;
    kso msg;
    KS_ARGS("self:* msg", &self, ksiot_FileIO, &msg);
    if (self->is_bin) {
        /* Write bytes */
        ks_bytes vm = kso_bytes(msg);
        if (!vm) return NULL;
        if (!ksio_FileIO_writeb(self, vm->len_b, vm->data)) {
            KS_DECREF(vm);
            return NULL;
        }
        KS_DECREF(vm);
    } else {
        /* Write string */
        ks_str vm = ks_fmt("%S", msg);
        if (!vm) return NULL;
        if (!ksio_FileIO_writes(self, vm->len_b, vm->data)) {
            KS_DECREF(vm);
            return NULL;
        }
        KS_DECREF(vm);
    }
    return KSO_NONE;
}


/** Iterable type **/

#define TI_NAME T_NAME ".__iter__"

typedef struct _iter_s {
    KSO_BASE

    ksio_FileIO of;
}* _iter;


static struct ks_type_s _t_iter;
static ks_type t_iter = &_t_iter;

static KS_TFUNC(TI, free) {
    _iter self;
    KS_ARGS("self:*", &self, t_iter);

    KS_DECREF(self->of);

    KSO_DEL(self);

    return KSO_NONE;
}

static KS_TFUNC(TI, init) {
    _iter self;
    ksio_FileIO of;
    KS_ARGS("self:* of:*", &self, t_iter, &of, ksiot_FileIO);

    KS_INCREF(of);
    self->of = of;

    return KSO_NONE;
}

static KS_TFUNC(TI, next) {
    _iter self;
    KS_ARGS("self:*", &self, t_iter);

    if (feof(self->of->fp)) {
        KS_OUTOFITER();
        return NULL;
    }

    char* data = NULL;
    ks_ssize_t num_c, rsz = 0;

    while (true) {
        data = ks_realloc(data, rsz + 4);
        ks_ssize_t csz = ksio_FileIO_reads(self->of, 1, data + rsz, &num_c);
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
            ks_str res = ks_str_new(rsz, data);
            ks_free(data);
            return (kso)res;
        }
    }
    assert(false);
    return NULL;
}



/* Export */

static struct ks_type_s tp;
ks_type ksiot_FileIO = &tp;

void _ksi_io_FileIO() {
    _ksinit(ksiot_FileIO, kst_object, T_NAME, sizeof(struct ksio_FileIO_s), -1, "File input/output stream, which implements read, write, flush, seek, and so forth\n\n    Internally, uses the 'FILE*' type in C, and makes most of the functionality available", KS_IKV(

    ));
}
