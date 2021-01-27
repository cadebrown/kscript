/* norm_fro.c - 'norm_fro' kernel
 *
 * @author: Cade Brown <cade@kscript.org>
 */
#include <ks/impl.h>
#include <ks/nxi.h>
#include <ks/nxt.h>

#define KERN_FUNC(_name) NXK_PASTE(kern_, _name)

#define NXK_DO_F
#define NXK_DO_C
#define NXK_FILE "norm_fro.kern"
#define K_NAME "norm_fro"
#include <ks/nxk.h>

bool nxla_norm_fro(nx_t X, nx_t R) {

    /* Rpad = R[..., new, new], so it broadcasts with 'X' */
    nx_t Rpad = nx_newaxes(R, 2, (int[]){ R.rank, R.rank + 1 });

    if (false) {}
    #define LOOP(TYPE, NAME) else if (Rpad.dtype == nxd_##NAME) { \
        return !nx_apply_Nd(KERN_FUNC(NAME), 2, (nx_t[]){ X, Rpad }, 2, Rpad.dtype, NULL); \
    }
    NXT_PASTE_FC(LOOP)
    #undef LOOP

    KS_THROW(kst_TypeError, "Unsupported types for kernel '%s': %R, %R", K_NAME, X.dtype, R.dtype);
    return false;
}
