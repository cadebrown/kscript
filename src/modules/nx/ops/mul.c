/* mul.c - 'mul' kernel
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
    ks_uint pR = (ks_uint)inp[0].data, pX = (ks_uint)inp[1].data, pY = (ks_uint)inp[2].data;
    ks_cint sR = inp[0].strides[0], sX = inp[1].strides[0], sY = inp[2].strides[0];

#define LOOP(TYPE) do { \
    for (i = 0; i < len; i++, pR += sR, pX += sX, pY += sY) { \
        *(TYPE*)pR = *(TYPE*)pX * *(TYPE*)pY; \
    } \
    return 0; \
} while (0);

    NXT_DO_INTS(dtype, LOOP);
    NXT_DO_FLOATS(dtype, LOOP);
#undef LOOP

#define LOOP(TYPE) do { \
    for (i = 0; i < len; i++, pR += sR, pX += sX, pY += sY) { \
        TYPE t; \
        t.re = ((TYPE*)pX)->re * ((TYPE*)pY)->re - ((TYPE*)pX)->im * ((TYPE*)pY)->im; \
        ((TYPE*)pR)->im = ((TYPE*)pX)->re * ((TYPE*)pY)->im + ((TYPE*)pX)->im * ((TYPE*)pY)->re; \
        ((TYPE*)pR)->re = t.re; \
    } \
    return 0; \
} while (0);

    NXT_DO_COMPLEXS(dtype, LOOP);
#undef LOOP

    return 1;
}

bool nx_mul(nxar_t r, nxar_t x, nxar_t y) {

    nxar_t tX, tY;
    kso rX, rY;
    if (!nx_getcast(x, r.dtype, &tX, &rX)) {
        return false;
    }
    if (!nx_getcast(y, r.dtype, &tY, &rY)) {
        KS_NDECREF(rX);
        return false;
    }

    bool res = !nx_apply_elem(kern, 3, (nxar_t[]){ r, tX, tY }, NULL);

    KS_NDECREF(rX);
    KS_NDECREF(rY);
    return res;
}


