/* kpm/cext/Project.c - 'kpm.cext.Project' module
 *
 * 
 * @author:    Cade Brown <cade@kscript.org>
 */
#include <ks/impl.h>
#include <ks/kpm.h>

#define T_NAME "kpm.cext.Project"

/* C-API */


/* Type Functions */


static KS_TFUNC(T, init) {
    kpm_cext_project self;
    ks_str target;
    KS_ARGS("self:* target:*", &self, kpm_cextt_project, &target, kst_str);

    /* Add defaults */
    #define DEFA(_key, _val) do { \
        ks_str k = ks_str_new(-1, _key), v = ks_str_new(-1, _val); \
        ks_str rr = (ks_str)ksos_getenv(k, (kso)v); \
        KS_DECREF(v); \
        if (!rr) { \
            KS_DECREF(k); \
            return NULL; \
        } \
        ks_dict_set(self->attr, (kso)k, (kso)rr); \
        KS_DECREF(k); \
        KS_DECREF(rr); \
    } while (0)

    DEFA("CC",  "cc");
    DEFA("CXX", "c++");

    DEFA("LDFLAGS",  "");
    DEFA("CFLAGS",  "");
    DEFA("CXXFLAGS",  "");

    kso tmp = NULL;
    
    tmp = (kso)ks_list_new(0, NULL);
    ks_dict_set_c(self->attr, "DEFS", tmp);
    KS_DECREF(tmp);

    tmp = (kso)ks_list_new(0, NULL);
    ks_dict_set_c(self->attr, "SRC_C", tmp);
    KS_DECREF(tmp);

    ks_dict_set_c(self->attr, "target", (kso)target);

    return KSO_NONE;
}

static KS_TFUNC(T, cmd) {
    kpm_cext_project self;
    ks_str cmd;
    KS_ARGS("self:* cmd:*", &self, kpm_cextt_project, &cmd, kst_str);

    ksio_add(ksos_stderr, "$ %S\n", cmd);

    int rc = ksos_exec(cmd);
    if (rc < 0) {
        return NULL;
    } else if (rc != 0) {
        KS_THROW(kst_Error, "Command %R exited with non-zero code: %i", cmd, rc);
        return NULL;
    }

    return KSO_NONE;
}

static KS_TFUNC(T, build) {
    kpm_cext_project self;
    KS_ARGS("self:*", &self, kpm_cextt_project);

    /* Source file */
    ks_list SRC_C = (ks_list)ks_dict_get_c(self->attr, "SRC_C");
    if (!SRC_C) {
        return NULL;
    }
    ks_list SRC_C_O = ks_list_new(0, NULL);

    int i;
    for (i = 0; i < SRC_C->len; ++i) {
        ks_str out = ks_fmt("%S.o", SRC_C->elems[i]);
        if (!out) {
            KS_DECREF(SRC_C);
            KS_DECREF(SRC_C_O);
            return NULL;
        }

        ks_list_push(SRC_C_O, (kso)out);
        KS_DECREF(out);
    }

    ks_str k = ks_str_new(-1, "cmd");
    kso cmd = kso_getattr((kso)self, k);
    KS_DECREF(k);
    if (!cmd) {
        KS_DECREF(SRC_C);
        KS_DECREF(SRC_C_O);
        return NULL;
    }
    k = ks_str_new(-1, "rule_C");
    kso rule_C = kso_getattr((kso)self, k);
    KS_DECREF(k);
    if (!rule_C) {
        KS_DECREF(cmd);
        KS_DECREF(SRC_C);
        KS_DECREF(SRC_C_O);
        return NULL;
    }

    for (i = 0; i < SRC_C->len; ++i) {
        kso torun = kso_call(rule_C, 2, (kso[]) {
            SRC_C_O->elems[i],
            SRC_C->elems[i],
        });
        if (!torun) {
            KS_DECREF(cmd);
            KS_DECREF(rule_C);
            KS_DECREF(SRC_C);
            KS_DECREF(SRC_C_O);
            return NULL;
        }

        kso rt = kso_call(cmd, 1, (kso[]) { 
            torun
        });
        KS_DECREF(torun);
        if (!rt) {
            KS_DECREF(cmd);
            KS_DECREF(rule_C);
            KS_DECREF(SRC_C);
            KS_DECREF(SRC_C_O);
            return NULL;
        }

        KS_DECREF(rt);
    }
    KS_DECREF(rule_C);

    k = ks_str_new(-1, "link_C");
    kso link_C = kso_getattr((kso)self, k);
    KS_DECREF(k);
    if (!link_C) {
        KS_DECREF(cmd);
        KS_DECREF(SRC_C);
        KS_DECREF(SRC_C_O);
        return NULL;
    }

    k = ks_str_new(-1, "target");
    kso target = kso_getattr((kso)self, k);
    KS_DECREF(k);
    if (!target) {
        KS_DECREF(cmd);
        KS_DECREF(link_C);
        KS_DECREF(SRC_C);
        KS_DECREF(SRC_C_O);
        return NULL;
    }

    kso torun = kso_call(link_C, 2, (kso[]) {
        target,
        (kso)SRC_C_O
    });
    KS_DECREF(target);
    if (!torun) {
        KS_DECREF(cmd);
        KS_DECREF(link_C);
        KS_DECREF(SRC_C);
        KS_DECREF(SRC_C_O);
        return NULL;
    }

    kso rt = kso_call(cmd, 1, (kso[]) { 
        torun
    });
    KS_DECREF(torun);
    if (!rt) {
        KS_DECREF(cmd);
        KS_DECREF(link_C);
        KS_DECREF(SRC_C);
        KS_DECREF(SRC_C_O);
        return NULL;
    }

    KS_DECREF(rt);

    KS_DECREF(cmd);
    KS_DECREF(SRC_C);
    KS_DECREF(SRC_C_O);

    return KSO_NONE;
}


static KS_TFUNC(T, rule_C) {
    kpm_cext_project self;
    ks_str c_out, c_in;
    KS_ARGS("self:* c_out:* c_in:*", &self, kpm_cextt_project, &c_out, kst_str, &c_in, kst_str);

    ksio_StringIO sio = ksio_StringIO_new();
    kso CC = ks_dict_get_c(self->attr, "CC");
    if (!CC) {
        KS_DECREF(sio);
        return false;
    }

    if (!ksio_add(sio, "%S", CC)) {
        KS_DECREF(CC);
        KS_DECREF(sio);
        return NULL;
    } 
    KS_DECREF(CC);

    kso CFLAGS = ks_dict_get_c(self->attr, "CFLAGS");
    if (!CFLAGS) {
        KS_DECREF(sio);
        return NULL;
    }
    
    if (!ksio_add(sio, " %S", CFLAGS)) {
        KS_DECREF(CFLAGS);
        KS_DECREF(sio);
        return NULL;
    }
    KS_DECREF(CFLAGS);

    ks_list DEFS = (ks_list)ks_dict_get_c(self->attr, "DEFS");
    if (!DEFS) {
        KS_DECREF(sio);
        return false;
    }
    if (!kso_issub(DEFS->type, kst_list)) {
        KS_THROW(kst_TypeError, "Expected '.DEFS' to be a 'list', but was a '%T' object", DEFS);
        KS_DECREF(sio);
        KS_DECREF(DEFS);
        return false;
    }

    int i;
    for (i = 0; i < DEFS->len; ++i) {
        kso def = DEFS->elems[i];
        if (!ksio_add(sio, " -D%S", def)) {
            KS_DECREF(DEFS);
            KS_DECREF(sio);
            return false;
        }
    }

    KS_DECREF(DEFS);

    if (!ksio_add(sio, " -fPIC -c %S -o %S", c_in, c_out)) {
        KS_DECREF(sio);
        return NULL;
    }

    return (kso)ksio_StringIO_getf(sio);
}

static KS_TFUNC(T, link_C) {
    kpm_cext_project self;
    ks_str out;
    ks_list objs;
    bool isdyn = true;
    KS_ARGS("self:* out:* objs:* ?isdyn:bool", &self, kpm_cextt_project, &out, kst_str, &objs, kst_list, &isdyn);

    ksio_StringIO sio = ksio_StringIO_new();
    kso CC = ks_dict_get_c(self->attr, "CC");
    if (!CC) {
        KS_DECREF(sio);
        return NULL;
    }

    if (!ksio_add(sio, "%S", CC)) {
        KS_DECREF(CC);
        KS_DECREF(sio);
        return NULL;
    } 
    KS_DECREF(CC);


    kso LDFLAGS = ks_dict_get_c(self->attr, "LDFLAGS");
    if (!LDFLAGS) {
        KS_DECREF(sio);
        return NULL;
    }
    
    if (!ksio_add(sio, " %S", LDFLAGS)) {
        KS_DECREF(LDFLAGS);
        KS_DECREF(sio);
        return NULL;
    }
    KS_DECREF(LDFLAGS);

    if (!ksio_add(sio, " -shared")) {
        KS_DECREF(sio);
        return NULL;
    }

    int i;
    for (i = 0; i < objs->len; ++i) {
        if (!ksio_add(sio, " %S", objs->elems[i])) {
            KS_DECREF(sio);
            return NULL;
        }
    }

    if (!ksio_add(sio, " -o %S", out)) {
        KS_DECREF(sio);
        return NULL;
    }

    return (kso)ksio_StringIO_getf(sio);
}

/* Export */

static struct ks_type_s tp;
ks_type kpm_cextt_project = &tp;

void _ksi_kpm_cext_project() {

    _ksinit(kpm_cextt_project, kst_object, T_NAME, sizeof(struct kpm_cext_project_s), offsetof(struct kpm_cext_project_s, attr), "C-style extension manager and builder", KS_IKV(
        {"__init",                 ksf_wrap(T_init_, T_NAME ".__init(self, target)", "")},

        {"cmd",                    ksf_wrap(T_cmd_, T_NAME ".cmd(self, cmd)", "Executes a command 'cmd'")},

        {"build",                  ksf_wrap(T_build_, T_NAME ".build(self)", "")},

        {"rule_C",                 ksf_wrap(T_rule_C_, T_NAME ".rule_C(self, c_out, c_in)", "Returns a string for a compilation rule of a single C source to object file. Can be overriden by a subtype")},
        {"link_C",                 ksf_wrap(T_link_C_, T_NAME ".link_C(self, out, objs, isdyn=true)", "Returns a string command for linking 'objs' (a list of strings) into a single output 'out'. If 'isdyn', then a dynamic object is created. Otherwise, a static library is created")},

    ));

}
