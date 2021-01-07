/* ks.c - kscript interpreter, commandline interface
 *
 * @author: Cade Brown <cade@kscript.org>
 */
#include <ks/impl.h>
#include <ks/compiler.h>
#include <ks/getarg.h>

/* Variables currently being used */
static ks_dict vars = NULL;

/* Compile and run generically */
static kso do_gen(ks_str fname, ks_str src, ks_dict usedict) {
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

    while (prog->kind == KS_AST_BLOCK && prog->args->len == 1) {
        ks_ast ch = (ks_ast)prog->args->elems[0];
        KS_INCREF(ch);
        KS_DECREF(prog);
        prog = ch;
    }

    /* Check if we should print */
    bool do_print = false;
    if (ks_ast_is_expr(prog->kind)) {
        do_print = true;
        prog = ks_ast_newn(KS_AST_RET, 1, &prog, NULL, prog->tok);
    }

    /* Compile the AST into a bytecode object which can be executed */
    ks_code code = ks_compile(fname, src, prog, NULL);
    KS_DECREF(prog);
    if (!code) {
        return NULL;
    }

    /*

    ks_str t = ks_str_new(-1, "dis");
    kso x = kso_getattr((kso)code, t);
    KS_DECREF(t);
    if (x) {
        kso y = kso_call(x, 0, NULL);
        KS_DECREF(x);
        ks_printf("DIS: \n%S\n", y);
        KS_DECREF(y);
    } else {
        kso_catch_ignore_print();
    }

    */

    ks_ssize_t sz_w = ksos_stdout->sz_w;

    /* Execute the program, which should return the value */
    kso res = kso_call_ext((kso)code, 0, NULL, usedict, NULL);
    KS_DECREF(code);
    if (!res) return NULL;

    if (sz_w == ksos_stdout->sz_w && do_print) {
        ks_printf("%S\n", res);
    } 

    return res;
}

/* Do expression with '-e' */
static bool do_e(ks_str src) {
    ks_str fname = ks_fmt("<expr>");


    kso res = do_gen(fname, src, ksg_inter_vars);
    KS_DECREF(fname);
    if (!res) return NULL;

    KS_DECREF(res);
    return true;
}


/* Do file */
static bool do_f(ks_str fname) {
    ks_str src = ksio_readall(fname);
    if (!src) return false;

    kso res = do_gen(fname, src, NULL);
    KS_DECREF(src);
    if (!res) return false;

    KS_DECREF(res);
    return true;
}

/** Action **/

static KS_FUNC(import) {
    ks_str name;
    KS_ARGS("name:*", &name, kst_str);

    ks_module m = ks_import(name);
    if (!m) {
        return NULL;
    }

    ks_dict_set(ksg_inter_vars, (kso)name, (kso)m);
    KS_DECREF(m);

    return KSO_NONE;
}
static KS_FUNC(verbose) {
    kso parser;
    ks_str name, arg;
    KS_ARGS("parser name:* arg:*", &parser, &name, kst_str, &arg, kst_str);

    ks_logger lg = ks_logger_get_c("ks");
    lg->level -= 10;
    KS_DECREF(lg);

    return KSO_NONE;
}


int main(int argc, char** argv) {
    if (!ks_init()) return 1;
    int i;

    /* Initialize 'os.argv' */
    ks_list_clear(ksos_argv);
    for (i = 0; i < argc; ++i) {
        ks_list_pushu(ksos_argv, (kso)ks_str_new(-1, argv[i]));
    }

    ksga_Parser p = ksga_Parser_new("ks", "kscript interpreter, commandline interface", "0.0.1", "Cade Brown <cade@kscript.org>");
    p->stop_at_pos = true;

    kso on_import = ksf_wrap(import_, "on_import(name)", "Imports a module name to the global interpreter vars");
    kso on_verbose = ksf_wrap(verbose_, "on_verbose(name)", "Increases verbosity");

    ksga_opt(p, "import", "Imports a module name before running anything", "-i,--import", on_import, KSO_NONE);
    ksga_flag(p, "verbose", "Increase the default verbosity", "-v,--verbose", on_verbose);
    ksga_opt(p, "expr", "Compiles and runs an expression", "-e,--expr", NULL, KSO_NONE);
    ksga_opt(p, "code", "Compiles and runs code", "-c,--code", NULL, KSO_NONE);
    ksga_pos(p, "args", "File to run and arguments given to it", NULL, -1);

    KS_DECREF(on_import);
    KS_DECREF(on_verbose);

    ks_dict args = ksga_parse(p, ksos_argv);
    kso_exit_if_err();

    /* Get arguments */
    kso expr = ks_dict_get_c(args, "expr"), code = ks_dict_get_c(args, "code");
    ks_list newargv = (ks_list)ks_dict_get_c(args, "args");
    kso_exit_if_err();

    /* Reclaim 'os.argv' */
    ks_list_clear(ksos_argv);

    if (expr != KSO_NONE || code != KSO_NONE) {
        ks_list_insertu(ksos_argv, 0, (kso)ks_str_new(-1, "-"));
    }

    for (i = 0; i < newargv->len; ++i) {
        ks_list_pushu(ksos_argv, newargv->elems[i]);
    }

    if (expr == KSO_NONE && code == KSO_NONE && ksos_argv->len == 0) {
        ks_list_insertu(ksos_argv, 0, (kso)ks_str_new(-1, "-"));
    }

    KS_DECREF(p);
    bool res = false;

    if (expr == KSO_NONE && code == KSO_NONE) {
        /* Run file */
        ks_str fname = (ks_str)ksos_argv->elems[0];
        if (fname->data[0] == '-') {
            /* Interactive session */
            res = ks_inter();
        } else {
            /* Execute file */
            res = do_f(fname);
        }
    } else if (expr != KSO_NONE && code == KSO_NONE) {
        ks_list_del(ksos_argv, 0);
        ks_list_insertu(ksos_argv, 0, (kso)ks_str_new(-1, "<expr>"));
        res = do_e((ks_str)expr);
    } else if (expr == KSO_NONE && code != KSO_NONE) {
        ks_list_del(ksos_argv, 0);
        ks_list_insertu(ksos_argv, 0, (kso)ks_str_new(-1, "<code>"));
        res = do_e((ks_str)code);
    } else {
        KS_THROW(kst_Error, "Given both '-e' and '-c'");
    }


    KS_DECREF(expr);
    KS_DECREF(code);
    KS_DECREF(newargv);
    KS_DECREF(args);
    kso_exit_if_err();

    return 0;
}
