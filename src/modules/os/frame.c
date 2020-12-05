/* os/frame.c - 'os.frame' type
 *
 * 
 * @author: Cade Brown <cade@kscript.org>
 */
#include <ks/impl.h>

#define T_NAME "os.frame"


/* C-API */


ksos_frame ksos_frame_new(kso func) {
    ksos_frame self = KSO_NEW(ksos_frame, ksost_frame);

    KS_INCREF(func);
    self->func = func;

    self->locals = NULL;
    self->pc = NULL;
    self->closure = NULL;

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

    return self;
}



/* Type Functions */


/* Export */

static struct ks_type_s tp;
ks_type ksost_frame = &tp;


void _ksi_os_frame() {
    _ksinit(ksost_frame, kst_object, T_NAME, sizeof(struct ksos_frame_s), -1, NULL);
    

}
