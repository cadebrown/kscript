/* apply.c - vectorized function application
 * 
 *
 * @author:    Cade Brown <cade@kscript.org>
 */
#include <ks/impl.h>
#include <ks/nxi.h>


/* Utils */

bool nx_getbc(int nargs, nx_t* args, int* rank, ks_size_t* shape) {
    assert(nargs > 0);
    int i, j;

    /* The resulting size will be the maximum rank */
    *rank = args[0].rank;
    for (i = 1; i < nargs; ++i) {
        if (args[i].rank > *rank) *rank = args[i].rank;
    }

    /* Negative index (from the end of *each* shape) */
    int ni = -1;

    while (true) {
        /* Required size for this dimension, or -1 if not established */
        ks_ssize_t req = -1;
        bool hadany = false;

        for (i = 0; i < nargs; ++i) {
            if (args[i].rank < -ni) {
                /* Since we are right-aligned and out of bounds, this is broadcastable 
                 * We do this by treating missing dimensions as 1, so we can skip
                 */
                continue;
            } else {
                /* Get dimension */
                hadany = true;
                ks_size_t dim = args[i].shape[args[i].rank + ni];
                if (dim == 1) {
                    /* Always can be broadcasted, and does not change requirements */
                    continue;
                } else if (req < 0) {
                    /* No size requirement yet, so set it */
                    req = dim;
                    continue;
                } else if (dim == req) {
                    /* Fits the requirement */
                    continue;
                } else {
                    /* Bad size */
                    ksio_StringIO sio = ksio_StringIO_new();
                    ksio_add(sio, "Shapes were not broadcastable: ");
                    for (i = 0; i < nargs; ++i) {
                        if (i > 0) ksio_add(sio, ", ");
                        ksio_add(sio, "(");
                        for (j = 0; j < args[i].rank; ++j) {
                            if (j > 0) ksio_add(sio, ", ");
                            ksio_add(sio, "%u", (ks_uint)args[i].shape[j]);
                        }
                        if (j == 1) ksio_add(sio, ",");
                        ksio_add(sio, ")");
                    }

                    ks_str rs = ksio_StringIO_getf(sio);
                    KS_THROW(kst_SizeError, "%S", rs);
                    KS_DECREF(rs);
                    return false;
                }
            }
        }

        /* Check if we are out of inputs */
        if (!hadany) break;

        /* Set the requirement and continue */
        shape[*rank + ni] = req < 0 ? 1 : req;
        ni--;
    }

    return true;
}

/* Whether or not the '_i'th argument should be allocated */
#define SHOULD_ALLOC(_i) (dtype != NULL && args[(_i)].dtype != dtype && (dtypeidx < 0 || (_i) == dtypeidx))

int nx_apply_eleme(nxf_elem func, int nargs, nx_t* args, nx_dtype dtype, int dtypeidx, void* extra) {
    assert(nargs > 0);
    assert(nargs <= NX_MAXBCS);
    
    /* 1D-apply */
    int M = 1;

    /* Calculate broadcast size */
    int rank;
    ks_size_t shape[NX_MAXRANK];
    if (!nx_getbc(nargs, args, &rank, shape)) {
        return -1;
    }

    if (rank < 1) {
        rank = 1;
        shape[0] = 1;
    }

    /* Length of the 1D slices */
    int len = shape[rank - 1];

    /* Create new arguments arrays, one fully padded, and the other that is a single slice
     *   of the other one, given to 'func'
     */
    nx_t kargs[NX_MAXBCS], kargsS[NX_MAXBCS];

    /* Prepare arguments and slice arrays */
    ks_cint i, j, jr;
    for (i = 0; i < nargs; ++i) {

        /* 'kargs[i]' gives the entire 'i'th input broadcasted to the maximum rank size */
        kargs[i] = args[i];
        kargs[i].rank = rank;

        /* Extend dimensions to the left */
        for (j = 0; j < rank - args[i].rank; ++j) {
            kargs[i].shape[j] = 1;
            kargs[i].strides[j] = 0;
        }

        /* Copy the rest, aligned to the right */
        for (jr = 0; j < kargs[i].rank && jr < args[i].rank; ++j, ++jr) {
            kargs[i].shape[j] = args[i].shape[jr];
            kargs[i].strides[j] = kargs[i].shape[j] == 1 ? 0 : args[i].strides[jr];
        }

        /* 'kargsS[i]' gives the slice of the 'i'th input broadcasted to the desired size */
        if (SHOULD_ALLOC(i)) {
            /* Allocate temporary slice */
            /* Correct type, or aren't casting */
            kargsS[i] = kargs[i];

            /* Pad to maximum rank */
            kargsS[i].rank = M;

            /* We should copy from the right of 'kargs[i]', since it will be a right-most slice */
            for (j = 0; j < kargsS[i].rank; ++j) {
                kargsS[i].shape[j] = kargs[i].shape[j + kargs[i].rank - 1];
            }

            /* Allocate data */
            void* dptr = ks_malloc(len * dtype->size);
            if (!dptr) {
                for (j = 0; j < i; ++j) {
                    if (SHOULD_ALLOC(j)) {
                        ks_free(kargsS[j].data);
                    }
                }
                KS_THROW(kst_InternalError, "Malloc failed");
                return -1;
            }
            kargsS[i] = nx_make(dptr, dtype, 1, (ks_size_t[]){ len }, NULL);

        } else {
            /* Correct type, or aren't casting */
            kargsS[i] = kargs[i];

            /* Pad to maximum rank */
            kargsS[i].rank = M;

            /* We should copy from the right of 'kargs[i]', since it will be a right-most slice */
            for (j = 0; j < kargsS[i].rank; ++j) {
                kargsS[i].shape[j] = kargs[i].shape[j + kargs[i].rank - M];
                kargsS[i].strides[j] = kargs[i].strides[j + kargs[i].rank - M];
            }
        }
    }


    /* Now, call the function with slices */

    /* N-dimensional loop indices */
    int looprank = rank - 1;
    ks_size_t idxs[NX_MAXRANK];

    /* Zero out indices */
    for (i = 0; i < looprank; ++i) idxs[i] = 0;

    /* We are looping over all the dimensions not being forwarded to the caller */
    while (true) {
        /* Set data pointers for the slices (strides & dimensions stay the same between invocations) */
        for (i = 0; i < nargs; ++i) {
            if (dtype != NULL && args[i].dtype != dtype) {
                nx_t src = nx_make(
                    szdot(kargs[i].data, looprank, kargs[i].strides, idxs), 
                    kargs[i].dtype,
                    kargs[i].rank - looprank,
                    kargs[i].shape + looprank, 
                    kargs[i].strides + looprank
                );
                /* Cast */
                if (!nx_cast(
                    src,
                    kargsS[i]
                )) {
                    for (j = 0; j < nargs; ++j) {
                        if (SHOULD_ALLOC(j)) {
                            ks_free(kargsS[j].data);
                        }
                    }
                    return -1;
                }
            } else {
                /* Compute offset */
                kargsS[i].data = szdot(kargs[i].data, looprank, kargs[i].strides, idxs);
            }
        }

        /* Apply to 1D slice */
        int res = func(nargs, kargsS, len, extra);
        if (res != 0) {
            /* We return first non-zero exit code (may indicate an error) */
            for (j = 0; j < nargs; ++j) {
                if (SHOULD_ALLOC(j)) {
                    ks_free(kargsS[j].data);
                }
            }
            return res;
        }

        /* Increase least significant index */
        i = looprank - 1;
        if (i < 0) break;
        idxs[i]++;

        while (i >= 0 && idxs[i] >= shape[i]) {
            idxs[i] = 0;
            i--;
            if (i >= 0) idxs[i]++;
        }

        /* Done; overflowed */
        if (i < 0) break;
    }

    for (j = 0; j < nargs; ++j) {
        if (SHOULD_ALLOC(j)) {
            ks_free(kargsS[j].data);
        }
    }
    return 0;
}

int nx_apply_Nde(nxf_Nd func, int nargs, nx_t* args, int M, nx_dtype dtype, int dtypeidx, void* extra) {
    assert(nargs > 0);
    assert(nargs <= NX_MAXBCS);
    assert(M >= 0);
    assert(M <= NX_MAXRANK);

    /* Calculate broadcast size */
    int rank;
    ks_size_t shape[NX_MAXRANK];
    if (!nx_getbc(nargs, args, &rank, shape)) {
        return -1;
    }

    /* Pad ranks */
    while (rank < M) {
        shape[rank] = 1;
        rank++;
    }

    ks_cint i, j, jr;

    /* Calculate slice shape */
    int srank = M;
    ks_size_t sshape[NX_MAXRANK];
    for (i = 0; i < M; ++i) {
        sshape[i] = shape[i + rank - M];
    }

    /* Create new arguments arrays, one fully padded, and the other that is a single slice
     *   of the other one, given to 'func'
     */
    nx_t kargs[NX_MAXBCS], kargsS[NX_MAXBCS];

    /* Prepare arguments and slice arrays */
    for (i = 0; i < nargs; ++i) {

        /* 'kargs[i]' gives the entire 'i'th input broadcasted to the maximum rank size */
        kargs[i] = args[i];
        kargs[i].rank = rank;

        /* Extend dimensions to the left */
        for (j = 0; j < rank - args[i].rank; ++j) {
            kargs[i].shape[j] = 1;
            kargs[i].strides[j] = 0;
        }

        /* Copy the rest, aligned to the right */
        for (jr = 0; j < kargs[i].rank && jr < args[i].rank; ++j, ++jr) {
            kargs[i].shape[j] = args[i].shape[jr];
            kargs[i].strides[j] = kargs[i].shape[j] == 1 ? 0 : args[i].strides[jr];
        }

        /* 'kargsS[i]' gives the slice of the 'i'th input broadcasted to the desired size */
        if (SHOULD_ALLOC(i)) {
            /* Allocate temporary slice */
            /* Correct type, or aren't casting */
            kargsS[i] = kargs[i];

            /* Pad to broadcast rank */
            kargsS[i].rank = M;

            /* We should copy from the right of 'kargs[i]', since it will be a right-most slice */
            for (j = 0; j < kargsS[i].rank; ++j) {
                kargsS[i].shape[j] = kargs[i].shape[j + kargs[i].rank - M];
            }

            /* Allocate data */
            void* dptr = ks_malloc(szprod(srank, sshape) * dtype->size);
            if (!dptr) {
                for (j = 0; j < i; ++j) {
                    if (SHOULD_ALLOC(j)) {
                        ks_free(kargsS[j].data);
                    }
                }
                KS_THROW(kst_InternalError, "Malloc failed");
                return -1;
            }
            kargsS[i] = nx_make(dptr, dtype, srank, sshape, NULL);

        } else {
            /* Correct type, or aren't casting */
            kargsS[i] = kargs[i];

            /* Pad to maximum rank */
            kargsS[i].rank = M;

            /* We should copy from the right of 'kargs[i]', since it will be a right-most slice */
            for (j = 0; j < kargsS[i].rank; ++j) {
                kargsS[i].shape[j] = kargs[i].shape[j + kargs[i].rank - M];
                kargsS[i].strides[j] = kargs[i].strides[j + kargs[i].rank - M];
            }
        }
    }


    /* Now, call the function with slices */

    /* N-dimensional loop indices */
    int looprank = rank - M;
    ks_size_t idxs[NX_MAXRANK];

    /* Zero out indices */
    for (i = 0; i < looprank; ++i) idxs[i] = 0;

    /* We are looping over all the dimensions not being forwarded to the caller */
    while (true) {
        /* Set data pointers for the slices (strides & dimensions stay the same between invocations) */
        for (i = 0; i < nargs; ++i) {
            if (dtype != NULL && args[i].dtype != dtype) {
                nx_t src = nx_make(
                    szdot(kargs[i].data, looprank, kargs[i].strides, idxs), 
                    kargs[i].dtype,
                    kargs[i].rank - looprank,
                    kargs[i].shape + looprank, 
                    kargs[i].strides + looprank
                );
                /* Cast */
                if (!nx_cast(
                    src,
                    kargsS[i]
                )) {
                    for (j = 0; j < nargs; ++j) {
                        if (SHOULD_ALLOC(j)) {
                            ks_free(kargsS[j].data);
                        }
                    }
                    return -1;
                }
            } else {
                /* Compute offset */
                kargsS[i].data = szdot(kargs[i].data, looprank, kargs[i].strides, idxs);
            }
        }

        /* Apply to 1D slice */
        int res = func(nargs, kargsS, srank, sshape, extra);
        if (res != 0) {
            /* We return first non-zero exit code (may indicate an error) */
            for (j = 0; j < nargs; ++j) {
                if (SHOULD_ALLOC(j)) {
                    ks_free(kargsS[j].data);
                }
            }
            return res;
        }

        /* Increase least significant index */
        i = looprank - 1;
        if (i < 0) break;
        idxs[i]++;

        while (i >= 0 && idxs[i] >= shape[i]) {
            idxs[i] = 0;
            i--;
            if (i >= 0) idxs[i]++;
        }

        /* Done; overflowed */
        if (i < 0) break;
    }

    for (j = 0; j < nargs; ++j) {
        if (SHOULD_ALLOC(j)) {
            ks_free(kargsS[j].data);
        }
    }
    return 0;
}

int nx_apply_elem(nxf_elem func, int nargs, nx_t* args, nx_dtype dtype, void* extra) {
    return nx_apply_eleme(func, nargs, args, dtype, -1, extra);
}

int nx_apply_Nd(nxf_Nd func, int nargs, nx_t* args, int M, nx_dtype dtype, void* extra) {
    return nx_apply_Nde(func, nargs, args, M, dtype, -1, extra);
}
