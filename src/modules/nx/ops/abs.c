/* abs.c - 'abs' kernel
 *
 * @author: Cade Brown <cade@kscript.org>
 */
#include <ks/impl.h>
#include <ks/nxi.h>
#include <ks/nxt.h>

#define KERN_FUNC(_name) NXK_PASTE(kern_, _name)

#define NXK_DO_I
#define NXK_DO_F
#define NXK_DO_C
#define NXK_FILE "abs.kern"
#define K_NAME "abs"
#include <ks/nxk.h>

bool nx_abs(nx_t X, nx_t R) {

    if (false) {}
    #define LOOP(TYPE, NAME) else if (X.dtype == nxd_##NAME) { \
        return !nx_apply_elem(KERN_FUNC(NAME), 2, (nx_t[]){ X, R }, NULL, NULL); \
    }
    NXT_PASTE_IFC(LOOP)
    #undef LOOP

    KS_THROW(kst_TypeError, "Unsupported types for kernel '%s': %R, %R", K_NAME, X.dtype, R.dtype);
    return false;
}
