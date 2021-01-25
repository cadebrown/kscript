/* apply.c - vectorized function application
 * 
 *
 * @author:    Cade Brown <cade@kscript.org>
 */
#include <ks/impl.h>
#include <ks/nx.h>


/* Internal apply function */
static int I_apply_elem(nxf_elem func, nx_t ebc, int N, nx_t* kargs, nx_t* kargsS, ks_size_t* idxs, void* extra) {
    ks_cint i, j;

    /* We are looping over all the dimensions not being forwarded to the caller */
    int loop_N = ebc.rank - 1;

    /* Zero out indices */
    for (i = 0; i < loop_N; ++i) idxs[i] = 0;

    while (true) {

        /* Set data pointers for the slices (strides & dimensions stay the same) */
        for (i = 0; i < N; ++i) {
            kargsS[i].data = nx_szdot(kargs[i].data, loop_N, kargs[i].strides, idxs);
        }

        /* Apply to 1D slice */
        int res = func(N, kargsS, ebc.rank == 0 ? 1 : ebc.shape[ebc.rank - 1], extra);
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

    if (ebc.rank == 0) ebc = nx_with_newaxis(ebc, 0);

    /* Loop indices */
    ks_size_t idxs[NX_MAXRANK];

    /* Create new arguments arrays, one fully padded, and the other that is a single slice
     *   of the other one, given to 'func'
     */
    nx_t kargs[NX_MAXBCS], kargsS[NX_MAXBCS];

    int i, j, jr;
    for (i = 0; i < N; ++i) {
        /* 'kargs[i]' gives the entire 'i'th input broadcasted to the maximum rank size */
        kargs[i] = args[i];
        kargs[i].rank = ebc.rank;

        /* Extend dimensions to the left */
        for (j = 0; j < ebc.rank - args[i].rank; ++j) {
            kargs[i].shape[j] = 1;
            kargs[i].strides[j] = 0;
        }

        /* Copy the rest, aligned to the right */
        for (jr = 0; j < kargs[i].rank && jr < args[i].rank; ++j, ++jr) {
            kargs[i].shape[j] = args[i].shape[jr];
            kargs[i].strides[j] = kargs[i].shape[j] == 1 ? 0 : args[i].strides[jr];
        }

        /* 'kargsS[i]' gives the slice of the 'i'th input broadcasted to the desired size ('M') */
        kargsS[i] = kargs[i];

        /* Pad to maximum rank */
        kargsS[i].rank = 1;

        /* We should copy from the right of 'kargs[i]', since it will be a right-most slice */
        for (j = 0; j < kargsS[i].rank; ++j) {
            kargsS[i].shape[j] = kargs[i].shape[j + kargs[i].rank - 1];
            kargsS[i].strides[j] = kargs[i].strides[j + kargs[i].rank - 1];
        }
    }

    int res = I_apply_elem(func, ebc, N, kargs, kargsS, idxs, extra);
    return res;
}

/* Internal apply function */
static int I_apply_Nd(nxf_Nd func, nx_t ebc, int N, nx_t* kargs, nx_t* kargsS, int M, ks_size_t* idxs, void* extra) {
    ks_cint i, j;

    /* We are looping over all the dimensions not being forwarded to the caller */
    int loop_N = ebc.rank - M;

    /* Zero out indices */
    for (i = 0; i < loop_N; ++i) idxs[i] = 0;

    while (true) {

        /* Set data pointers for the slices (strides & dimensions stay the same) */
        for (i = 0; i < N; ++i) {
            kargsS[i].data = nx_szdot(kargs[i].data, loop_N, kargs[i].strides, idxs);
        }

        /* Apply to 1D slice */
        int res = func(N, kargsS, extra);
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

int nx_apply_Nd(nxf_Nd func, int N, nx_t* args, int M, void* extra) {
    assert(N > 0);
    assert(N <= NX_MAXBCS);
    assert(M > 0);
    assert(M <= NX_MAXRANK);

    /* Calculate a broadcast size */
    nx_t ebc = nx_make_bcast(N, args);
    if (ebc.rank < 0) {
        return -1;
    }
    if (ebc.rank < M) {
        KS_THROW(kst_SizeError, "Operation requires at least %i-D inputs", M);
        return -1;
    }
    assert(ebc.rank >= M);

    /* Loop indices */
    ks_size_t idxs[NX_MAXRANK];

    /* Create new arguments arrays, one fully padded, and the other that is a single slice
     *   of the other one, given to 'func'
     */
    nx_t kargs[NX_MAXBCS], kargsS[NX_MAXBCS];

    int i, j, jr;
    for (i = 0; i < N; ++i) {
        /* 'kargs[i]' gives the entire 'i'th input broadcasted to the maximum rank size */
        kargs[i] = args[i];
        kargs[i].rank = ebc.rank;

        /* Extend dimensions to the left */
        for (j = 0; j < ebc.rank - args[i].rank; ++j) {
            kargs[i].shape[j] = 1;
            kargs[i].strides[j] = 0;
        }

        /* Copy the rest, aligned to the right */
        for (jr = 0; j < kargs[i].rank; ++j, ++jr) {
            kargs[i].shape[j] = args[i].shape[jr];
            kargs[i].strides[j] = kargs[i].shape[j] == 1 ? 0 : args[i].strides[jr];
        }

        /* 'kargsS[i]' gives the slice of the 'i'th input broadcasted to the desired size ('M') */
        kargsS[i] = kargs[i];

        /* Pad to maximum rank */
        kargsS[i].rank = M;

        /* We should copy from the right of 'kargs[i]', since it will be a right-most slice */
        for (j = 0; j < kargsS[i].rank; ++j) {
            kargsS[i].shape[j] = kargs[i].shape[j + kargs[i].rank - M];
            kargsS[i].strides[j] = kargs[i].strides[j + kargs[i].rank - M];
        }

    }

    int res = I_apply_Nd(func, ebc, N, kargs, kargsS, M, idxs, extra);
    return res;
}



