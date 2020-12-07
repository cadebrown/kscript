/* types/names.c - 'names' type
 *
 * @author: Cade Brown <cade@kscript.org>
 */
#include <ks/impl.h>

#define T_NAME "names"


/* C-API */


ks_names ks_names_new(ks_dict of, bool copy) {
    ks_names self = KSO_NEW(ks_names, kst_names);

    ks_dict_merge(self->attr, of);

    return self;
}


/* Export */

static struct ks_type_s tp;
ks_type kst_names = &tp;

void _ksi_names() {
    _ksinit(kst_names, kst_object, T_NAME, sizeof(struct ks_names_s), offsetof(struct ks_names_s, attr), "Names (or namespaces) are like dictionaries, but elements are accessed as attributes rather than with '[]' subscripts\n\n    For example, with a dictionary you might have 'd[\"a\"]', but a 'names' object would use 'n.a'", KS_IKV(

    ));
    
}
