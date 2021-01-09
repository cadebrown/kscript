/* mem.c - memory related functions, including allocation, management, size calculation
 *
 * 
 * @author:    Cade Brown <cade@kscript.org>
 */
#include <ks/impl.h>


/* Computes the next size in the reallocation scheme
 * 
 * By having a ratio, we reduce repeated resizing to an amortized constant time operation
 */
#define _NEXTSIZE(_sz) (2 * (_sz) / 1)

KS_API void* ks_malloc(ks_size_t sz) {
    void* res = malloc(sz);

    if (!res && sz > 0) {
        /* Failed to allocate requested bytes */
    }

    return res;
}

KS_API void* ks_zmalloc(ks_size_t sz, ks_size_t num) {
    return ks_malloc(sz * num);
}

void* ks_realloc(void* ptr, ks_size_t sz) {
    void* res = realloc(ptr, sz);

    if (!res && sz > 0) {
        /* Failed to allocate requested bytes */
    }

    return res;
}

void* ks_zrealloc(void* ptr, ks_size_t sz, ks_size_t num) {
    return ks_realloc(ptr, sz * num);
}

void ks_free(void* ptr) {
    if (!ptr) return;

    free(ptr);
}

ks_ssize_t ks_nextsize(ks_ssize_t cur_sz, ks_ssize_t req) {

    if (cur_sz >= req) {
        /* Already large enough */
        return cur_sz;
    } else {
        /* For small values, just go ahead and request 4 as the minimum elements */
        if (req <= 4) return 4;

        return _NEXTSIZE(req);
    }
}

