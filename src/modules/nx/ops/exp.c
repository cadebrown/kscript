/* exp.c - 'exp' kernel
 *
 * @author: Cade Brown <cade@kscript.org>
 */
#include <ks/impl.h>
#include <ks/nxi.h>
#include <ks/nxt.h>

#define KERN_FUNC(_name) NXK_PASTE(kern_, _name)

#define NXK_DO_F
#define NXK_DO_C
#define NXK_FILE "exp.kern"
#define K_NAME "exp"
#include <ks/nxk.h>

bool nx_exp(nx_t X, nx_t R) {

    if (false) {}
    #define LOOP(TYPE, NAME) else if (R.dtype == nxd_##NAME) { \
        return !nx_apply_elem(KERN_FUNC(NAME), 2, (nx_t[]){ X, R }, R.dtype, NULL); \
    }
    NXT_PASTE_FC(LOOP)
    #undef LOOP

    KS_THROW(kst_TypeError, "Unsupported types for kernel '%s': %R, %R", K_NAME, X.dtype, R.dtype);
    return false;
}
