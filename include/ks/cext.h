/* ks/cext.h - header to be included by C-extension modules
 *
 * Example:
 * 
 * ```
 * #include <ks/cext.h>
 *  
 * // Get module
 * static ks_module get() {
 *   return ks_module_new(...);
 * }
 * KS_CEXT_DECL(get);
 * ```
 *
 * Then, compile that file to `ksm_<name>.so`, and it will be loadable via `import <name>`
 *
 * 
 * @author: Cade Brown <brown.cade@gmail.com>
 */

#pragma once
#ifndef KS_CEXT_H__
#define KS_CEXT_H__

#include <ks/ks.h>

/* Macro to declare a C extension as being visible to the kscript module importer.
 *
 * Place this only once in a C extension, in the main 'module.c' file (recommended). Give it the argument
 *   of a function which returns the module when called (or NULL if there was an error)
 * 
 * So, you should define like:
 * 
 * ```
 * static ks_module get() {
 *    ks_module res = ks_module_new(...);
 *    ...
 *    return res;
 * }
 * 
 * KS_CEXT_DECL(get);
 * ```
 *
 */
#define KS_CEXT_DECL(_loadfunc) struct ks_cextinit _KS_CEXTINIT_SYMBOL = { \
    _loadfunc \
}

/* Declare with 'extern' linkage so it can be read from 'dlopen()' or similar */
KS_API
#ifdef __cplusplus
extern "C"
#else
extern
#endif
    struct ks_cextinit _KS_CEXTINIT_SYMBOL;

#endif /* KS_CEXT_H__ */
