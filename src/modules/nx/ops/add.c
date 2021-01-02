/* add.c - 'add' kernel
 *
 * @author: Cade Brown <cade@kscript.org>
 */
#include <ks/impl.h>
#include <ks/nx.h>
#include <ks/nxt.h>

static int kern(int N, nxar_t* inp, int len, void* _data) {
    assert(N == 3);
    assert(inp[0].dtype == inp[1].dtype && inp[1].dtype == inp[2].dtype);

    ks_cint i;
    nx_dtype dtype = inp[0].dtype;
    ks_uint pA = (ks_uint)inp[0].data, pB = (ks_uint)inp[1].data, pC = (ks_uint)inp[2].data;
    ks_cint sA = inp[0].strides[0], sB = inp[1].strides[0], sC = inp[2].strides[0];

#define LOOP(TYPE) do { \
    for (i = 0; i < len; i++, pA += sA, pB += sB, pC += sC) { \
        *(TYPE*)pA = *(TYPE*)pB + *(TYPE*)pC; \
    } \
    return 0; \
} while (0);

    NXT_DO_INTS(dtype, LOOP);
    NXT_DO_FLOATS(dtype, LOOP);
#undef LOOP

#define LOOP(TYPE) do { \
    for (i = 0; i < len; i++, pA += sA, pB += sB, pC += sC) { \
        ((TYPE*)pA)->re = ((TYPE*)pB)->re + ((TYPE*)pC)->re; \
        ((TYPE*)pA)->im = ((TYPE*)pB)->im + ((TYPE*)pC)->im; \
    } \
    return 0; \
} while (0);

    NXT_DO_COMPLEXS(dtype, LOOP);
#undef LOOP


    return 1;
}

bool nx_add(nxar_t A, nxar_t B, nxar_t C) {
    return !nx_apply_elem(kern, 3, (nxar_t[]){ A, B, C }, NULL);
}


