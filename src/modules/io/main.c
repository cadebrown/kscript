/* io/main.c - 'io' module
 *
 * 
 * @author:    Cade Brown <cade@kscript.org>
 */
#include <ks/impl.h>

#define M_NAME "io"

/* C-API */


/* Module functions */

static KS_TFUNC(M, fdopen) {
    ks_cint fd;
    ks_str mode = _ksv_r;
    ks_str src = NULL;
    KS_ARGS("fd:cint ?mode:* ?src:*", &fd, &mode, kst_str, &src, kst_str);

    if (src) {
        KS_INCREF(src);
    } else {
        src = ks_fmt("<fd:%i>", (int)fd);
    }
    ksio_FileIO res = ksio_FileIO_fdopen(fd, src, mode);
    KS_DECREF(src);
    return (kso)res;
}

/* Export */
ks_type
    ksioe_Seek
;

ks_module _ksi_io() {
    _ksi_io_BaseIO();
    _ksi_io_RawIO();
    _ksi_io_FileIO();
    _ksi_io_StringIO();
    _ksi_io_BytesIO();

    ksioe_Seek = ks_enum_make(M_NAME ".Seek", KS_EIKV(
        {"SET", KSIO_SEEK_SET},
        {"CUR", KSIO_SEEK_CUR},
        {"END", KSIO_SEEK_END},
    ));

    ks_module res = ks_module_new(M_NAME, KS_BIMOD_SRC, "Input/output utilities", KS_IKV(
        /* Enums */
        {"Seek",                   KS_NEWREF(ksioe_Seek)},

        /* Types */
        {"BaseIO",                 KS_NEWREF(ksiot_BaseIO)},
        {"RawIO",                  KS_NEWREF(ksiot_RawIO)},
        {"FileIO",                 KS_NEWREF(ksiot_FileIO)},
        {"StringIO",               KS_NEWREF(ksiot_StringIO)},
        {"BytesIO",                KS_NEWREF(ksiot_BytesIO)},

        /* Functions */
        {"fdopen",                 ksf_wrap(M_fdopen_, M_NAME ".fdopen(fd, mode='r', src=none)", "Open an integer file descriptor as a buffered FileIO")},

    ));

    
    return res;
}
