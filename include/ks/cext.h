/* ks/cext.h - header to be included by modules/extensions written in C
 *
 * @author: Cade Brown <brown.cade@gmail.com>
 */

#pragma once
#ifndef KS_CEXT_H__
#define KS_CEXT_H__

#ifndef KS_CEXT_NAME
#error 'KS_CEXT_NAME' is not defined; when including `ks/cext.h`, you must define a extension name!
#endif

#include <ks/ks.h>


/* Macro to declare a C extension as being visible to the kscript module importer.
 *
 * Place this only once in a C extension, in the main 'module.c' file (recommended). Give it the argument
 *   of a function which returns the module when called (or NULL if there was an error)
 * 
 * So, you should define like:
 * 
 * ```
 * static ks_module get_module() {
 *    ...
 *    return res;
 * }
 * 
 * KS_CEXT_DECL(get_module);
 * ```
 *
 */
#ifdef __cplusplus
#define KS_CEXT_DECL(_load_func) struct ks_cextinit _KS_CEXTINIT_SYMBOL = { \
    _load_func \
}
#else
#define KS_CEXT_DECL(_load_func) struct ks_cextinit _KS_CEXTINIT_SYMBOL = (struct ks_cextinit) { \
    .load_func = _load_func \
}
#endif

/* Declare with 'extern' linkage so it can be read from 'dlopen()' or similar */
KS_API
#ifdef __cplusplus
extern "C"
#else
extern
#endif
    struct ks_cextinit _KS_CEXTINIT_SYMBOL;


#endif /* KS_CEXT_H__ */
