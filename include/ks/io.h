/* ks/io.h - header for the `io` (input/output) module of kscript
 *
 * Provides general interfaces to file streams, buffer streams, and more
 * 
 * General methods:
 *   s.read(sz=none): Read a given size (default: everything left) message and return it
 *   s.write(msg): Write a (string, bytes, or object) to the stream
 *   close(s): Close a stream
 * 
 * @author:    Cade Brown <cade@kscript.org>
 */

#pragma once
#ifndef KSIO_H__
#define KSIO_H__

#ifndef KS_H__
#include <ks/ks.h>
#endif

/** Constants **/

/* Buffer size */
#define KSIO_BUFSIZ 2048

/** Types **/

/* Any Input/Output stream */
typedef kso ksio_AnyIO;

/* 'io.FileIO' - represents a file that can be read or written
 */
typedef struct ksio_FileIO_s {
    KSO_BASE

    /* Whether the IO is open */
    bool is_open;

    /* Whether the file is text or binary */
    bool is_bin;

    /* Whether the FileIO is readable/writeable */
    bool is_r, is_w;

    /* Number of bytes read and written (not rigorous, don't rely on these) */
    ks_ssize_t sz_r, sz_w;


    /* If true, actually close 'fp' once the object has been closed */
    bool do_close;

    /* The name of the source */
    ks_str src_name;

    /* File descriptor from 'fopen()' */
    FILE* fp;

}* ksio_FileIO;

/* 'io.StringIO' - in-memory stream of Unicode text
 */
typedef struct ksio_StringIO_s {
    KSO_BASE

    /* Whether the FileIO is readable/writeable */
    bool is_r, is_w;

    /* Number of bytes read and written (not rigorous, don't rely on these) */
    ks_ssize_t sz_r, sz_w;


    /* Length of the data (in bytes and characters)  */
    int len_b, len_c;

    /* Allocated array of data */
    char* data;

    /* Internal maximum length it is allocated to hold */
    ks_ssize_t max_len_b;

}* ksio_StringIO;

/* 'io.BytesIO' - in-memory stream of raw bytes
 */
typedef struct ksio_BytesIO_s {
    KSO_BASE

    /* Whether the FileIO is readable/writeable */
    bool is_r, is_w;

    /* Number of bytes read and written (not rigorous, don't rely on these) */
    ks_ssize_t sz_r, sz_w;


    /* Length of the data (in bytes)  */
    int len_b;

    /* Allocated array of data */
    unsigned char* data;

    /* Internal maximum length it is allocated to hold */
    ks_ssize_t max_len_b;

}* ksio_BytesIO;



/** Unicode Translation **/



/* Decodes a single character from UTF8 source
 *
 * Set '_to' (which should be a 'ks_ucp') to the ordinal given by '_src' (in UTF8 encoding, as a 'char*') 
 * And, set '_n' to the number of bytes read
 */
#define KS_UCP_FROM_UTF8(_to, _src, _n) do { \
    unsigned char _c = (_src)[0]; \
    if (_c < 0x80) { \
        _to = _c; \
        _n = 1;   \
    } else if ((_c & 0xE0) == 0xC0) { \
        _to = ((ks_unich)(_c & 0x1F) << 6) \
            | ((ks_unich)((_src)[1] & 0x3F) << 0); \
        _n = 2; \
    } else if ((_c & 0xF0) == 0xE0) { \
        _to = ((ks_unich)(_c & 0x0F) << 12) \
            | ((ks_unich)((_src)[1] & 0x3F) << 6) \
            | ((ks_unich)((_src)[2] & 0x3F) << 0); \
        _n = 3; \
    } else if ((_c & 0xF8) == 0xF0 && (_c <= 0xF4)) { \
        _to = ((ks_unich)(_c & 0x07) << 18) \
            | ((ks_unich)((_src)[1] & 0x3F) << 12) \
            | ((ks_unich)((_src)[2] & 0x3F) << 6) \
            | ((ks_unich)((_src)[3] & 0x3F) << 0); \
        _n = 4; \
    } else { \
        _to = 0; \
        _n = -1; \
    } \
} while (0)

/* Encode a single character to UTF8
 *
 * Expects '_src' to be a 'ks_ucp', '_to' to be a 'char*',
 *   and '_n' be an assignable name which tells how many bytes were written
 *   to '_to'. '_to' should have at least 4 bytes allocated after it
 * 
 * See: https://www.fileformat.info/info/unicode/utf8.htm
 */
#define KS_UCP_TO_UTF8(_to, _n, _src) do { \
    if (_src <= 0x7F) { \
        _n = 1; \
        _to[0] = _src; \
    } else if (_src <= 0x7FF) { \
        _n = 2; \
        _to[0] = 0xC0 | ((_src >>  6) & 0x1F); \
        _to[1] = 0x80 | ((_src >>  0) & 0x3F); \
    } else if (_src <= 0xFFFF) { \
        _n = 3; \
        _to[0] = 0xE0 | ((_src >> 12) & 0x0F); \
        _to[1] = 0x80 | ((_src >>  6) & 0x3F); \
        _to[2] = 0x80 | ((_src >>  0) & 0x3F); \
    } else if (_src <= 0x10FFFF) { \
        _n = 4; \
        _to[0] = 0xF0 | ((_src >> 18) & 0x0F); \
        _to[1] = 0x80 | ((_src >> 12) & 0x3F); \
        _to[2] = 0x80 | ((_src >>  6) & 0x3F); \
        _to[3] = 0x80 | ((_src >>  0) & 0x3F); \
    } else { \
        _n = -1; \
        _to[0] = 0; \
    } \
} while (0)



/** Functions **/

/* Return a wrapper around an opened C-style FILE*
 */
KS_API ksio_FileIO ksio_FileIO_wrap(ks_type tp, FILE* fp, bool do_close, bool is_r, bool is_w, bool is_bin, ks_str src_name);

/* Read up to 'sz_b' bytes, and store in 'data'
 *
 * Number of bytes written is returned, or negative number on an error
 */
KS_API ks_ssize_t ksio_FileIO_readb(ksio_FileIO self, ks_ssize_t sz_b, void* data);

/* Read up to 'sz_c' characters (real number stored in '*num_c')
 *
 * Writes characters in UTF8 format to 'data', which should have been allocated for 'sz_c * 4' bytes
 * Number of bytes written is returned, or negative number on an error
 */
KS_API ks_ssize_t ksio_FileIO_reads(ksio_FileIO self, ks_ssize_t sz_c, void* data, ks_ssize_t* num_c);

/* Write 'sz_b' bytes of data to the file
 */
KS_API bool ksio_FileIO_writeb(ksio_FileIO self, ks_ssize_t sz_b, const void* data);

/* Write 'sz_b' of 'data' (which should be in UTF8 encoding) to the file
 */
KS_API bool ksio_FileIO_writes(ksio_FileIO self, ks_ssize_t sz_b, const void* data);


/* Create a new StringIO
 */
KS_API ksio_StringIO ksio_StringIO_new();

/* Get the current string
 * 'getf' also calls 'KS_DECREF(self)'
 */
KS_API ks_str ksio_StringIO_get(ksio_StringIO self);
KS_API ks_str ksio_StringIO_getf(ksio_StringIO self);


/* Create a new BytesIO
 */
KS_API ksio_BytesIO ksio_BytesIO_new();

/* Get the current contents
 * 'getf' also calls 'KS_DECREF(self)'
 */
KS_API ks_bytes ksio_BytesIO_get(ksio_BytesIO self);
KS_API ks_bytes ksio_BytesIO_getf(ksio_BytesIO self);

/* Adds a C-style printf-like formatting to the output
 *
 */
KS_API bool ksio_add(ksio_AnyIO self, const char* fmt, ...);
KS_API bool ksio_addv(ksio_AnyIO self, const char* fmt, va_list ap);

/* Add a buffer to an IO
 */
KS_API bool ksio_addbuf(ksio_AnyIO self, ks_ssize_t sz, const char* data);


/* Read entire file and return as a string. Returns NULL and throws an error if there was a problem
 */
KS_API ks_str ksio_readall(ks_str fname);


/* Types */
KS_API extern ks_type
    ksiot_FileIO,
    ksiot_StringIO,
    ksiot_BytesIO
;

#endif /* KSIO_H__ */
