/* factlu.c - 'factlu' kernel
 *
 * @author: Cade Brown <cade@kscript.org>
 */
#include <ks/impl.h>
#include <ks/nxi.h>
#include <ks/nxt.h>

#define KERN_FUNC(_name) NXK_PASTE(kern_, _name)


struct extra_data {
    
    /* Size of the transform */
    int M, N, K;

    /* Strides for the inputs */
    ks_ssize_t srX, scX;
    ks_ssize_t srY, scY;
    ks_ssize_t srR, scR;

};


#define NXK_DO_I
#define NXK_DO_F
//#define NXK_DO_C
#define NXK_FILE "factlu.kern"
#define K_NAME "factlu"
#include <ks/nxk.h>

bool nxla_factlu(nx_t X, nx_t P, nx_t L, nx_t U) {

    if (X.rank < 2 || P.rank < 1 || L.rank < 2 || U.rank < 2) {
        KS_THROW(kst_SizeError, "Unsupported ranks for kernel '%s': %i, %i, %i, %i (expected matrices to have rank >= 2, and vector to have rank >= 1)", K_NAME, X.rank, P.rank, L.rank, U.rank);
        return false;
    }

    /* U = 0, L = X */
    if (!nx_zero(U) || !nx_cast(X, L)) {
        return false;
    }

    /* Since P is a vector, add a rank */
    P = nx_newaxis(P, P.rank - 1);

    if (false) {}
    #define LOOP(TYPE, NAME) else if (L.dtype == nxd_##NAME && U.dtype == nxd_##NAME) { \
        return !nx_apply_Nd(KERN_FUNC(NAME), 3, (nx_t[]){ P, L, U }, 2, NULL, NULL); \
    }
    NXT_PASTE_IF(LOOP)
    #undef LOOP

    KS_THROW(kst_TypeError, "Unsupported types for kernel '%s': %R, %R, %R, %R", K_NAME, X.dtype, P.dtype, L.dtype, U.dtype);
    return false;
}
