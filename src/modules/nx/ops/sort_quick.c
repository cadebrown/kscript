/* sort_quick.c - 'sort_quick' kernel
 *
 * @author: Cade Brown <cade@kscript.org>
 */
#include <ks/impl.h>
#include <ks/nxi.h>
#include <ks/nxt.h>

#define KERN_FUNC(_name) NXK_PASTE(kern_, _name)

#define NXK_DO_I
#define NXK_DO_F
#define NXK_FILE "sort_quick.kern"
#define K_NAME "sort_quick"
#include <ks/nxk.h>

bool nx_sort_quick(nx_t X, int axis) {
    assert(axis >= 0 && axis < X.rank);

    /* Sink so the sorting axis is the last */
    nx_t sX = nx_sinkaxes(X, 1, &axis);

    if (false) {}
    #define LOOP(TYPE, NAME) else if (X.dtype == nxd_##NAME) { \
        bool res = !nx_apply_Nd(KERN_FUNC(NAME), 1, (nx_t[]){ sX }, 1, NULL, NULL); \
        return res; \
    }
    NXT_PASTE_IF(LOOP)
    #undef LOOP

    KS_THROW(kst_TypeError, "Unsupported types for kernel '%s': %R", K_NAME, X.dtype);
    return false;
}
