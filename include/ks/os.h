/* ks/os.h - `os` module
 *
 * 
 * @author: Cade Brown <cade@kscript.org>
 */

#pragma once
#ifndef KSOS_H__
#define KSOS_H__

#ifndef KS_H__
#include <ks/ks.h>
#endif

/** Constants **/


/** Types **/


/* 'os.path' - Resource locator for a file/directory/symlink on disk
 *
 * This type is immutable, so it cannot be modified
 * 
 */
typedef struct ksos_path_s {
    KSO_BASE

    /* Parts of the path (implicitly) between seperators 
     *
     * i.e. '/my/path/to' -> ('my', 'path', 'to')
     */
    ks_tuple parts;

    /* The root of the path, which is 'none' for relative paths, and either '/' or a drive (i.e. 'C:\')
     *   for absolute paths
     */
    kso root;

    /* The string that the path transforms into. May be NULL. Internal use only for caching */
    ks_str str_;

}* ksos_path;

/* 'os.thread' - Single thread of execution
 *
 */
typedef struct ksos_thread_s {
    KSO_BASE


    /* Name of the function, which might be auto generated */
    ks_str name;

    /* Current scope name, which is used for '__fullname' on newly created functions and types */
    ks_str scopename;


    /* Function */
    kso of;

    /* Arguments the thread was started */
    ks_tuple args;


    /* Stack frames for functions currently executing */
    ks_list frames;

    /* Data stack */
    ks_list stk;

    /* The exception which was thrown, or NULL if none was thrown */
    ks_Exception exc;


    /* Whether it is active, or it has some action queued up */
    bool is_active, is_queue;


#ifdef KS_HAVE_PTHREADS

    /* pthreads internal */
    pthread_t pth_;

#endif

}* ksos_thread;


/* 'os.mutex' - Mutual exclusion object
 *
 */
typedef struct ksos_mutex_s {
    KSO_BASE

    /* The thread the mutex is currently held by, or 'NULL' if it is open */
    ksos_thread owned_by;

    /* Number of threads waiting */
    int _waitct;


#ifdef KS_HAVE_PTHREADS

    /* pthreads internal */
    pthread_mutex_t pm_;

#endif

}* ksos_mutex;


/** Functions **/

/* Create a new path from a C-style string, which will split on seperators
 *
 * If 'len_b < 0' then data is assumed to be NUL-terminated
 */
KS_API ksos_path ksos_path_new(ks_ssize_t len_b, const char* data, kso root);

/* Convert 'ob' to a path (if it isn't already one)
 */
KS_API ksos_path ksos_path_new_o(kso ob);

/* Create a new thread
 * Does not start executing the thread
 */
KS_API ksos_thread ksos_thread_new(ks_type tp, ks_str name, kso of, ks_tuple args);

/* Get the current thread executing (i.e. the thread you are in)
 * Does **not** return a new reference to the result
 */
KS_API ksos_thread ksos_thread_get();

/* Start executing the thread
 */
KS_API bool ksos_thread_start(ksos_thread self);

/* Join the thread (i.e. wait for completion on the calling thread)
 */
KS_API bool ksos_thread_join(ksos_thread self);


/* Create a new mutex (which will be unlocked)
 */
KS_API ksos_mutex ksos_mutex_new(ks_type tp);

/* Locks/unlocks the mutex
 */
KS_API void ksos_mutex_lock(ksos_mutex self);
KS_API void ksos_mutex_unlock(ksos_mutex self);

/* Attempts to lock the thread, returns 'true' if the lock was successful
 */
KS_API bool ksos_mutex_trylock(ksos_mutex self);


/* Types */
KS_API extern ks_type
    ksost_path,
    ksost_thread,
    ksost_mutex
;

/* Globals */
KS_API extern ksio_FileIO
    ksos_stdin,
    ksos_stdout,
    ksos_stderr
;


#endif /* KSOS_H__ */
