/* ks.c - kscript interpreter
 *
 * @author: Cade Brown <cade@kscript.org>
 */
#include <ks/impl.h>


int main(int argc, char** argv) {
    if (!ks_init()) return 1;

    ksio_add((ksio_AnyIO)ksos_stdout, "Hello world and I am %f!\n", (ks_cfloat)1.0 * 4);

    return 0;
}

