/* os/frame.c - 'os.frame' type
 *
 * 
 * @author: Cade Brown <cade@kscript.org>
 */
#include <ks/impl.h>
#include <ks/compiler.h>

#define T_NAME "os.frame"


/* C-API */


ksos_frame ksos_frame_new(kso func) {
    ksos_frame self = KSO_NEW(ksos_frame, ksost_frame);

    KS_INCREF(func);
    self->func = func;

    self->locals = NULL;
    self->pc = NULL;
    self->closure = NULL;

    self->n_handlers = 0;
    self->handlers = NULL;

    return self;
}

ksos_frame ksos_frame_copy(ksos_frame of) {
    ksos_frame self = KSO_NEW(ksos_frame, ksost_frame);

    KS_INCREF(of->func);
    self->func = of->func;

    if (of->locals) KS_INCREF(of->locals);
    self->locals = of->locals;

    if (of->closure) KS_INCREF(of->closure);
    self->closure = of->closure;

    self->pc = of->pc;

    self->n_handlers = of->n_handlers;
    self->handlers = ks_zmalloc(sizeof(*self->handlers), self->n_handlers);

    ks_size_t i;
    for (i = 0; i < self->n_handlers; ++i) {
        self->handlers[i] = of->handlers[i];
    }

    return self;
}


ks_str ksos_frame_get_tb(ksos_frame of) {
    ksio_StringIO sio = ksio_StringIO_new();
    ksio_BaseIO aio = (ksio_BaseIO)sio;

    kso f = of->func;
    if (kso_issub(f->type, kst_func)) {

        ks_func ff = (ks_func)f;
        if (ff->is_cfunc) {
            kso t = ks_dict_get_c(ff->attr, "__sig");
            ksio_add(aio, "In %S [cfunc]", t);
            KS_DECREF(t);
            return ksio_StringIO_getf(sio);
        } else {
            /* Extract bytecode and let later code handle it*/
            //f = (kso)ff->bfunc.;
        }
    }

    if (kso_issub(f->type, kst_code)) {
        ks_code bc = (ks_code)f;

        ksio_add(aio, "In %R", bc->fname);
        ks_tok tok = KS_TOK_MAKE_EMPTY();

        struct ks_code_meta meta;
        if (!ks_code_get_meta(bc, (int)(of->pc - bc->bc->data), &meta)) {
            kso_catch_ignore();

            if (tok.sline > 0) {
                ksio_add(aio, " (line %i):\n", tok.sline + 1);
                ks_tok_add(aio, bc->fname, bc->src, tok);
            }

        } else {
            ksio_add(aio, " (line %i):\n", meta.tok.sline + 1);
            ks_tok_add(aio, bc->fname, bc->src, meta.tok);
        }
    } else {
        ksio_add(aio, "In %R", f);
    }


    return ksio_StringIO_getf(sio);
}



/* Type Functions */

static KS_TFUNC(T, free) {
    ksos_frame self;
    KS_ARGS("self:*", &self, ksost_frame);

    if (self->closure) KS_DECREF(self->closure);
    if (self->locals) KS_DECREF(self->locals);

    KS_DECREF(self->func);
    if (self->args) KS_DECREF(self->args);
    ks_free(self->handlers);

    KSO_DEL(self);

    return KSO_NONE;
}


/* Export */

static struct ks_type_s tp;
ks_type ksost_frame = &tp;


void _ksi_os_frame() {
    _ksinit(ksost_frame, kst_object, T_NAME, sizeof(struct ksos_frame_s), -1, "Frame of execution, which represents a certain thread state, as well as closures", KS_IKV(
        {"__free",                 ksf_wrap(T_free_, T_NAME ".__free(self)", "")},
    ));
    

}
