/* inter.c - implementation of the interactive interpreter in kscript
 *
 * I've decoupled the generators so that you can pretty much plug-and-play any line editor (i.e.
 *   GNU Readline, BSD libedit, kscript's linenoise, etc), and you just have to add ~10 lines or so
 * 
 * I haven't gotten around to implementing libedit and others because I figure it's probably not worth it
 * 
 * 
 * @author:    Cade Brown <cade@kscript.org>
 */

#include <ks/impl.h>
#include <ks/os.h>
#include <ks/compiler.h>

#if defined(KS_HAVE_readline)

/* Use GNU readline */
#include <readline/readline.h>
#include <readline/history.h>

#else

/* Fallback (TODO: make even better) */
//#warning Building kscript without 'readline', so limited interactivity and line editing features

#endif

/** Internals **/

/* Whether 'stdin' is TTY. If it is, we should print things out */
static bool is_tty_stdin = false;

/* Misc. keywords that should be completed */
static char* misc_complete[] = {
    "import ",
    "from ",

    "in ",
    "as ",

    "break ",
    "cont ",
    "ret ",
    "throw ",

    "if ",
    "elif ",
    "else ",
    "while ",
    "for ",
    "try ",
    "catch ",
    "finally ",

    "extends ",

    NULL
};

/* String buffer being built from the input */
static ksio_StringIO code = NULL;

/* Primary prompt, continuation prompt, and last variable prompt */
static ks_str prompt0 = NULL, prompt1 = NULL, prompt2 = NULL;

/* Attempt to match a set of names to a valid 'root', i.e. dictionary of variables which they may reference */
static void match(ks_list out, ks_dict root, int n_names, ks_str* names, int pos, const char* text, int dep, bool req_call /* require callable */) {
    bool hidden = names[0]->data[0] == '_';

    int i;
    for (i = 0; i < root->len_ents; ++i) {
        /* Get key/value pair */
        ks_str k = (ks_str)root->ents[i].key;
        kso v = root->ents[i].val;
        if (!k || k->type != kst_str) continue;

        /* Determine whether it should be ignored because it is only callable */
        bool is_call = kso_is_callable(v) && dep != 0;
        if (req_call && !is_call) continue;

        /* What should be put after the match? */
        char* postfix = is_call ? "(" : (kso_issub(v->type, kst_module) ? "." : "");

        if (n_names == 1) {
            /* Directly match without recursion */
            if ((hidden || k->data[0] != '_') && strncmp(k->data, names[0]->data, names[0]->len_b) == 0) {
                ks_list_pushu(out, (kso)ks_fmt("%.*s%S%s", pos, text, k, postfix));
            }
        } else {
            /* Must match exactly, since the user has entered things after this */
            if (ks_str_eq(k, names[0])) {
                if (kso_issub(v->type, kst_type)) {
                    /* Match attributes defined via the type, recursively up to the parents */
                    ks_type tp = (ks_type)v;
                    do {
                        match(out, tp->attr, n_names-1, names+1, pos + k->len_b + 1, text, dep+1, false);
                        tp = tp->i__base;
                    } while (tp != tp->i__base);

                } else if (kso_issub(v->type, kst_module)) {
                    /* Complete with module members */
                    match(out, ((ks_module)v)->attr, n_names-1, names+1, pos + k->len_b + 1, text, dep+1, false);
                }

                /* No matter what the type, also match callable attributes on the parent (which can be member functions) */
                ks_type tp = v->type;
                do {
                    match(out, tp->attr, n_names-1, names+1, pos + k->len_b + 1, text, dep+1, true /* require callable */);
                    tp = tp->i__base;
                } while (tp != tp->i__base);
            }
        }
    }
}


/* Attempt to complete a given text buffer, storing possible matches/completions in 'out' */
static void complete(ks_list out, const char* text) {
    int len = strlen(text);

    /* Find the position of the start of this 'word' (which includes '.', and the like), which will be the object
     *   we try and complete
     */
    int pos = len;
    while (pos > 0) {
        char c = text[pos - 1];
        if (!(('a' <= c && c <= 'z') || ('A' <= c && c <= 'Z') || ('0' <= c && c <= '9') || (c == '_' || c == '.'))) {
            break;
        }

        pos--;
    }

    /* Splut into names */
    ks_str word = ks_str_new(len - pos, text + pos);
    ks_str dot = ks_str_new(-1, ".");
    ks_list names = ks_str_split(word, dot);
    KS_DECREF(word);
    KS_DECREF(dot);

    /* Now, complete from the set of roots (globals, interpreter variables) */
    match(out, ksg_inter_vars, names->len, (ks_str*)names->elems, pos, text, 0, false);
    match(out, ksg_globals, names->len, (ks_str*)names->elems, pos, text, 0, false);

    if (names->len == 1) {
        /* Iterate over grammar/misc symbols */
        char** it = misc_complete;
        while (*it) {
            if (strncmp(*it, ((ks_str)names->elems[0])->data, ((ks_str)names->elems[0])->len_b) == 0) {
                ks_list_pushu(out, (kso)ks_fmt("%s", *it));
            }
            it++;
        }
    }
    KS_DECREF(names);
}


#ifdef KS_HAVE_readline


/* Generates matches
 *
 * SEE: http://web.mit.edu/gnu/doc/html/rlman_2.html#SEC36
 */
static char* my_rl_gen(const char* text, int state) {

    /* State variables, which are the list of completions, and the position being returned */
    static ks_list out = NULL;
    static int i = 0;

    if (!state) {
        /* First run */

        /* Go ahead and compute completions */        
        out = ks_list_new(0, NULL);
        complete(out, text);
        i = 0;
    }

    if (out && i < out->len) {
        ks_str v = (ks_str)out->elems[i];

        /* Make a copy that readline can free (and copy NUL-terminator) */
        char* res = malloc(v->len_b + 1);
        memcpy(res, v->data, v->len_b+1);

        /* Advance to next one */
        i++;

        return res;
    }

    /* No more matches */
    if (out) KS_DECREF(out);
    out = NULL;
    return NULL;
}

/* Return list of possible matches for text
 * Uses 'my_rl_gen' to actually generate them
 */
static char** my_rl_complete(const char* text, int start, int end) {
    rl_attempted_completion_over = 1;
    rl_completion_append_character = '\0';
    return rl_completion_matches(text, my_rl_gen);
}

#endif


/** Signal Handling **/


/* Handle SIGINT, causes Ctrl+C to print a message and then redisplay */
static void handle_sigint(int signum) {
    #define MSG_SIGINT_RESET "\nUse 'CTRL-D' or 'exit()' to quit the process\n"
    fwrite(MSG_SIGINT_RESET, 1, sizeof(MSG_SIGINT_RESET) - 1, stdout);

    code->len_b = code->len_c = 0;

    #ifdef KS_HAVE_readline

    /* Regenerate prompt and clear text */
    rl_on_new_line();
#ifndef __APPLE__
    rl_replace_line("", 0);
#endif
    rl_set_prompt(prompt0->data);
    rl_redisplay();

    #else

    fwrite(prompt0->data, 1, prompt0->len_b, stdout);
    fflush(stdout);

    #endif


    /* Re-set the signal handler for next time */
    signal(SIGINT, handle_sigint);
}


static void update_prompts() {
    if (prompt0) KS_DECREF(prompt0);
    if (prompt1) KS_DECREF(prompt1);
    if (prompt2) KS_DECREF(prompt2);
    prompt0 = (ks_str)ks_dict_get_c(ksg_config, "prompt0");
    prompt1 = (ks_str)ks_dict_get_c(ksg_config, "prompt1");
    prompt2 = (ks_str)ks_dict_get_c(ksg_config, "prompt2");
    assert(prompt0 && prompt1 && prompt2);
}

/* Run forever */
bool ks_inter() {
    static bool hasinit = false;

    if (!hasinit) {
        /* First time initialization */
        hasinit = true;

        is_tty_stdin = isatty(0);

        code = ksio_StringIO_new();

        /* If to a terminal, then initialize terminal libraries for the first time */
        if (is_tty_stdin) {
            update_prompts();

            #if defined(KS_HAVE_readline)
    
            rl_attempted_completion_function = my_rl_complete;
            
            // don't append a space
            rl_completion_append_character = '\0';

#ifndef __APPLE__
            rl_catch_signals = 0;
            rl_clear_signals();
#endif
            
            #else

            //ks_warn("ks", "Built without readline support, so line editing support is limited");

            #endif

            signal(SIGINT, handle_sigint);
        }
    }

    /* Number of inputs so far */
    int ct = 0;
    
    /* Ensure prompts are updated */
    update_prompts();

    /* Keep taking prompts */
    bool done = false;
    while (!done) {
        ks_str fname = ks_fmt("<inter-%i>", ct++);
        
        /* Clear buffer for source code */
        code->len_b = code->len_c = 0;

        ks_str src = NULL;
        ks_tok* toks = NULL;
        ks_ssize_t n_toks = 0;

        /* Parse lines contained in this input */
        while (true) {

            /* First, read the input */
            if (is_tty_stdin) {
                /* Read interactively */
                update_prompts();

                char* prompt = code->len_b == 0 ? prompt0->data : prompt1->data;

                #ifdef KS_HAVE_readline
                /* Use GNU readline, which will allow for more sophisticated features from input */
                KS_GIL_UNLOCK();
                char* rl_res = readline(prompt);
                KS_GIL_LOCK();
                if (!rl_res) {
                    done = true;
                    break;
                }

                /* If non-empty, add to history */
                if (*rl_res) add_history(rl_res);

                /* Append to the code result */
                ksio_add((ksio_BaseIO)code, "%s", rl_res);
                free(rl_res);
                #else

                /* Just use the default print and then read back */
                printf("%s", prompt);
                char* gl_res = NULL;
                ks_ssize_t max_len = 0;
                ks_ssize_t len = ksu_getline(&gl_res, &max_len, stdin);
                if (len < 0) {
                    kso_catch_ignore();
                    done = true;
                    break;
                }
                ksio_add((ksio_BaseIO)code, "%.*s", (int)len, gl_res);

                ks_free(gl_res);

                #endif
            } else {
                char* gl_res = NULL;
                ks_ssize_t max_len = 0;
                ks_ssize_t len = ksu_getline(&gl_res, &max_len, stdin);
                if (len < 0) {
                    kso_catch_ignore();
                    ks_free(gl_res);
                    done = true;
                    break;
                }
                ksio_add((ksio_BaseIO)code, "%.*s", (int)len, gl_res);

                ks_free(gl_res);
            }

            /* Now, tokenize the new source code, and see if a continuation should be added */
            if (src) KS_DECREF(src);
            src = ksio_StringIO_get(code);
                
            toks = NULL;
            n_toks = ks_lex(fname, src, &toks);
            if (n_toks < 0) {
                kso_catch_ignore_print();
                ks_free(toks);
                continue;
            }

            /* Count tokens */
            int n_brk = 0, n_brc = 0, n_par = 0, i;
            for (i = 0; i < n_toks; ++i) {
                switch (toks[i].kind)
                {
                case KS_TOK_LPAR:
                    n_par++;
                    break;
                case KS_TOK_RPAR:
                    n_par--;
                    break;
                case KS_TOK_LBRC:
                    n_brc++;
                    break;
                case KS_TOK_RBRC:
                    n_brc--;
                    break;
                
                case KS_TOK_LBRK:
                    n_brk++;
                    break;
                case KS_TOK_RBRK:
                    n_brk--;
                    break;
                
                default:
                    break;
                }
            }

            ks_free(toks);

            if (n_brk > 0 || n_brc > 0 || n_par > 0) {
                continue;
            } else {
                break;
            }
        }

        /* Now, actually compile and run the input */
        KS_NDECREF(src);
        src = ksio_StringIO_get(code);

        /* Turn the input code into a list of tokens */
        toks = NULL;
        n_toks = ks_lex(fname, src, &toks);
        if (n_toks < 0) {
            kso_catch_ignore_print();
            ks_free(toks);
            KS_DECREF(fname);
            KS_DECREF(src);
            continue;
        }

        /* Parse the tokens into an AST */
        ks_ast prog = ks_parse_prog(fname, src, n_toks, toks);
        ks_free(toks);
        if (!prog) {
            kso_catch_ignore_print();
            KS_DECREF(fname);
            KS_DECREF(src);
            continue;
        }

        /* Reduce to single statement */
        if (prog->kind == KS_AST_BLOCK && prog->args->len == 1) {
            ks_ast tmp = (ks_ast)prog->args->elems[0];
            KS_INCREF(tmp);
            KS_DECREF(prog);
            prog = tmp;
        }

        bool do_print = is_tty_stdin && ks_ast_is_expr(prog->kind);

        /* Make it so it is the return value */
        if (do_print) {
            prog = ks_ast_newn(KS_AST_RET, 1, &prog, NULL, prog->tok);
        }

        /* Compile the AST into a bytecode object which can be executed */
        ks_code code = ks_compile(fname, src, prog, NULL);
        KS_DECREF(prog);
        if (!code) {
            KS_DECREF(fname);
            KS_DECREF(src);
            kso_catch_ignore_print();
            continue;
        }

        /* Capture status (so we can tell if the expression printed something out) */
        ks_ssize_t sz_w = ksos_stdout->sz_w;

        /* Execute the program, which should return the value */
        kso res = kso_call_ext((kso)code, 0, NULL, ksg_inter_vars, NULL);
        KS_DECREF(code);
        if (!res) {
            KS_DECREF(fname);
            KS_DECREF(src);
            kso_catch_ignore_print();
            continue;
        }

        if (do_print && sz_w == ksos_stdout->sz_w) {
            /* We should actually print it out */
            ksio_add((ksio_BaseIO)ksos_stdout, "%R\n", res);
        }
        ks_dict_set(ksg_inter_vars, (kso)prompt2, res);
        KS_DECREF(res);

        KS_DECREF(fname);
        KS_DECREF(src);
    }

    return true;
}


