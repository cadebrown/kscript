/* apply.c - vectorized function application
 * 
 *
 * @author:    Cade Brown <cade@kscript.org>
 */
#include <ks/impl.h>
#include <ks/nx.h>


/* Internally used as addresses */
static ks_size_t my_sz0 = 0, my_sz1 = 1;

/* Internal apply function */
static int I_apply_elem(nxf_elem func, nx_t ebc, int N, nx_t* args, nx_t* args_1D, ks_size_t* idxs, void* extra) {
    ks_cint i;
    ks_ssize_t sz0 = 0;
    int loop_N = args[0].rank - 1;

    /* Zero out indices */
    for (i = 0; i < loop_N; ++i) idxs[i] = 0;

    while (true) {

        /* Convert to 1D slices */
        for (i = 0; i < N; ++i) {
            args_1D[i] = args[i];

            args_1D[i].rank = 1;
            if (args[i].rank == 0) {
                /* Element tensor */
                args_1D[i].shape[0] = 1;
                args_1D[i].strides[0] = 0;
                args_1D[i].data = args[i].data;

            } else {
                /* 1D tensor */
                args_1D[i].shape[0] = args[i].shape[args[i].rank - 1];
                args_1D[i].strides[0] = args_1D[i].shape[0] == 1 ? 0 : args[i].strides[args[i].rank - 1];
                args_1D[i].data = nx_szdot(args[i].data, loop_N, args[i].strides, idxs);
            }
        }

        /* Apply to 1D slice */
        int res = func(N, args_1D, args[0].rank == 0 ? 1 : ebc.shape[args[0].rank - 1], extra);
        if (res) return res;

        /* Increase least significant index */
        i = loop_N - 1;
        if (i < 0) break;
        idxs[i]++;

        while (i >= 0 && idxs[i] >= ebc.shape[i]) {
            idxs[i] = 0;
            i--;
            if (i >= 0) idxs[i]++;
        }

        /* Done; overflowed */
        if (i < 0) break;
    }

    return 0;
}


int nx_apply_elem(nxf_elem func, int N, nx_t* args, void* extra) {
    assert(N > 0);
    assert(N <= NX_MAXBCS);

    /* Calculate a broadcast size */
    nx_t ebc = nx_make_bcast(N, args);
    if (ebc.rank < 0) {
        return -1;
    }

    /* Loop indices */
    ks_size_t idxs[NX_MAXRANK];

    /* Create new arrays with paded dimensions to the maximum rank */
    nx_t new_args[NX_MAXBCS], new_args_1D[NX_MAXBCS];

    int i, j, jr;
    for (i = 0; i < N; ++i) {
        new_args[i] = args[i];

        /* Pad to maximum rank */
        new_args[i].rank = ebc.rank;

        /* Dimensions and strides that are not filled in should act like shape==1, stride==0 */
        for (j = 0; j < ebc.rank - args[i].rank; ++j) {
            new_args[i].shape[j] = 1;
            new_args[i].strides[j] = 0;
        }

        /* Copy the rest, aligned to the right */
        for (jr = 0; j < ebc.rank; ++j, ++jr) {
            new_args[i].shape[j] = args[i].shape[jr];
            new_args[i].strides[j] = new_args[i].shape[j] == 1 ? 0 : args[i].strides[jr];
        }
    }


    int res = I_apply_elem(func, ebc, N, new_args, new_args_1D, idxs, extra);
    return res;
}

