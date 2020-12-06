/* ks.c - kscript interpreter, commandline interface
 *
 * @author: Cade Brown <cade@kscript.org>
 */
#include <ks/impl.h>
#include <ks/compiler.h>


/* Compile and run generically */
static kso do_gen(ks_str fname, ks_str src) {
    /* Turn the input code into a list of tokens */
    ks_tok* toks = NULL;
    ks_ssize_t n_toks = ks_lex(fname, src, &toks);
    if (n_toks < 0) {
        ks_free(toks);
        return NULL;
    }

    /* Parse the tokens into an AST */
    ks_ast prog = ks_parse_prog(fname, src, n_toks, toks);
    ks_free(toks);
    if (!prog) {
        return NULL;
    }

    /* Compile the AST into a bytecode object which can be executed */
    ks_code code = ks_compile(fname, src, prog, NULL);
    KS_DECREF(prog);
    if (!code) {
        return NULL;
    }

    /* Execute the program, which should return the value */
    kso res = kso_call((kso)code, 0, NULL);
    KS_DECREF(code);
    if (!res) return NULL;

    return res;
}

/* Do expression with '-e' */
static bool do_e(ks_str src) {
    ks_str fname = ks_fmt("<expr>");

    kso res = do_gen(fname, src);
    KS_DECREF(fname);
    if (!res) return NULL;

    KS_DECREF(res);
    return true;
}


/* Do file */
static bool do_f(ks_str fname) {
    ks_str src = ksio_readall(fname);
    if (!src) return false;

    kso res = do_gen(fname, src);
    KS_DECREF(src);
    if (!res) return false;

    KS_DECREF(res);
    return true;
}



int main(int argc, char** argv) {
    if (!ks_init()) return 1;

    ks_str src = ks_str_new(-1, "print ('abc', 'def')");
    
    bool res = do_e(src);
    kso_exit_if_err();


    KS_DECREF(src);

    return 0;
}
