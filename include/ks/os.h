/* ks/os.h - `os` module
 *
 * 
 * @author:    Cade Brown <cade@kscript.org>
 *             Gregory Croisdale <greg@kscript.org>
 */

#pragma once
#ifndef KSOS_H__
#define KSOS_H__

#ifndef KS_H__
#include <ks/ks.h>
#endif

#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <fcntl.h>


#ifdef KS_HAVE_SYS_WAIT_H
  #include <sys/wait.h>
#endif

#ifdef KS_HAVE_DIRENT_H
  #include <dirent.h>
#endif


/** Constants **/

#ifdef PATH_MAX
  #define KSOS_PATH_MAX PATH_MAX
#else
  #define KSOS_PATH_MAX 4096
#endif


/** Types **/

/* Internal stat type, meant for the C-API only (this should be wrapped by another type, or just used internally) 
 * Use KSOS_CSTAT_* macros
 */
struct ksos_cstat {

    /* Linux/Unix/BSD/MacOS */
    struct stat v_stat;

};

#ifdef WIN32

#define KSOS_CSTAT_ISFILE(_stat) (((_stat).v_stat.st_mode & S_IFMT) == S_IFREG)
#define KSOS_CSTAT_ISDIR(_stat)  (((_stat).v_stat.st_mode & S_IFMT) == S_IFDIR)
#define KSOS_CSTAT_ISLINK(_stat) (!KSOS_CSTAT_ISFILE(_stat) && !KSOS_CSTAT_ISDIR(_stat))

#else

/* Yield whether a cstat is a regular file */
#define KSOS_CSTAT_ISFILE(_stat) (S_ISREG((_stat).v_stat.st_mode))

/* Yield whether a cstat is a directory */
#define KSOS_CSTAT_ISDIR(_stat)  (S_ISDIR((_stat).v_stat.st_mode))

/* Yield whether a cstat is a symlink */
#define KSOS_CSTAT_ISLINK(_stat) (S_ISLNK((_stat).v_stat.st_mode))


#endif

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

/* 'os.path.walk' - Iterator over OS paths
 *
 */
typedef struct ksos_path_walk_s {
    KSO_BASE

    /* If 'true', then generate directories, files, and then yield subdirectories recursively 
     * If 'false', then generate subdirectories recursively, directories, and then files
     */
    bool is_topdown;

    /* If 'true', then only yield directories
     */
    bool is_dirs_only;

    /* Internal flag */
    bool first_0;


    /* Number of items in the recursion stack */
    int n_stk;

    /* Stack of directories being walked */
    struct {

        /* Base path of this part of the stack */
        ksos_path base;

        /* Subdirectories in 'base' */
        ks_list dirs;

        /* Files in 'base' */
        ks_list files;

        /* Position within 'dirs' and 'files'; if < dirs->len, still processing directories */
        int pos;

        /* Restult to be returned (may be NULL) */
        ks_tuple res;

    }* stk;

}* ksos_path_walk;



/* 'os.frame' - Single execution frame
 *
 */
typedef struct ksos_frame_s* ksos_frame;

struct ksos_frame_s {
    KSO_BASE

    /* Function being executed */
    kso func;

    /* Arguments being executed */
    ks_tuple args;

    /* Dictionary of local variables (if NULL, there were none) */
    ks_dict locals;

    /* If non-NULL */
    ksos_frame closure;

    /* Program counter, current position */
    unsigned char* pc;

};

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


    /* List of objects currently inside 'repr()' */
    ks_list inrepr;

    /* Variadic index of the assignment */
    int assv;


    /* Stack frames for functions currently executing */
    ks_list frames;

    /* Data stack */
    ks_list stk;

    /* The exception which was thrown, or NULL if none was thrown */
    ks_Exception exc;


    /* Whether it is active, or it has some action queued up */
    bool is_active, is_queue;

    /* Number of bytecode handlers in the current thread */
    int n_handlers;

    /* Array of bytecode handlers */
    struct ksos_thread_handler {
        /* Stack length to restore to */
        int stklen;

        /* Program counter to jump to */
        unsigned char* topc;

    }* handlers;


#ifdef KS_HAVE_pthreads

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


#ifdef KS_HAVE_pthreads

    /* pthreads internal */
    pthread_mutex_t pm_;

#endif

}* ksos_mutex;

/* 'os.proc' - A process which can be executed and redirected
 *
 */

typedef struct ksos_proc_s {
    KSO_BASE

    /* Program being executed */
    ks_tuple argv;

	/* Process ID */
    int pid;

    /* stdin, stdout, stderr */
    ksio_FileIO v_in, v_out, v_err;

} *ksos_proc;


/** Functions **/



/** Misc. Process/Environment Functions **/

/* Execute a command and wait as if run in shell - returns exit code
 */
KS_API int ksos_exec(ks_str cmd);

/* Forks current process - returns 0 if parent, pid > 0 if else
 */
KS_API int ksos_fork();

/* Creates a pipe as according to the cstd
 */
KS_API int ksos_pipe(int* fd);

/* Duplicates an fd according to the cstd
 */
KS_API int ksos_dup2(int oldfd, int newfd);


/* Wait for a pid, and calculate the status
 */
KS_API bool ksos_waitpid(int pid, int* status);

/* Tell whether 'pid' refers to an alive process
 */
KS_API bool ksos_isalive(int pid, bool* out);

/* Attempt to send a signal to a pid
 */
KS_API bool ksos_kill(int pid, int sig);



/* Attempt to retrieve an environment variable, and return 'defa' if none was found
 * If 'defa==NULL', then an exception will be thrown if the key was not found
 * Returns whether one was found
 */
KS_API kso ksos_getenv(ks_str key, kso defa);
KS_API bool ksos_setenv(ks_str key, ks_str val);

/* Attempt to retrieve the current working directory, and return NULL if an error
 * has occurred. In the event that the c command getcwd() is not available,
 * an execption is thrown.
 */
KS_API ksos_path ksos_getcwd();

/** Filesystem/Paths **/

/* Create a new path from a C-style string, which will split on seperators
 *
 * If 'len_b < 0' then data is assumed to be NUL-terminated
 */
KS_API ksos_path ksos_path_new(ks_ssize_t len_b, const char* data, kso root);

/* Convert 'ob' to a path (if it isn't already one)
 */
KS_API ksos_path ksos_path_new_o(kso ob);

/* Joins together 'map(os.path, paths)'
 */
KS_API ksos_path ksos_path_join(kso* paths, int len);

/* Returns the parent of a path object.
 *  If len(self.parts) == 0, ret os.path('..')
 */
KS_API ksos_path ksos_path_parent(kso self);


/* Attempts to resolve 'path' to an absolute path */
KS_API ksos_path ksos_path_real(kso path);

/* Performs a C-style 'stat' on a path object
 */
KS_API bool ksos_path_stat(kso path, struct ksos_cstat* out);
KS_API bool ksos_path_lstat(kso path, struct ksos_cstat* out);
KS_API bool ksos_path_fstat(int fd, struct ksos_cstat* out);

/* Return whether 'path' exists, and set '*res' to whether it does
 */
KS_API bool ksos_path_exists(kso path, bool* res);

/* Calculate whether 'path' is a file */
KS_API bool ksos_path_isfile(kso path, bool* res);

/* Calculate whether 'path' is a directory */
KS_API bool ksos_path_isdir(kso path, bool* res);

/* Calculate whether 'path' is a symbolic link */
KS_API bool ksos_path_islink(kso path, bool* res);

/* Return a list of directories and files in a given directory */
KS_API bool ksos_path_listdir(kso path, ks_list* dirs, ks_list* files);

/* Attempts to create a directory. If 'parents' is given, then parents are created as well */
KS_API bool ksos_path_mkdir(kso path, int mode, bool parents);

/* Attempts to remove a path (file or directory). If 'children' is given, then children are recursively 
 *   removed as well (otherwise, non-empty directories throw errors) 
 */
KS_API bool ksos_path_rm(kso path, bool children);

/* Changes the current working direcotory to a given path
 */
KS_API bool ksos_path_chdir(kso path);



/** Threading **/

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


/* Create new 'os.frame'
 */
KS_API ksos_frame ksos_frame_new(kso func);

/* Create a copy of an 'os.frame', with shared reference to 'of''s variables,
 *   but now is distinct (usefull for when exceptions are thrown)
 */
KS_API ksos_frame ksos_frame_copy(ksos_frame of);

/* Return a string representing the traceback of a frame's execution
 */
KS_API ks_str ksos_frame_get_tb(ksos_frame self);

/* Returns information about the the given frame
 * References are NOT returned to the output variables, and no exception is thrown if it returns false
 */
KS_API bool ksos_frame_get_info(ksos_frame self, ks_str* fname, ks_str* func, int* line);


/* Linearize the linked-list structure of the frames, returning a list of frames with 
 *   'self' at the beginning
 */
KS_API ks_list ksos_frame_expand(ksos_frame self);

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
KS_API_DATA ks_type
    ksost_path,
    ksost_path_walk,
    ksost_thread,
    ksost_frame,
    ksost_mutex,
    ksost_proc
;

/* Globals */
KS_API_DATA ksio_FileIO
    ksos_stdin,
    ksos_stdout,
    ksos_stderr
;

KS_API_DATA ks_list
    ksos_argv
;


#endif /* KSOS_H__ */
