/* zero.c - 'zero' kernel
 *
 * 
 * @author:    Cade Brown <cade@kscript.org>
 */
#include <ks/impl.h>
#include <ks/nxt.h>

#define K_NAME "zero"



/* Access Macros */

#define LOOPR(TYPE, NAME) static int kern_##NAME(int N, nx_t* args, int len, void* extra) { \
    assert(N == 1); \
    nx_t R = args[0]; \
    ks_uint \
        pR = (ks_uint)R.data  \
    ; \
    ks_cint \
        sR = R.strides[0]  \
    ; \
    ks_cint i; \
    for (i = 0; i < len; ++i, pR += sR) { \
        *(TYPE*)pR = 0; \
    } \
    return 0; \
}

#define LOOPC(TYPE, NAME) static int kern_##NAME(int N, nx_t* args, int len, void* extra) { \
    assert(N == 1); \
    nx_t R = args[0]; \
    ks_uint \
        pR = (ks_uint)R.data  \
    ; \
    ks_cint \
        sR = R.strides[0]  \
    ; \
    ks_cint i; \
    for (i = 0; i < len; ++i, pR += sR) { \
        ((TYPE*)pR)->re = 0; \
        ((TYPE*)pR)->im = 0; \
    } \
    return 0; \
}



NXT_PASTE_I(LOOPR);
NXT_PASTE_F(LOOPR);

NXT_PASTE_C(LOOPC);

bool nx_zero(nx_t R) {
    #define LOOP(NAME) do { \
        bool res = !nx_apply_elem(kern_##NAME, 1, (nx_t[]){ R }, NULL); \
        return res; \
    } while (0);

    NXT_PASTE_ALL(R.dtype, LOOP);
    #undef LOOP

    KS_THROW(kst_TypeError, "Unsupported types for kernel '%s': %R", K_NAME, R.dtype);
    return false;
}
