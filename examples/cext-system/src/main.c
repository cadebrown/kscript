/* main.c - main implementation of the 'system' package, a simple example package
 *
 * @author: Cade Brown <cade@kscript.org>
 */

#include <ks/ks.h>
#include <ks/cext.h>

#define M_NAME "system"

/* Module functions */

static KS_TFUNC(M, system) {
    ks_str cmd;
    KS_ARGS("cmd:*", &cmd, kst_str);

    int res = system(cmd->data);
    if (res < 0) {
        KS_THROW_ERRNO(errno, "Executing command failed");
        return NULL;
    }

    return (kso)ks_int_new(res);
}

/* Export */

static ks_module get() {

    /* Construct a module  */
    ks_module res = ks_module_new(M_NAME, "", "This module provides access to the system shell, via the 'system()' function in C", KS_IKV(
        {"system",                 ksf_wrap(M_system_, M_NAME ".system(cmd)", "Execute a command, and return the return code from it")},
    ));

    return res;
}

/* Declares this as a C-style extension */
KS_CEXT_DECL(get);
