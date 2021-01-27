/* sort.c - 'sort' kernel
 *
 * @author: Cade Brown <cade@kscript.org>
 */
#include <ks/impl.h>
#include <ks/nxi.h>
#include <ks/nxt.h>

bool nx_sort(nx_t X, int axis) {
    return nx_sort_quick(X, axis);
}
