/* ks.c - kscript interpreter
 *
 * @author: Cade Brown <cade@kscript.org>
 */
#include <ks/impl.h>
#include <ks/compiler.h>


int main(int argc, char** argv) {
    if (!ks_init()) return 1;

    ks_str fname = ks_str_new(-1, "myfile.txt");
    ks_str src = ks_str_new(-1, "print(1, 2, 3)");

    ks_tok* toks = NULL;
    ks_ssize_t n_toks = ks_lex(fname, src, &toks);
    kso_exit_if_err();

    ks_ast prog = ks_parse_prog(fname, src, n_toks, toks);
    kso_exit_if_err();

    ks_free(toks);

    ks_code code = ks_compile(fname, src, prog, NULL);
    kso_exit_if_err();

    ksio_add((ksio_AnyIO)ksos_stdout, "Hello world and I am %R!\n", code->vc);

    kso res = kso_call((kso)code, 0, NULL);
    kso_exit_if_err();

    KS_DECREF(res);

    KS_DECREF(prog);
    KS_DECREF(fname);
    KS_DECREF(src);
    KS_DECREF(code);

    return 0;
}
