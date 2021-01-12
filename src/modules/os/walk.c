/* os/walk.c - 'os.walk' type
 *
 * 
 * @author: Cade Brown <cade@kscript.org>
 */
#include <ks/impl.h>

#define T_NAME "os.walk"

/* Internals */

/* C-API */

/* Type Functions */

static KS_TFUNC(T, free) {
    ksos_walk self;
    KS_ARGS("self:*", &self, ksost_walk);

    int i;
    for (i = 0; i < self->n_stk; ++i) {
        KS_DECREF(self->stk[i].base);
        KS_DECREF(self->stk[i].dirs);
        KS_DECREF(self->stk[i].files);
        KS_NDECREF(self->stk[i].res);
    }

    ks_free(self->stk);

    KSO_DEL(self);

    return KSO_NONE;
}

static KS_TFUNC(T, new) {
    ks_type tp;
    kso path;
    bool topdown = false;
    KS_ARGS("tp:* path ?topdown:bool", &tp, kst_type, &path, &topdown);

    ksos_path p = ksos_path_new_o(path);
    if (!p) KS_DECREF(p);

    ks_list dirs = NULL, files = NULL;
    if (!ksos_listdir((kso)p, &dirs, &files)) {
        KS_DECREF(p);
        return NULL;
    }

    ksos_walk self = KSO_NEW(ksos_walk, tp);

    self->is_topdown = topdown;

    self->n_stk = 1;
    self->stk = ks_zmalloc(sizeof(*self->stk), self->n_stk);

    self->stk[0].base = p;
    self->stk[0].dirs = dirs;
    self->stk[0].files = files;
    self->stk[0].pos = 0;
    self->stk[0].res = NULL;
    self->stk[0].res = ks_tuple_new(3, (kso[]){
        (kso)p,
        (kso)dirs,
        (kso)files,
    });

    self->first_0 = false;
    return (kso)self;
}

static KS_TFUNC(T, next) {
    ksos_walk self;
    KS_ARGS("self:*", &self, ksost_walk);

    /* Keep going (while there are items on the stack) */
    while (true) {

        /* The top of the recursion stack */
        #define TOP (self->stk[self->n_stk - 1])

        if (self->n_stk == 1 && self->is_topdown && !self->first_0) {
            self->first_0 = true;
            return KS_NEWREF(TOP.res);
        }

        /* Keep going while the top frame still has a directory to emit */
        while (self->n_stk > 0 && TOP.pos < TOP.dirs->len) {

            /* Get the subdirectory by joining the paths of the base and this element */
            ksos_path sub = ksos_path_join((kso[]){ (kso)TOP.base, TOP.dirs->elems[TOP.pos++] }, 2);
            if (!sub) {
                return NULL;
            }

            /* Get contents of the directories */
            ks_list dirs = NULL, files = NULL;
            if (!ksos_listdir((kso)sub, &dirs, &files)) {
                KS_DECREF(sub);
                return NULL;
            }

            /* Push on an item to the recursion stack */
            int i = self->n_stk++;
            self->stk = ks_zrealloc(self->stk, sizeof(*self->stk), self->n_stk);

            /* Initialize entries to the new subdirectory */
            TOP.base = sub;
            TOP.dirs = dirs;
            TOP.files = files;
            TOP.pos = 0;
            TOP.res = ks_tuple_new(3, (kso[]){
                (kso)sub,
                (kso)dirs,
                (kso)files,
            });
            
            /* Top-down recursion should go ahead and emit the subdirectory before full recursion */
            if (self->is_topdown) {
                return KS_NEWREF(TOP.res);
            }
        }

        /* Pop off empty directories, returning their results (only if they are not top down) */
        while (self->n_stk > 0 && TOP.pos >= TOP.dirs->len) {
            KS_DECREF(TOP.base);
            KS_DECREF(TOP.dirs);
            KS_DECREF(TOP.files);
            ks_tuple res = TOP.res;
            self->n_stk--;

            if (!self->is_topdown) {
                /* Return the reference */
                return (kso)res;
            } else {
                /* Free the reference */
                KS_DECREF(res);
            }
        }

        /* No more entries */
        if (self->n_stk < 1) {
            KS_OUTOFITER();
            return NULL;
        }

        /* Repeat, as we have not found a result */
    }


    assert(false);
    return NULL;
}



/* Export */

static struct ks_type_s tp;
ks_type ksost_walk = &tp;

void _ksi_os_walk() {

    _ksinit(ksost_walk, kst_object, T_NAME, sizeof(struct ksos_walk_s), -1, "Recursive iterator through directory entries", KS_IKV(
        {"__free",                 ksf_wrap(T_free_, T_NAME ".__free(self)", "")},
        {"__new",                  ksf_wrap(T_new_, T_NAME ".__new(tp, path='.', topdown=false)", "")},
       // {"__repr",                 ksf_wrap(TW_repr_, TW_NAME ".__repr(self)", "")},

        {"__next",                 ksf_wrap(T_next_, T_NAME ".__next(self)", "")},

    ));
}
