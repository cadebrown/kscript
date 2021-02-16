/* kpm/main.c - 'kpm' module
 *
 * 
 * @author:    Cade Brown <cade@kscript.org>
 */
#include <ks/impl.h>
#include <ks/kpm.h>

#define M_NAME "kpm"

/* C-API */

ks_str kpm_get_path() {
    assert(ksm_kpm != NULL);

    return (ks_str)ks_dict_get_c(ksm_kpm->attr, "path");
}


void kpm_set_path(ks_str path) {
    assert(ksm_kpm != NULL);

    ks_dict_set_c(ksm_kpm->attr, "path", (kso)path);
}


/* Module functions */

static KS_TFUNC(M, exists) {
    ks_str name;
    KS_ARGS("name:*", &name, kst_str);
    
    /* Get installation path */
    ks_str path = kpm_get_path();
    if (!path) {
        return NULL;
    }

    /* Get directory where it should be stored */
    ks_str dir = ks_fmt("%S/%S", path, name);
    KS_DECREF(path);
    if (!dir) {
        return NULL;
    }

    /* Now, calculate whether it exists */
    bool res;
    if (!ksos_path_exists((kso)dir, &res)) {
        KS_DECREF(dir);
        return NULL;
    }

    KS_DECREF(dir);
    return KSO_BOOL(res);
}

/* Export */

ks_module ksm_kpm = NULL;

ks_module _ksi_kpm() {
    if (ksm_kpm) {
        KS_INCREF(ksm_kpm);
        return ksm_kpm;
    }

    ksm_kpm = ks_module_new(M_NAME, KS_BIMOD_SRC, "Kscript Package Manager (KPM)", KS_IKV(
        
        /* Submodules */
        {"cext",                   (kso)_ksi_kpm_cext()},

        /* Types */

        /* Aliases */

        /* Functions */

        {"exists",                 ksf_wrap(M_exists_, M_NAME ".exists(name)", "Whether the package 'name' exists")},

    ));


    /* Set the path to a reasonable default 
     * TODO: Read environment variable?
     */
    ks_str path = ks_fmt("%s/lib/ks-%i.%i.%i/kpm", KS_BUILD_PREFIX, KS_VERSION_MAJOR, KS_VERSION_MINOR, KS_VERSION_PATCH);
    kpm_set_path(path);
    KS_DECREF(path);

    return ksm_kpm;
}
