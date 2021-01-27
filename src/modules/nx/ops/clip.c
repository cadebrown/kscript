/* clip.c - 'clip' kernel
 *
 * @author: Cade Brown <cade@kscript.org>
 */
#include <ks/impl.h>
#include <ks/nxi.h>
#include <ks/nxt.h>

#define KERN_FUNC(_name) NXK_PASTE(kern_, _name)

#define NXK_DO_I
#define NXK_DO_F
#define NXK_FILE "clip.kern"
#define K_NAME "clip"
#include <ks/nxk.h>

bool nx_clip(nx_t X, nx_t Y, nx_t Z, nx_t R) {

    if (false) {}
    #define LOOP(TYPE, NAME) else if (R.dtype == nxd_##NAME) { \
        return !nx_apply_elem(KERN_FUNC(NAME), 4, (nx_t[]){ X, Y, Z, R }, R.dtype, NULL); \
    }
    NXT_PASTE_IF(LOOP)
    #undef LOOP

    KS_THROW(kst_TypeError, "Unsupported types for kernel '%s': %R, %R, %R, %R", K_NAME, X.dtype, Y.dtype, Z.dtype, R.dtype);
    return false;
}
