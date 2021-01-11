/* os/stat.c - 'os.stat' type
 * 
 * Attributes:
 *   os.stat.dev: Device ID on which the file was located
 *   os.stat.inode: inode number on disk
 *   os.stat.gid: Group ID of owner
 *   os.stat.uid: User ID of owner
 *   os.stat.size: Size of the file, in bytes
 * 
 * TODO: os.stat.dev is actually typically a composition of major and minor ID's.
 * For example, 'os.stat("README.md").dev == 2049', which actually should be decomposed as:
 *   minor = x & 0xFF
 *   major = (x >> 8) & 0xFF
 * 
 * Should we have attributes that are '.dev_major' and '.dev_minor' ?
 * SEE: https://stackoverflow.com/questions/4309882/device-number-in-stat-command-output
 * 
 * @author: Cade Brown <cade@kscript.org>
 */
#include <ks/impl.h>

#define T_NAME "os.stat"


/* C-API */

ksos_stat ksos_stat_wrap(struct ksos_cstat val) {
    ksos_stat self = KSO_NEW(ksos_stat, ksost_stat);

    self->val = val;

    return self;
}


/* Type Functions */

static KS_TFUNC(T, free) {
    ksos_stat self;
    KS_ARGS("self:*", &self, ksost_stat);

    KSO_DEL(self);

    return KSO_NONE;
}

static KS_TFUNC(T, init) {
    ksos_stat self;
    kso path;
    KS_ARGS("self:* path", &self, ksost_stat, &path);


    if (!ksos_pstat(&self->val, path)) return NULL;

    return KSO_NONE;
}

static KS_TFUNC(T, str) {
    ksos_stat self;
    KS_ARGS("self:*", &self, ksost_stat);

    /* Mode string in octal */
    char modestr[64];
    snprintf(modestr, sizeof(modestr) - 1, "0o%o", (int)self->val.val.st_mode);

    return (kso)ks_fmt("<%T dev=%u, inode=%u, gid=%u, uid=%u, size=%u, mode=%s>", self, 
        (ks_uint)self->val.val.st_dev, 
        (ks_uint)self->val.val.st_ino,
        (ks_uint)self->val.val.st_gid,
        (ks_uint)self->val.val.st_uid,
        (ks_uint)self->val.val.st_size,
        modestr
    );
}

static KS_TFUNC(T, getattr) {
    ksos_stat self;
    ks_str attr;
    KS_ARGS("self:* attr:*", &self, ksost_stat, &attr, kst_str);

    if (ks_str_eq_c(attr, "dev", 3)) {
        return (kso)ks_int_new(self->val.val.st_dev);
    } else if (ks_str_eq_c(attr, "inode", 5)) {
        return (kso)ks_int_new(self->val.val.st_ino);
    } else if (ks_str_eq_c(attr, "gid", 3)) {
        return (kso)ks_int_new(self->val.val.st_gid);
    } else if (ks_str_eq_c(attr, "uid", 3)) {
        return (kso)ks_int_new(self->val.val.st_uid);
    } else if (ks_str_eq_c(attr, "size", 4)) {
        return (kso)ks_int_new(self->val.val.st_size);
    } else if (ks_str_eq_c(attr, "mode", 4)) {
        return (kso)ks_int_new(self->val.val.st_mode);
    }

    KS_THROW_ATTR(self, attr);
    return NULL;
}

/* Export */

static struct ks_type_s tp;
ks_type ksost_stat = &tp;

void _ksi_os_stat() {
    _ksinit(ksost_stat, kst_object, T_NAME, sizeof(struct ksos_stat_s), -1, "Information about a file on disk, or an open file descriptor. Callable as a function, which performs the query on a given path (like 'os.stat(path)')", KS_IKV(
        {"__free",                 ksf_wrap(T_free_, T_NAME ".__free(self)", "")},
        {"__init",                 ksf_wrap(T_init_, T_NAME ".__init(self, path)", "")},
        {"__str",                  ksf_wrap(T_str_, T_NAME ".__str(self)", "")},
        {"__repr",                 ksf_wrap(T_str_, T_NAME ".__repr(self)", "")},

        {"__getattr",              ksf_wrap(T_getattr_, T_NAME ".__getattr(self, attr)", "")},
    ));

}
