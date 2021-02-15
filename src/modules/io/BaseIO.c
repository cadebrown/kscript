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

bool ksio_close(ksio_BaseIO self) {
    if (kso_issub(self->type, ksiot_FileIO)) {
        ksio_FileIO fio = (ksio_FileIO)self;
        if (fio->do_close && fio->fp) {
            if (fclose(fio->fp) != 0) {
                KS_THROW_ERRNO(errno, "Failed to close %R");
                return false;
            }
        }
        fio->fp = NULL;

        return true;

    } else if (kso_issub(self->type, ksiot_RawIO) && self->type == ksiot_RawIO) {
        ksio_RawIO rio = (ksio_RawIO)self;

        if (rio->do_close && rio->fd >= 0) {
            if (close(rio->fd) != 0) {
                KS_THROW_ERRNO(errno, "Failed to close %R");
                return false;
            }
        }
        rio->fd = -1;

        return true;
    } else if (kso_issub(self->type, ksiot_BytesIO)) {
        ksio_BytesIO bio = (ksio_BytesIO)self;

        return true;
    } else if (kso_issub(self->type, ksiot_StringIO)) {
        ksio_StringIO bio = (ksio_StringIO)self;

        return true;
    } else {
        ks_str key = ks_str_new(-1, "close");
        kso rf = kso_getattr(self, key);
        KS_DECREF(key);
        if (!rf) return -1;

        kso res = kso_call(rf, 0, NULL);
        KS_DECREF(rf);
        if (!res) {
            return false;
        }
        KS_DECREF(res);
        return true;
    }
}

bool ksio_seek(ksio_BaseIO self, ks_cint pos, int whence) {
    if (kso_issub(self->type, ksiot_FileIO)) {
        ksio_FileIO fio = (ksio_FileIO)self;
        
        int cw;
        if (whence == KSIO_SEEK_SET) {
            cw = SEEK_SET;
        } else if (whence == KSIO_SEEK_CUR) {
            cw = SEEK_CUR;
        } else if (whence == KSIO_SEEK_END) {
            cw = SEEK_END;
        } else {
            KS_THROW(kst_Error, "Unknown 'whence': %i", whence);
            return NULL;
        }
        int rc = fseek(fio->fp, pos, whence);
        int eno = errno;
        if (rc < 0) {
            KS_THROW_ERRNO(errno, "Failed to seek %R", self);
            return NULL;
        }

        return true;

    } else if (kso_issub(self->type, ksiot_RawIO)) {
        ksio_RawIO rio = (ksio_RawIO)self;

        int cw;
        if (whence == KSIO_SEEK_SET) {
            cw = SEEK_SET;
        } else if (whence == KSIO_SEEK_CUR) {
            cw = SEEK_CUR;
        } else if (whence == KSIO_SEEK_END) {
            cw = SEEK_END;
        } else {
            KS_THROW(kst_Error, "Unknown 'whence': %i", whence);
            return false;
        }
        int rc = lseek(rio->fd, pos, whence);
        int eno = errno;
        if (rc < 0) {
            KS_THROW_ERRNO(errno, "Failed to seek %R", self);
            return false;
        }

        return true;
    } else if (kso_issub(self->type, ksiot_BytesIO)) {
        ksio_BytesIO bio = (ksio_BytesIO)self;

        ks_cint res = pos;
        int cw;
        if (whence == KSIO_SEEK_SET) {
        } else if (whence == KSIO_SEEK_CUR) {
            res += bio->pos_b;
        } else if (whence == KSIO_SEEK_END) {
            res += bio->len_b;
        } else {
            KS_THROW(kst_Error, "Unknown 'whence': %i", whence);
            return false;
        }

        bio->pos_b = bio->pos_c = res;

        return true;
    } else if (kso_issub(self->type, ksiot_StringIO)) {
        KS_THROW(kst_IOError, "Failed to seek %R: String-based IOs cannot be seek'd; use byte-based IOs", self);
        return false;
    } else {
        ks_str key = ks_str_new(-1, "seek");
        kso rf = kso_getattr(self, key);
        KS_DECREF(key);
        if (!rf) return -1;

        ks_int v0 = ks_int_new(pos), v1 = ks_int_new(whence);

        kso res = kso_call(rf, 2, (kso[]){ (kso)v0, (kso)v1 });
        KS_DECREF(v0);
        KS_DECREF(v1);
        KS_DECREF(rf);
        if (!res) {
            return false;
        }
        KS_DECREF(res);
        return true;
    }
}

ks_cint ksio_tell(ksio_BaseIO self) {
    if (kso_issub(self->type, ksiot_FileIO)) {
        ksio_FileIO fio = (ksio_FileIO)self;
        long rc = ftell(fio->fp);
        int eno = errno;
        if (rc < 0) {
            KS_THROW_ERRNO(errno, "Failed to tell %R", self);
            return -1;
        }

        return rc;

    } else if (kso_issub(self->type, ksiot_RawIO)) {
        ksio_RawIO rio = (ksio_RawIO)self;

        int rc = lseek(rio->fd, 0, SEEK_CUR);
        int eno = errno;
        if (rc < 0) {
            KS_THROW_ERRNO(errno, "Failed to tell %R", self);
            return -1;
        }

        return rc;
    } else if (kso_issub(self->type, ksiot_BytesIO)) {
        ksio_BytesIO bio = (ksio_BytesIO)self;

        return bio->pos_b;
    } else if (kso_issub(self->type, ksiot_StringIO)) {
        ksio_StringIO sio = (ksio_StringIO)self;
        return sio->pos_c;
    } else {
        ks_str key = ks_str_new(-1, "tell");
        kso rf = kso_getattr(self, key);
        KS_DECREF(key);
        if (!rf) return -1;

        kso res = kso_call(rf, 0, NULL);
        KS_DECREF(rf);
        if (!res) {
            return false;
        }
        KS_DECREF(res);
        return true;
    }
}

bool ksio_eof(ksio_BaseIO self, bool* out) {
    if (kso_issub(self->type, ksiot_FileIO)) {
        ksio_FileIO fio = (ksio_FileIO)self;

        *out = feof(fio->fp) != 0;

        return true;

    } else if (kso_issub(self->type, ksiot_RawIO)) {
        ksio_RawIO rio = (ksio_RawIO)self;

        /* Capture current position */
        ks_cint cpos = ksio_tell(self);
        if (cpos < 0) {
            return false;
        }

        /* Seek to the end */
        if (!ksio_seek(self, 0, KSIO_SEEK_END)) {
            return false;
        }

        /* Capture end position */
        ks_cint epos = ksio_tell(self);
        if (epos < 0) {
            return false;
        }

        /* Seek to where we were */
        if (!ksio_seek(self, cpos, KSIO_SEEK_SET)) {
            return false;
        }

        *out = cpos >= epos;
        return true;
    } else if (kso_issub(self->type, ksiot_BytesIO)) {
        ksio_BytesIO bio = (ksio_BytesIO)self;

        *out = bio->pos_b < bio->len_b;
        return true;
    } else if (kso_issub(self->type, ksiot_StringIO)) {
        ksio_StringIO sio = (ksio_StringIO)self;
        *out = sio->pos_b < sio->len_b;
        return true;
    } else {
        ks_str key = ks_str_new(-1, "eof");
        kso rf = kso_getattr(self, key);
        KS_DECREF(key);
        if (!rf) return -1;

        kso res = kso_call(rf, 0, NULL);
        KS_DECREF(rf);
        if (!res) {
            return false;
        }
        KS_DECREF(res);
        return true;
    }
}

bool ksio_trunc(ksio_BaseIO self, ks_cint sz) {
    if (kso_issub(self->type, ksiot_FileIO)) {
        ksio_FileIO fio = (ksio_FileIO)self;

#ifdef WIN32
		LONG up = sz >> 32;
		SetFilePointer(fileno(fio->fp), (UINT32)sz, &up, FILE_BEGIN);
		SetEndOfFile(fileno(fio->fp));
#else
        int rc = ftruncate(fileno(fio->fp), sz);
        if (rc < 0) {
            KS_THROW_ERRNO(errno, "Failed to trunc %R", self);
            return false;
        }
#endif
		return true;
    } else if (kso_issub(self->type, ksiot_RawIO)) {
        ksio_RawIO rio = (ksio_RawIO)self;

#ifdef WIN32
		LONG up = sz >> 32;
		SetFilePointer(rio->fd, (UINT32)sz, &up, FILE_BEGIN);
		SetEndOfFile(rio->fd);
#else
		int rc = ftruncate(rio->fd, sz);
		if (rc < 0) {
            KS_THROW_ERRNO(errno, "Failed to trunc %R", self);
			return false;
		}
#endif

        return true;
    } else if (kso_issub(self->type, ksiot_BytesIO)) {
        ksio_BytesIO bio = (ksio_BytesIO)self;

        if (sz > bio->len_b) {
            if (sz > bio->max_len_b) {
                bio->max_len_b = ks_nextsize(bio->max_len_b, sz);
                bio->data = ks_realloc(bio->data, bio->max_len_b);
            }
            int i;
            for (i = bio->len_b; i < sz; ++i) bio->data[i] = 0;
        }

        bio->len_b = bio->len_c = sz;
        if (bio->pos_b > bio->len_b) {
            bio->pos_b = bio->pos_c = bio->len_b;
        }
        return true;
    } else if (kso_issub(self->type, ksiot_StringIO)) {
        ksio_StringIO sio = (ksio_StringIO)self;

        if (sz > sio->len_b) {
            KS_THROW(kst_Error, "Cannot trunc StringIO objects to larger sizes (no zero padding)");
            return NULL;
        }
        sio->len_b = sz;
        sio->len_c = ks_str_lenc(sio->len_b, sio->data);
        if (sio->pos_b > sio->len_b) {
            sio->pos_b = sio->len_b;
            sio->pos_c = sio->len_c;
        }
        return true;
    } else {
        ks_str key = ks_str_new(-1, "trunc");
        kso rf = kso_getattr(self, key);
        KS_DECREF(key);
        if (!rf) return -1;

        ks_int v0 = ks_int_new(sz);
        kso res = kso_call(rf, 1, (kso[]){ (kso)v0 });
        KS_DECREF(v0);
        KS_DECREF(rf);
        if (!res) {
            return false;
        }
        KS_DECREF(res);
        return true;
    }
}


ks_ssize_t ksio_readb(ksio_BaseIO self, ks_ssize_t sz_b, void* data) {
    if (kso_issub(self->type, ksiot_FileIO)) {
        ksio_FileIO fio = (ksio_FileIO)self;

        /* Unlock GIL for multithreading */
        KS_GIL_UNLOCK();
        ks_ssize_t real_sz = fread(data, 1, sz_b, fio->fp);
        int eno = errno;
        KS_GIL_LOCK();

        /* Update state variables */
        fio->sz_r += real_sz;

        if (real_sz == 0 && sz_b != 0) {
            KS_THROW_ERRNO(eno, "Failed to read from %R", self);
            return -1;
        }

        return real_sz;

    } else if (kso_issub(self->type, ksiot_RawIO)) {
        ksio_RawIO rio = (ksio_RawIO)self;

        /* Unlock GIL for multithreading */
        KS_GIL_UNLOCK();
        ks_ssize_t real_sz = read(rio->fd, data, sz_b);
        int eno = errno;
        KS_GIL_LOCK();
        if (real_sz < 0) {
            KS_THROW_ERRNO(eno, "Failed to read from %R", self);
            return -1;
        }

        /*if (real_sz == 0) {
            rio->is_open = false;
        }*/

        /* Update state variables */
        rio->sz_r += real_sz;

        return real_sz;

    } else if (kso_issub(self->type, ksiot_BytesIO)) {
        ksio_BytesIO bio = (ksio_BytesIO)self;

        /* Number of bytes to read */
        ks_ssize_t rsz = bio->len_b - bio->pos_b;
        if (rsz > sz_b) rsz = sz_b;

        KS_GIL_UNLOCK();
        memcpy(data, bio->data + bio->pos_b, rsz);
        KS_GIL_LOCK();
        bio->pos_b += rsz;
        bio->pos_c = bio->pos_b;        

        return rsz;


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
    if (kso_issub(self->type, ksiot_FileIO)) {
        ksio_FileIO fio = (ksio_FileIO)self;

        /* Allow other threadings */
        KS_GIL_UNLOCK();

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

        KS_GIL_LOCK();


        return sz_b;
    } else if (kso_issub(self->type, ksiot_BytesIO) || kso_issub(self->type, ksiot_StringIO)) {
        ksio_StringIO sio = (ksio_StringIO)self;

        /* Done */
        KS_GIL_UNLOCK();

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

        KS_GIL_LOCK();

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

    } else if (kso_issub(self->type, ksiot_RawIO)) {
        ksio_RawIO rio = (ksio_RawIO)self;

        /* Unlock GIL for multithreading */
        KS_GIL_UNLOCK();
        ks_ssize_t real_sz = write(rio->fd, data, sz_b);
        int eno = errno;
        KS_GIL_LOCK();
        if (real_sz < 0) {
            KS_THROW_ERRNO(eno, "Failed to write to %R", self);
            return -1;
        }

        /* Update state variables */
        rio->sz_r += real_sz;

        return real_sz;
    } else if (kso_issub(self->type, ksiot_BytesIO) || kso_issub(self->type, ksiot_StringIO)) {
        ksio_StringIO sio = (ksio_StringIO)self;

        /* We always write to the end */
        if (sio->len_b + sz_b >= sio->max_len_b) {
            sio->max_len_b = ks_nextsize(sio->max_len_b, sio->len_b + sz_b);
            sio->data = ks_realloc(sio->data, sio->max_len_b);
        }

        /* Copy the memory to the end */
        KS_GIL_UNLOCK();
        memcpy(sio->data + sio->len_b, data, sz_b);
        KS_GIL_LOCK();

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
    } else if (kso_issub(self->type, ksiot_RawIO)) {
        ksio_RawIO rio = (ksio_RawIO)self;

        /* Unlock GIL for multithreading */
        KS_GIL_UNLOCK();
        ks_ssize_t real_sz = write(rio->fd, data, sz_b);
        int eno = errno;
        KS_GIL_LOCK();
        if (real_sz < 0) {
            KS_THROW_ERRNO(eno, "Failed to write to %R", self);
            return -1;
        }

        /* Update state variables */
        rio->sz_r += real_sz;

        return real_sz;
    } else if (kso_issub(self->type, ksiot_BytesIO) || kso_issub(self->type, ksiot_StringIO)) {
        ksio_StringIO sio = (ksio_StringIO)self;

        /* We always write to the end */
        if (sio->len_b + sz_b >= sio->max_len_b) {
            sio->max_len_b = ks_nextsize(sio->max_len_b, sio->len_b + sz_b);
            sio->data = ks_realloc(sio->data, sio->max_len_b);
        }

        /* Copy the memory to the end */
        KS_GIL_UNLOCK();
        memcpy(sio->data + sio->len_b, data, sz_b);
        KS_GIL_LOCK();

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

static KS_TFUNC(T, new) {
    ks_type tp;
    int nargs;
    kso* args;
    KS_ARGS("tp:* *args", &tp, kst_type, &nargs, &args);

    KS_THROW(kst_TypeError, "Type '%R' is abstract and cannot be created", tp);
    return NULL;
}

static KS_TFUNC(T, bool) {
    ksio_BaseIO self;
    KS_ARGS("self:*", &self, ksiot_BaseIO);
    bool g;
    if (!ksio_eof(self, &g)) {
        return NULL;
    }

    return KSO_BOOL(!g);
}

static KS_TFUNC(T, close) {
    ksio_BaseIO self;
    KS_ARGS("self:*", &self, ksiot_BaseIO);

    if (!ksio_close(self)) {
        return NULL;
    }

    return KSO_NONE;
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

static KS_TFUNC(T, seek) {
    ksio_BaseIO self;
    ks_cint pos, whence = KSIO_SEEK_SET;
    KS_ARGS("self:* ?pos:cint ?whence:cint", &self, ksiot_BaseIO, &pos, &whence);

    if (!ksio_seek(self, pos, whence)) {
        return NULL;
    }

    return KSO_NONE;
}

static KS_TFUNC(T, tell) {
    ksio_BaseIO self;
    KS_ARGS("self:*", &self, ksiot_BaseIO);

    ks_cint res = ksio_tell(self);
    if (res < 0) return NULL;

    return (kso)ks_int_new(res);
}

static KS_TFUNC(T, eof) {
    ksio_BaseIO self;
    KS_ARGS("self:*", &self, ksiot_BaseIO);

    bool g;
    if (!ksio_eof(self, &g)) return NULL;

    return KSO_BOOL(g);
}


static KS_TFUNC(T, trunc) {
    ksio_BaseIO self;
    ks_cint sz = 0;
    KS_ARGS("self:* ?sz:cint", &self, ksiot_BaseIO, &sz);

    if (!ksio_trunc(self, sz)) return NULL;

    return KSO_NONE;
}


static KS_TFUNC(T, printf) {
    ksio_BaseIO self;
    ks_str fmt;
    int nargs;
    kso* args;
    KS_ARGS("self:* fmt:* *args", &self, ksiot_BaseIO, &fmt, kst_str, &nargs, &args);

    if (!ks_fmt2(self, fmt->data, nargs, args)) {
        return NULL;
    }

    return KSO_NONE;
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

    bool is_r, is_w, is_b;
    if (!ksio_info(self->of, &is_r, &is_w, &is_b)) {
        return NULL;
    }

    /*
    bool g;
    if (!ksio_is_open(self->of, &g)) return NULL;
    if (!g) {
        KS_OUTOFITER();
        return NULL;
    }
    */



    char* data = NULL;
    ks_ssize_t num_c, rsz = 0;

    while (true) {
        data = ks_realloc(data, rsz + 4);
        ks_ssize_t csz = 0;
        if (is_b) {
            csz = ksio_readb(self->of, 1, data + rsz);
        } else {
            csz = ksio_reads(self->of, 1, data + rsz, &num_c);
        }

        rsz += csz;
        if (csz < 0) {
            ks_free(data);
            return NULL;
        } else if (csz == 0) {
            if (rsz == 0) {
                ks_free(data);
                KS_OUTOFITER();
                return NULL;
            }

            kso res = NULL;
            if (is_b) {
                res = (kso)ks_bytes_new(rsz, data);
            } else {
                res = (kso)ks_str_new(rsz, data);
            }
            ks_free(data);
            return (kso)res;
        } else if (data[rsz - csz] == '\n') {
            rsz--;
            if (rsz > 0 && data[rsz - 1] == '\r') {
                rsz--;
            }
            kso res = NULL;
            if (is_b) {
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
        {"__new",                  ksf_wrap(T_new_, T_NAME ".__new(tp)", "")},
        {"__bool",                 ksf_wrap(T_bool_, T_NAME ".__bool(self)", "")},
        {"__iter",                 KS_NEWREF(ksiot_BaseIO_iter)},

        {"close",                  ksf_wrap(T_close_, T_NAME ".close(self)", "Closes the stream")},
        {"seek",                   ksf_wrap(T_seek_, T_NAME ".seek(self, pos, whence=io.Seek.SET)", "Seeks to a position from a relative location\n\n    'whence': Either 'io.Seek.SET' (from start of stream), 'io.Seek.CUR' (from current position), or 'io.Seek.END' (from the end of the stream)")},
        {"tell",                   ksf_wrap(T_tell_, T_NAME ".tell(self)", "Return the current position of the stream, from the origin")},
        {"trunc",                  ksf_wrap(T_trunc_, T_NAME ".trunc(self, sz=0)", "Truncate the stream to a given size")},
        {"eof",                    ksf_wrap(T_eof_, T_NAME ".eof(self)", "Calculate whether the stream has hit the EOF indicator")},

        {"read",                   ksf_wrap(T_read_, T_NAME ".read(self, sz=-1)", "Reads a message from the stream")},
        {"write",                  ksf_wrap(T_write_, T_NAME ".write(self, msg)", "Writes a messate to the stream")},


        {"printf",                 ksf_wrap(T_printf_, T_NAME ".printf(self, fmt, *args)", "Prints a formatted message to the stream")},
    ));
}
