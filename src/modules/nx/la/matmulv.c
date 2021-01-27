/* matmulv.c - 'matmulv' kernel
 *
 * @author: Cade Brown <cade@kscript.org>
 */
#include <ks/impl.h>
#include <ks/nxi.h>
#include <ks/nxt.h>


bool nxla_matmulv(nx_t X, nx_t Y, nx_t R) {
    /* TODO: Check BLAS for optimized routines? */
    return nxla_matmul(X, nx_newaxis(Y, Y.rank), nx_newaxis(R, R.rank));
}
