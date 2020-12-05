/* funcs.c - kscript standard builtin functions
 *
 * @author: Cade Brown <cade@kscript.org>
 */
#include <ks/impl.h>

ks_func 
    ksf_any,
    ksf_all,
    ksf_min,
    ksf_max,
    ksf_sum,

    ksf_open,
    ksf_close,

    ksf_eval,
    ksf_exec,

    ksf_print,

    ksf_hash,
    ksf_abs,
    ksf_len,
    ksf_repr,
    ksf_id,

    ksf_ord,
    ksf_chr,

    ksf_issub,
    ksf_isinst,
    
    ksf_iter,
    ksf_next
;


static KS_FUNC(print) {
    int n_args;
    kso* args;
    KS_ARGS("*args", &n_args, &args);

    /* Where to output to */
    ksio_AnyIO out = (ksio_AnyIO)ksos_stdout;

    int i;
    for (i = 0; i < n_args; ++i) {
        if (i > 0) ksio_addbuf(out, 1, " ");
        if (!ksio_add(out, "%S", args[i])) return NULL;
    }
    ksio_addbuf(out, 1, "\n");

    return KSO_NONE;
}




void _ksi_funcs() {

    /* Create a function */
    #define F(_name, _sig, _doc) ksf_##_name = (ks_func)ksf_wrap(_name##_, _sig, _doc);

    F(print, "print(*args)", "Prints out all the arguments to 'os.stdout', seperated by spaces, and followed by a newline")


    #undef F

}


