/* sort.c - sort algorithm
 *
 * @author: Cade Brown <cade@kscript.org>
 */
#include <ks/impl.h>


// Compute whether 'L <= R', storing in 'is_le'
// NOTE: Returns success, or false if an error was thrown
static bool my_le(kso L, kso R, bool* is_le, kso cmpfunc) {
    if (!cmpfunc) {
        /*if (L == R) {
            // special case
            *is_le = true;
            return true;
        }*/
        if (kso_issub(L->type, kst_int) && kso_issub(R->type, kst_int)) {
            ks_int Li = (ks_int)L, Ri = (ks_int)R;
            *is_le = mpz_cmp(Li->val, Ri->val) <= 0;
            return true;
        } else if (kso_issub(L->type, kst_float) && kso_issub(R->type, kst_float)) {
            *is_le = ((ks_float)L)->val <= ((ks_float)R)->val;
            return true;
        } else if (kso_issub(L->type, kst_str) && kso_issub(R->type, kst_str)) {
            *is_le = ks_str_cmp(((ks_str)L), ((ks_str)R)) <= 0;
            return true;
        }

        kso res = ks_bop_le(L, R);
        if (!res) {
            return false;
        }

        bool g;
        if (!kso_truthy(res, &g)) {
            KS_DECREF(res);
            return false;
        }

        *is_le = g;
        return true;
    } else {
        /* Custom comparison function */
        kso res = kso_call(cmpfunc, 2, (kso[]){ L, R });
        if (!res) return false;

        /* Should return boolean of 'L <= R' */
        bool g;
        if (!kso_truthy(res, &g)) {
            KS_DECREF(res);
            return false;
        }

        *is_le = g;
        return true;
    }
}

// Merge 'A' and 'B'
static bool my_merge(ks_size_t n, kso* elems, kso* keys, ks_size_t A_n, kso* A_elems, kso* A_keys, ks_size_t B_n, kso* B_elems, kso* B_keys, kso cmpfunc) {
    // check if they are already merged
    bool is_sorted;
    if (!my_le(A_keys[A_n - 1], B_keys[B_n - 1], &is_sorted, cmpfunc)) return false;

    if (is_sorted) {
        memcpy(elems, A_elems, sizeof(*elems) * A_n);
        memcpy(elems+A_n, B_elems, sizeof(*elems) * B_n);
        if (elems != keys) {
            memcpy(keys, A_keys, sizeof(*keys) * A_n);
            memcpy(keys+A_n, B_keys, sizeof(*keys) * B_n);
        }
        return true;
    }

    ks_size_t i, Ai = 0, Bi = 0;
    for (i = 0; Ai < A_n && Bi < B_n && i < n; ++i) {
        bool le_AB;
        if (!my_le(A_keys[Ai], B_keys[Bi], &le_AB, cmpfunc)) return false;

        if (le_AB) {
            // *A <= *B
            elems[i] = A_elems[Ai];
            keys[i] = A_keys[Ai++];
        } else {
            elems[i] = B_elems[Bi];
            keys[i] = B_keys[Bi++];
        }
    }

    // copy the rest of the elements
    while (Ai < A_n) {
        elems[i] = A_elems[Ai];
        keys[i++] = A_keys[Ai++];
    }
    while (Bi < B_n) {
        elems[i] = B_elems[Bi];
        keys[i++] = B_keys[Bi++];
    }

    assert(Bi == B_n && Ai == A_n && i == n);
    
    // success
    return true;
}


static bool my_mergesort(ks_size_t n, kso* elems, kso* keys, kso* tmp_elems, kso* tmp_keys, kso cmpfunc) {
    if (n <= 1) {
        // already sorted
        return true;
    } else if (n <= 32) {
        // cutoff for simpler sort
        return ks_sort_insertion(n, elems, keys, cmpfunc);
    }

    ks_size_t mid = n / 2;

    // recursively sort left and right subarrays
    // NOTE: We call with the main/tmp arrays swapped, so we ping-pong them and reduce overall memory
    if (!my_mergesort(mid, tmp_elems, tmp_keys, elems, keys, cmpfunc)) return false;
    if (!my_mergesort(n-mid, tmp_elems+mid, tmp_keys+mid, elems+mid, keys+mid, cmpfunc)) return false;

    // merge them back into the main array
    // NOTE: swapped again, so it writes back to the intended output
    if (!my_merge(n, elems, keys, mid, tmp_elems, tmp_keys, n-mid, tmp_elems+mid, tmp_keys+mid, cmpfunc)) return false;

    // success
    return true;
}

bool ks_sort_insertion(ks_size_t n, kso* elems, kso* keys, kso cmpfunc) {
    if (n <= 1) {
        /* Already sorted (trivially) */
        return true;
    }

    ks_cint i, j;
    for (i = 1; i < n; ++i) {
        kso ki = keys[i], ei = elems[i];

        j = i - 1;

        while (j >= 0) {
            kso kj = keys[j];
            
            /* compute 'ki <= kj' */
            bool le_j_i;
            if (!my_le(kj, ki, &le_j_i, cmpfunc)) {
                return false;
            }

            if (le_j_i) break;

            elems[j + 1] = elems[j];
            keys[j + 1] = keys[j];

            j--;
        }

        elems[j + 1] = ei;
        keys[j + 1] = ki;
    }

    return true;
}


bool ks_sort_merge(ks_size_t n, kso* elems, kso* keys, kso cmpfunc) {

    kso* new_elems = ks_zmalloc(n, sizeof(*new_elems));
    // only allocate twice if keys are unique from 'elems'
    kso* new_keys = elems == keys ? new_elems : ks_zmalloc(n, sizeof(*new_keys));

    memcpy(new_elems, elems, sizeof(*elems) * n);
    if (new_keys != new_elems) memcpy(new_keys, keys, sizeof(*keys) * n);

    bool res = my_mergesort(n, elems, keys, new_elems, new_keys, cmpfunc);

    // free temporary buffers
    ks_free(new_elems);
    if (new_keys != new_elems) ks_free(new_keys);

    return res;
}

bool ks_sort(ks_size_t n, kso* elems, kso* keys, kso cmpfunc) {
    return ks_sort_merge(n, elems, keys, cmpfunc);
}


