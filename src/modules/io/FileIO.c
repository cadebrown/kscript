/* io/FileIO.c - 'io.FileIO' type
 *
 * 
 * @author: Cade Brown <cade@kscript.org>
 */
#include <ks/impl.h>

#define T_NAME "io.FileIO"
#define TI_NAME T_NAME ".__iter"


/* C-API */

ksio_FileIO ksio_FileIO_wrap(ks_type tp, FILE* fp, bool do_close, bool is_r, bool is_w, bool is_bin, ks_str src_name) {
    ksio_FileIO self = KSO_NEW(ksio_FileIO, tp);

    self->do_close = do_close;
    self->is_r = is_r;
    self->is_w = is_w;
    self->is_bin = is_bin;
    KS_INCREF(src_name);
    self->fname = src_name;
    self->fp = fp;
    self->sz_r = self->sz_w = 0;
    self->is_open = true;

    return self;
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

    /* Attempt to open via the C library 
     * TODO: check filesystem encoding
     */
    FILE* fp = fopen(src->data, mode->data);
    if (!fp) {
        KS_THROW(kst_IOError, "Failed to open %R: %s", src, strerror(errno));
        return NULL;
    }

    self->fname = src;
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

    KS_DECREF(self->fname);
    
    if (self->do_close && self->is_open && self->fp) {
        fclose(self->fp);
    }

    KSO_DEL(self);

    return KSO_NONE;
}

static KS_TFUNC(T, str) {
    ksio_FileIO self;
    KS_ARGS("self:*", &self, ksiot_FileIO);

    bool both = self->is_r && self->is_w;
    return (kso)ks_fmt("<%T (src=%R, mode='%s%s')>", self, self->fname, both ? "r+" : self->is_r ? "r" : "w", self->is_bin ? "b" : "");
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
    if (self->is_bin) {
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


/* Export */

static struct ks_type_s tp;
ks_type ksiot_FileIO = &tp;


void _ksi_io_FileIO() {

    _ksinit(ksiot_FileIO, ksiot_BaseIO, T_NAME, sizeof(struct ksio_FileIO_s), -1, "File input/output stream, which implements read, write, flush, seek, and so forth\n\n    Internally, uses the 'FILE*' type in C, and makes most of the functionality available", KS_IKV(
        {"__free",                 ksf_wrap(T_free_, T_NAME ".__free(self)", "")},
        {"__init",                 ksf_wrap(T_init_, T_NAME ".__init(self, src, mode='r')", "")},
        {"__repr",                 ksf_wrap(T_str_, T_NAME ".__repr(self)", "")},
        {"__str",                  ksf_wrap(T_str_, T_NAME ".__str(self)", "")},

        {"read",                   ksf_wrap(T_read_, T_NAME ".read(self, sz=-1)", "Reads a message from the stream")},
        {"write",                  ksf_wrap(T_write_, T_NAME ".write(self, msg)", "Writes a messate to the stream")},

    ));
}
