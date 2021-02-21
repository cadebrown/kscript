/* pad.c - 'pad' kernel
 *
 * @author: Cade Brown <cade@kscript.org>
 */
#include <ks/impl.h>
#include <ks/nxi.h>
#include <ks/nxt.h>


bool nx_pad(nx_t X, nx_t R) {
    if (X.rank != R.rank) {
        KS_THROW(kst_SizeError, "Unsupported ranks for kernel 'pad': %i, %i (must be the same)", X.rank, R.rank);
        return false;
    }
    if (!nx_zero(R)) {
        return false;
    }

    /* Pad 'R' to the same size as 'X' */
    nx_t Xpad = X, Rpad = R;

    int i;
    for (i = 0; i < X.rank; ++i) {
        if (R.shape[i] > X.shape[i]) {
            Rpad.shape[i] = X.shape[i];
        } else if (R.shape[i] < X.shape[i]) {
            Xpad.shape[i] = R.shape[i];
        }
    }

    return nx_cast(Xpad, Rpad);
}
