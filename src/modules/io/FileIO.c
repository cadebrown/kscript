/* io/FileIO.c - 'io.FileIO' type
 *
 * 
 * @author: Cade Brown <cade@kscript.org>
 */
#include <ks/impl.h>

#define T_NAME "io.FileIO"
#define TI_NAME T_NAME ".__iter"


/* C-API */
ksio_FileIO ksio_FileIO_wrap(ks_type tp, FILE* fp, bool do_close, ks_str src, ks_str mode) {
    ksio_FileIO self = KSO_NEW(ksio_FileIO, tp);

    self->fp = fp;
    self->do_close = do_close;

    KS_INCREF(src);
    self->src = src;
    KS_INCREF(mode);
    self->mode = mode;

    if (!ksio_modeinfo(self->mode, &self->mr, &self->mw, &self->mb)) {
        KS_DECREF(self);
        return NULL;
    }

    return self;
}
ksio_FileIO ksio_FileIO_fdopen(int fd, ks_str src, ks_str mode) {
#ifdef KS_HAVE_fdopen
    FILE* res = fdopen(fd, mode->data);
    if (!res) {
        KS_THROW_ERRNO(errno, "Failed to fdopen %i", fd);
        return NULL;
    }

    return ksio_FileIO_wrap(ksiot_FileIO, res, false /* don't close */, src, mode);
#else
    KS_THROW(kst_OSError, "Failed to fdopen: platform did not provide a 'fdopen()' function");
    return -1;
#endif
}


/* Type Functions */

static KS_TFUNC(T, init) {
    ksio_FileIO self;
    kso ssrc;
    ks_str mode = _ksv_r;
    KS_ARGS("self:* src ?mode:*", &self, ksiot_FileIO, &ssrc, &mode, kst_str);

    ks_str src = NULL;
    int ssrc_fd = -1;
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

    /* Attempt to open via the C library 
     * TODO: check filesystem encoding
     */
    self->fp = fopen(src->data, mode->data);
    if (!self->fp) {
        KS_THROW_ERRNO(errno, "Failed to open %R", src);
        KS_DECREF(src);
        return NULL;
    }

    self->do_close = true;

    self->src = src;
    KS_INCREF(mode);
    self->mode = mode;

    if (!ksio_modeinfo(self->mode, &self->mr, &self->mw, &self->mb)) {
        return NULL;
    }

    return KSO_NONE;
}

static KS_TFUNC(T, free) {
    ksio_FileIO self;
    KS_ARGS("self:*", &self, ksiot_FileIO);

    KS_NDECREF(self->src);
    KS_NDECREF(self->mode);
    
    if (!ksio_close((ksio_BaseIO)self)) {
        return NULL;
    }

    KSO_DEL(self);

    return KSO_NONE;
}

static KS_TFUNC(T, str) {
    ksio_FileIO self;
    KS_ARGS("self:*", &self, ksiot_FileIO);

    return (kso)ks_fmt("<%T (src=%R, mode=%R)>", self, self->src, self->mode);
}


static KS_TFUNC(T, int) {
    ksio_FileIO self;
    KS_ARGS("self:*", &self, ksiot_FileIO);

    return (kso)ks_int_new(fileno(self->fp));
}


static KS_TFUNC(T, read) {
    ksio_FileIO self;
    ks_cint sz = KS_CINT_MAX;
    KS_ARGS("self:* ?sz:cint", &self, ksiot_FileIO, &sz);
    
    if (self->mb) {
        /* Read bytes */
        ks_ssize_t bsz = KSIO_BUFSIZ, rsz = 0;
        void* dest = NULL;
        while (rsz < sz) {
            dest = ks_realloc(dest, rsz + bsz);
            ks_ssize_t csz = ksio_readb((ksio_BaseIO)self, bsz, ((char*)dest) + rsz);
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
            ks_ssize_t csz = ksio_reads((ksio_BaseIO)self, bsz, ((char*)dest) + rsz, &num_c);
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
    if (self->mb) {
        /* Write bytes */
        ks_bytes vm = kso_bytes(msg);
        if (!vm) return NULL;
        if (ksio_writeb((ksio_BaseIO)self, vm->len_b, vm->data) < 0) {
            KS_DECREF(vm);
            return NULL;
        }
        KS_DECREF(vm);
    } else {
        /* Write string */
        ks_str vm = ks_fmt("%S", msg);
        if (!vm) {
            return NULL;
        }
        if (ksio_writes((ksio_BaseIO)self, vm->len_b, vm->data) < 0) {
            KS_DECREF(vm);
            return NULL;
        }
        KS_DECREF(vm);
    }
    return KSO_NONE;
}


static KS_TFUNC(T, getattr) {
    ksio_FileIO self;
    ks_str attr;
    KS_ARGS("self:* attr:*", &self, ksiot_FileIO, &attr, kst_str);

    if (ks_str_eq_c(attr, "fileno", 6)) {
        return (kso)ks_int_new(fileno(self->fp));
    }

    KS_THROW_ATTR(self, attr);
    return NULL;
}


static KS_TFUNC(T, setattr) {
    ksio_FileIO self;
    ks_str attr;
    kso val;
    KS_ARGS("self:* attr:* val", &self, ksiot_FileIO, &attr, kst_str, &val);

    if (ks_str_eq_c(attr, "fileno", 6)) {
        KS_THROW(kst_AttrError, "Cannot set '.fileno', is read only");
        return NULL;
    }

    KS_THROW_ATTR(self, attr);
    return NULL;
}


/* Export */

static struct ks_type_s tp;
ks_type ksiot_FileIO = &tp;


void _ksi_io_FileIO() {

    _ksinit(ksiot_FileIO, ksiot_BaseIO, T_NAME, sizeof(struct ksio_FileIO_s), -1, "File input/output stream, which implements read, write, flush, seek, and so forth\n\n    Internally, uses the 'FILE*' type in C, and makes most of the functionality available", KS_IKV(
        {"__free",                 ksf_wrap(T_free_, T_NAME ".__free(self)", "")},
        {"__init",                 ksf_wrap(T_init_, T_NAME ".__init(self, src, mode='r')", "")},
        {"__repr",                 ksf_wrap(T_str_, T_NAME ".__repr(self)", "")},
        {"__str",                  ksf_wrap(T_str_, T_NAME ".__str(self)", "")},
        {"__getattr",              ksf_wrap(T_getattr_, T_NAME ".__getattr(self, attr)", "")},
        {"__setattr",              ksf_wrap(T_setattr_, T_NAME ".__setattr(self, attr)", "")},

        {"__integral",             ksf_wrap(T_int_, T_NAME ".__integral(self)", "Acts as 'self.fileno'")},

        {"read",                   ksf_wrap(T_read_, T_NAME ".read(self, sz=-1)", "Reads a message from the stream")},
        {"write",                  ksf_wrap(T_write_, T_NAME ".write(self, msg)", "Writes a messate to the stream")},

    ));
}
