/* types/module.c - 'module' type
 *
 *
 * @author: Cade Brown <cade@kscript.org>
 */
#include <ks/impl.h>

#define T_NAME "module"



/* C-API */

ks_module ks_module_new(const char* name, const char* source, const char* doc, struct ks_ikv* ikv) {
    ks_module self = KSO_NEW(ks_module, kst_module);


    return self;
}




/* Export */

static struct ks_type_s tp;
ks_type kst_module = &tp;

void _ksi_module() {
    _ksinit(kst_module, kst_object, T_NAME, sizeof(*(ks_module)NULL), -1, NULL);
}
