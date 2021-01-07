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
static int I_apply(nx_elem_cf cf, int N, nxar_t* inp, nxar_t* inp_1d, ks_ssize_t* max_dims, ks_ssize_t* idxs, void* _data) {
    ks_cint i;
    ks_ssize_t sz0 = 0;
    int loop_N = inp[0].rank - 1;

    /* Zero out indices */
    for (i = 0; i < loop_N; ++i) idxs[i] = 0;

    while (true) {

        /* Convert to 1D slices */
        for (i = 0; i < N; ++i) {
            inp_1d[i] = inp[i];

            inp_1d[i].rank = 1;
            if (inp[i].rank == 0) {
                /* Element tensor */
                inp_1d[i].dims = &my_sz1;
                inp_1d[i].strides = &my_sz0;
                inp_1d[i].data = inp[i].data;

            } else {
                inp_1d[i].dims = &inp[i].dims[inp[i].rank - 1];
                /* Choose whether to repeat the element (stride==0, used when broadcasting), or the actual stride */
                inp_1d[i].strides = inp_1d[i].dims[0] == 1 ? &sz0 : &inp[i].strides[inp[i].rank - 1];
                inp_1d[i].data = nx_get_ptr(inp[i].data, loop_N, inp[i].dims, inp[i].strides, idxs);
            }
        }

        /* Apply to 1D slice */
        int res = cf(N, inp_1d, inp[0].rank == 0 ? 1 : max_dims[inp[0].rank - 1], _data);
        if (res) return res;

        /* Increase least significant index */
        i = loop_N - 1;
        if (i < 0) break;
        idxs[i]++;

        while (i >= 0 && idxs[i] >= max_dims[i]) {
            idxs[i] = 0;
            i--;
            if (i >= 0) idxs[i]++;
        }

        /* Done; overflowed */
        if (i < 0) break;
    }

    return 0;
}


int nx_apply_elem(nx_elem_cf cf, int N, nxar_t* inp, void* _data) {
    assert(N > 0);

    /* Error check */
    int eor;
    ks_size_t* ebc = nx_calc_bcast(N, inp, &eor);
    if (!ebc) {
        return -1;
    }
    ks_free(ebc);

    ks_cint i, j, jr;
    int max_rank = inp[0].rank;
    for (i = 1; i < N; ++i) {
        if (inp[i].rank > max_rank) max_rank = inp[i].rank;
    }
    

    /* Create new arrays with padded dimensions to the maximum rank */
    nxar_t* new_inp = ks_zmalloc(sizeof(*new_inp), N);
    nxar_t* new_inp_1d = ks_zmalloc(sizeof(*new_inp), N);

    /* Keep track of the maximum size in every dimension */
    ks_cint* max_dims = ks_zmalloc(sizeof(*max_dims), max_rank);
    for (i = 0; i < max_rank; ++i) max_dims[i] = 0;

    /* Indexes for iteration */
    ks_cint* idxs = ks_zmalloc(sizeof(*idxs), max_rank);

    for (i = 0; i < N; ++i) {
        new_inp[i] = inp[i];

        /* Pad to maximum rank */
        new_inp[i].rank = max_rank;
        new_inp[i].dims = ks_zmalloc(sizeof(*new_inp[i].dims), max_rank);
        new_inp[i].strides = ks_zmalloc(sizeof(*new_inp[i].strides), max_rank);

        /* Dimensions and strides that are not filled in should act like size==1 */
        for (j = 0; j < max_rank - inp[i].rank; ++j) {
            new_inp[i].dims[j] = 1;
            new_inp[i].strides[j] = 0;
        }

        /* Copy the rest, aligned to the right */
        for (jr = 0; j < max_rank; ++j, ++jr) {
            new_inp[i].dims[j] = inp[i].dims[jr];
            new_inp[i].strides[j] = new_inp[i].dims[j] == 1 ? 0 : inp[i].strides[jr];
        }

        /* Update maximum dimension */
        for (j = 0; j < max_rank; ++j) if (new_inp[i].dims[j] > max_dims[j]) max_dims[j] = new_inp[i].dims[j];

    }




    int res = I_apply(cf, N, new_inp, new_inp_1d, max_dims, idxs, _data);

    for (i = 0; i < N; ++i) {
        ks_free(new_inp[i].dims);
        ks_free(new_inp[i].strides);
    }

    ks_free(new_inp);
    ks_free(max_dims);
    ks_free(idxs);
    ks_free(new_inp_1d);

    return res;
}

