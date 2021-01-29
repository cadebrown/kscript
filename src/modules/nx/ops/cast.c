/* cast.c - 'cast' kernel
 *
 * @author: Cade Brown <cade@kscript.org>
 */
#include <ks/impl.h>
#include <ks/nxi.h>
#include <ks/nxt.h>

#define K_NAME "cast"

#define PASTE3(X, Y, Z) NXK_PASTE(NXK_PASTE(X, Y), Z)
#define KERN_FUNC(_name) PASTE3(kern_, RTYPE_NAME, _name)
#define RTYPE NXK_PASTE(nx_, RTYPE_NAME)


#define RTYPE_NAME bl
#define NXK_DO_I
#define NXK_DO_F
#define NXK_DO_C
#define NXK_FILE "cast.B.kern"
#include <ks/nxk.h>
#undef RTYPE_NAME
#undef NXK_FILE


#define RTYPE_NAME s8
#define NXK_DO_I
#define NXK_DO_F
#define NXK_DO_C
#define NXK_FILE "cast.R.kern"
#include <ks/nxk.h>
#undef RTYPE_NAME
#undef NXK_FILE
#define RTYPE_NAME s16
#define NXK_DO_I
#define NXK_DO_F
#define NXK_DO_C
#define NXK_FILE "cast.R.kern"
#include <ks/nxk.h>
#undef RTYPE_NAME
#undef NXK_FILE
#define RTYPE_NAME s32
#define NXK_DO_I
#define NXK_DO_F
#define NXK_DO_C
#define NXK_FILE "cast.R.kern"
#include <ks/nxk.h>
#undef RTYPE_NAME
#undef NXK_FILE
#define RTYPE_NAME s64
#define NXK_DO_I
#define NXK_DO_F
#define NXK_DO_C
#define NXK_FILE "cast.R.kern"
#include <ks/nxk.h>
#undef RTYPE_NAME
#undef NXK_FILE


#define RTYPE_NAME u8
#define NXK_DO_I
#define NXK_DO_F
#define NXK_DO_C
#define NXK_FILE "cast.R.kern"
#include <ks/nxk.h>
#undef RTYPE_NAME
#undef NXK_FILE
#define RTYPE_NAME u16
#define NXK_DO_I
#define NXK_DO_F
#define NXK_DO_C
#define NXK_FILE "cast.R.kern"
#include <ks/nxk.h>
#undef RTYPE_NAME
#undef NXK_FILE
#define RTYPE_NAME u32
#define NXK_DO_I
#define NXK_DO_F
#define NXK_DO_C
#define NXK_FILE "cast.R.kern"
#include <ks/nxk.h>
#undef RTYPE_NAME
#undef NXK_FILE
#define RTYPE_NAME u64
#define NXK_DO_I
#define NXK_DO_F
#define NXK_DO_C
#define NXK_FILE "cast.R.kern"
#include <ks/nxk.h>
#undef RTYPE_NAME
#undef NXK_FILE


#define RTYPE_NAME F
#define NXK_DO_I
#define NXK_DO_F
#define NXK_DO_C
#define NXK_FILE "cast.R.kern"
#include <ks/nxk.h>
#undef RTYPE_NAME
#undef NXK_FILE

#define RTYPE_NAME D
#define NXK_DO_I
#define NXK_DO_F
#define NXK_DO_C
#define NXK_FILE "cast.R.kern"
#include <ks/nxk.h>
#undef RTYPE_NAME
#undef NXK_FILE

#define RTYPE_NAME E
#define NXK_DO_I
#define NXK_DO_F
#define NXK_DO_C
#define NXK_FILE "cast.R.kern"
#include <ks/nxk.h>
#undef RTYPE_NAME
#undef NXK_FILE

#define RTYPE_NAME Q
#define NXK_DO_I
#define NXK_DO_F
#define NXK_DO_C
#define NXK_FILE "cast.R.kern"
#include <ks/nxk.h>
#undef RTYPE_NAME
#undef NXK_FILE


#define RTYPE_NAME cF
#define NXK_DO_I
#define NXK_DO_F
#define NXK_DO_C
#define NXK_FILE "cast.C.kern"
#include <ks/nxk.h>
#undef RTYPE_NAME
#undef NXK_FILE

#define RTYPE_NAME cD
#define NXK_DO_I
#define NXK_DO_F
#define NXK_DO_C
#define NXK_FILE "cast.C.kern"
#include <ks/nxk.h>
#undef RTYPE_NAME
#undef NXK_FILE

#define RTYPE_NAME cE
#define NXK_DO_I
#define NXK_DO_F
#define NXK_DO_C
#define NXK_FILE "cast.C.kern"
#include <ks/nxk.h>
#undef RTYPE_NAME
#undef NXK_FILE

#define RTYPE_NAME cQ
#define NXK_DO_I
#define NXK_DO_F
#define NXK_DO_C
#define NXK_FILE "cast.C.kern"
#include <ks/nxk.h>
#undef RTYPE_NAME
#undef NXK_FILE


bool nx_cast(nx_t X, nx_t R) {

    if (false) {}

    else if (R.dtype == nxd_bl) {
        if (false) {}
        #define LOOP(TYPE, NAME) else if (X.dtype == nxd_##NAME) { \
            return !nx_apply_elem(PASTE3(kern_, bl, NAME), 2, (nx_t[]){ X, R }, NULL, NULL); \
        }
        NXT_PASTE_IFC(LOOP)
        #undef LOOP
    }
    else if (R.dtype == nxd_s8) {
        if (false) {}
        #define LOOP(TYPE, NAME) else if (X.dtype == nxd_##NAME) { \
            return !nx_apply_elem(PASTE3(kern_, s8, NAME), 2, (nx_t[]){ X, R }, NULL, NULL); \
        }
        NXT_PASTE_IFC(LOOP)
        #undef LOOP
    }
    else if (R.dtype == nxd_s16) {
        if (false) {}
        #define LOOP(TYPE, NAME) else if (X.dtype == nxd_##NAME) { \
            return !nx_apply_elem(PASTE3(kern_, s16, NAME), 2, (nx_t[]){ X, R }, NULL, NULL); \
        }
        NXT_PASTE_IFC(LOOP)
        #undef LOOP
    }
    else if (R.dtype == nxd_s32) {
        if (false) {}
        #define LOOP(TYPE, NAME) else if (X.dtype == nxd_##NAME) { \
            return !nx_apply_elem(PASTE3(kern_, s32, NAME), 2, (nx_t[]){ X, R }, NULL, NULL); \
        }
        NXT_PASTE_IFC(LOOP)
        #undef LOOP
    }
    else if (R.dtype == nxd_s64) {
        if (false) {}
        #define LOOP(TYPE, NAME) else if (X.dtype == nxd_##NAME) { \
            return !nx_apply_elem(PASTE3(kern_, s64, NAME), 2, (nx_t[]){ X, R }, NULL, NULL); \
        }
        NXT_PASTE_IFC(LOOP)
        #undef LOOP
    }
    else if (R.dtype == nxd_u8) {
        if (false) {}
        #define LOOP(TYPE, NAME) else if (X.dtype == nxd_##NAME) { \
            return !nx_apply_elem(PASTE3(kern_, u8, NAME), 2, (nx_t[]){ X, R }, NULL, NULL); \
        }
        NXT_PASTE_IFC(LOOP)
        #undef LOOP
    }
    else if (R.dtype == nxd_u16) {
        if (false) {}
        #define LOOP(TYPE, NAME) else if (X.dtype == nxd_##NAME) { \
            return !nx_apply_elem(PASTE3(kern_, u16, NAME), 2, (nx_t[]){ X, R }, NULL, NULL); \
        }
        NXT_PASTE_IFC(LOOP)
        #undef LOOP
    }
    else if (R.dtype == nxd_u32) {
        if (false) {}
        #define LOOP(TYPE, NAME) else if (X.dtype == nxd_##NAME) { \
            return !nx_apply_elem(PASTE3(kern_, u32, NAME), 2, (nx_t[]){ X, R }, NULL, NULL); \
        }
        NXT_PASTE_IFC(LOOP)
        #undef LOOP
    }
    else if (R.dtype == nxd_u64) {
        if (false) {}
        #define LOOP(TYPE, NAME) else if (X.dtype == nxd_##NAME) { \
            return !nx_apply_elem(PASTE3(kern_, u64, NAME), 2, (nx_t[]){ X, R }, NULL, NULL); \
        }
        NXT_PASTE_IFC(LOOP)
        #undef LOOP
    }

    else if (R.dtype == nxd_F) {
        if (false) {}
        #define LOOP(TYPE, NAME) else if (X.dtype == nxd_##NAME) { \
            return !nx_apply_elem(PASTE3(kern_, F, NAME), 2, (nx_t[]){ X, R }, NULL, NULL); \
        }
        NXT_PASTE_IFC(LOOP)
        #undef LOOP
    }
    else if (R.dtype == nxd_D) {
        if (false) {}
        #define LOOP(TYPE, NAME) else if (X.dtype == nxd_##NAME) { \
            return !nx_apply_elem(PASTE3(kern_, D, NAME), 2, (nx_t[]){ X, R }, NULL, NULL); \
        }
        NXT_PASTE_IFC(LOOP)
        #undef LOOP
    }
    else if (R.dtype == nxd_E) {
        if (false) {}
        #define LOOP(TYPE, NAME) else if (X.dtype == nxd_##NAME) { \
            return !nx_apply_elem(PASTE3(kern_, E, NAME), 2, (nx_t[]){ X, R }, NULL, NULL); \
        }
        NXT_PASTE_IFC(LOOP)
        #undef LOOP
    }
    else if (R.dtype == nxd_Q) {
        if (false) {}
        #define LOOP(TYPE, NAME) else if (X.dtype == nxd_##NAME) { \
            return !nx_apply_elem(PASTE3(kern_, Q, NAME), 2, (nx_t[]){ X, R }, NULL, NULL); \
        }
        NXT_PASTE_IFC(LOOP)
        #undef LOOP
    }

    else if (R.dtype == nxd_cF) {
        if (false) {}
        #define LOOP(TYPE, NAME) else if (X.dtype == nxd_##NAME) { \
            return !nx_apply_elem(PASTE3(kern_, cF, NAME), 2, (nx_t[]){ X, R }, NULL, NULL); \
        }
        NXT_PASTE_IFC(LOOP)
        #undef LOOP
    }
    else if (R.dtype == nxd_cD) {
        if (false) {}
        #define LOOP(TYPE, NAME) else if (X.dtype == nxd_##NAME) { \
            return !nx_apply_elem(PASTE3(kern_, cD, NAME), 2, (nx_t[]){ X, R }, NULL, NULL); \
        }
        NXT_PASTE_IFC(LOOP)
        #undef LOOP
    }
    else if (R.dtype == nxd_cE) {
        if (false) {}
        #define LOOP(TYPE, NAME) else if (X.dtype == nxd_##NAME) { \
            return !nx_apply_elem(PASTE3(kern_, cE, NAME), 2, (nx_t[]){ X, R }, NULL, NULL); \
        }
        NXT_PASTE_IFC(LOOP)
        #undef LOOP
    }
    else if (R.dtype == nxd_cQ) {
        if (false) {}
        #define LOOP(TYPE, NAME) else if (X.dtype == nxd_##NAME) { \
            return !nx_apply_elem(PASTE3(kern_, cQ, NAME), 2, (nx_t[]){ X, R }, NULL, NULL); \
        }
        NXT_PASTE_IFC(LOOP)
        #undef LOOP
    }
    KS_THROW(kst_TypeError, "Unsupported types for kernel '%s': %R, %R", K_NAME, X.dtype, R.dtype);
    return false;
}
