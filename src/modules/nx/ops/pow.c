/* pow.c - 'pow' kernel
 *
 * @author: Cade Brown <cade@kscript.org>
 */
#include <ks/impl.h>
#include <ks/nxi.h>
#include <ks/nxt.h>

#define KERN_FUNC(_name) NXK_PASTE(kern_, _name)

#define NXK_DO_F
#define NXK_DO_C
#define NXK_FILE "pow.kern"
#define K_NAME "pow"
#include <ks/nxk.h>

bool nx_pow(nx_t X, nx_t Y, nx_t R) {

    if (false) {}
    #define LOOP(TYPE, NAME) else if (R.dtype == nxd_##NAME) { \
        return !nx_apply_elem(KERN_FUNC(NAME), 3, (nx_t[]){ X, Y, R }, R.dtype, NULL); \
    }
    NXT_PASTE_FC(LOOP)
    #undef LOOP

    KS_THROW(kst_TypeError, "Unsupported types for kernel '%s': %R, %R, %R", K_NAME, X.dtype, Y.dtype, R.dtype);
    return false;
}
