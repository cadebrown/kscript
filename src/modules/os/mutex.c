/* os/mutex.c - 'os.mutex' type
 *
 * 
 * @author: Cade Brown <cade@kscript.org>
 */
#include <ks/impl.h>

#define T_NAME "os.mutex"


/* C-API */

ksos_mutex ksos_mutex_new(ks_type tp) {
    ksos_mutex self = KSO_NEW(ksos_mutex, tp);

    self->owned_by = NULL;
    self->_waitct = 0;

    #ifdef KS_HAVE_pthreads

    /* Internal mutex */
    pthread_mutex_init(&self->pm_, NULL);

    #endif


    return self;
}

void ksos_mutex_lock(ksos_mutex self) {
    ksos_thread th = ksos_thread_get();
    if (self->owned_by == th) return;

    self->_waitct++;

    #ifdef KS_HAVE_pthreads

    /* Lock using the pthreads library */
    pthread_mutex_lock(&self->pm_);

    #else

    /* Technically wrong, but we gotta have something */
    while (self->owned_by != NULL) { }

    #endif


    /* Signal that we own it */
    self->owned_by = th;
    self->_waitct--;
}

bool ksos_mutex_trylock(ksos_mutex self) {
    ksos_thread th = ksos_thread_get();
    if (self->owned_by == th) return true;

    self->_waitct++;

    bool res = false;

    #ifdef KS_HAVE_pthreads

    if (pthread_mutex_trylock(&self->pm_) == 0) {
        /* We locked it successfully */
        res = true;
    }

    #else

    if (self->owned_by == NULL) {
        res = true;
    }

    #endif


    /* Signal that we own it */
    if (res) {
        self->owned_by = th;
        self->_waitct--;
    }
    return res;
}

void ksos_mutex_unlock(ksos_mutex self) {

    #ifdef KS_HAVE_pthreads
    /* Unlock using the pthreads library */
    pthread_mutex_unlock(&self->pm_);

    #else

    #endif

    self->owned_by = NULL;
}


/* Type Functions */


/* Export */

static struct ks_type_s tp;
ks_type ksost_mutex = &tp;


ksos_mutex ksg_GIL = NULL;

void _ksi_os_mutex() {
    _ksinit(ksost_mutex, kst_object, T_NAME, sizeof(struct ksos_mutex_s), -1, "Mutual exclusion lock, which can be acquired and released to enforce data dependencies", KS_IKV(

    ));
    

    /* Create GIL */
    ksg_GIL = ksos_mutex_new(ksost_mutex);

}
