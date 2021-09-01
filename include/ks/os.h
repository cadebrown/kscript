/* ks/os.h - kscript 'os' module C-API
 *
 * 
 * @author: Cade Brown <cade@kscript.org>
 */

#pragma once
#ifndef KS_OS_H
#define KS_OS_H

#ifndef KS_KS_H
  #include <ks/ks.h>
#endif

/** Type Definitions **/

/* os.proc - Represents a process being ran
 *
 */
typedef struct ksos_proc_s {
    KSO_BASE

    /* Process ID */
    int pid;

}* ksos_proc;

/* os.thread - Represents a thread of execution, which is normally an OS-level thread
 *
 */
typedef struct ksos_thread_s {
    KSO_BASE

    /* Function being called */
    kso fn;

    /* Arguments given to function */
    ks_tuple args;


    /* The exception that was thrown, or NULL if there is no exception currently being thrown
     */
    ks_exc exc;

    /* List of execution frames, which holds 'os.frame' objects describing what function is executing,
     *   and what the status is
     */
    ks_list frames;

    /* Data stack, which holds objects that are operated on by the bytecode
     */
    ks_list stk;

#ifdef KS_HAVE_PTHREAD

    /* Internal 'pthread' object, a handle to the OS thread */
    pthread_t pthread_;

#endif

}* ksos_thread;



/* os.mutex - MUTual EXclusion lock object, which can be locked to ensure critical sections/data 
 *              or other resources are accessed without side effects
 *
 */
typedef struct ksos_mutex_s {
    KSO_BASE

#ifdef KS_HAVE_PTHREAD

    /* Internal 'pthread' mutex */
    pthread_mutex_t pthread_mutex_;

#endif

}* ksos_mutex;


/* Internal stat type, meant for the C-API only (this should be wrapped by another type, or just used internally) 
 * Use KSOS_CSTAT_* macros
 *
 * TODO: Extract abstract parameters this early?
 */
struct ksos_cstat {

    /* Linux/Unix/BSD/MacOS */
    struct stat val;

};

#ifdef _WIN32

  #define KSOS_CSTAT_ISFILE(_stat) (((_stat).val.st_mode & S_IFMT) == S_IFREG)
  #define KSOS_CSTAT_ISDIR(_stat)  (((_stat).val.st_mode & S_IFMT) == S_IFDIR)
  #define KSOS_CSTAT_ISLINK(_stat) (!KSOS_CSTAT_ISFILE(_stat) && !KSOS_CSTAT_ISDIR(_stat))

#else

  /* Yield whether a cstat is a regular file */
  #define KSOS_CSTAT_ISFILE(_stat) (S_ISREG((_stat).val.st_mode))
  /* Yield whether a cstat is a directory */
  #define KSOS_CSTAT_ISDIR(_stat)  (S_ISDIR((_stat).val.st_mode))
  /* Yield whether a cstat is a symlink */
  #define KSOS_CSTAT_ISLINK(_stat) (S_ISLNK((_stat).val.st_mode))

#endif


/* os.stat - Status query of a file, or stream
 *
 */
typedef struct ksos_stat_s {
    KSO_BASE

    /* C-style status query value */
    struct ksos_cstat val;


}* ksos_stat;


/* os.path - Immutable resource locator for files, directories, and symlinks
 *
 * This type is designed to work easily across multiple operating systems, including
 *   Unix, Linux, and Windows. The basic idea is that instead of just having a string,
 *   there is a root (which is 'none' for relative paths, '/' on *NIX for absolute paths,
 *   and a drive letter (i.e. 'C:\\') on Windows), and a tuple of parts of the path (i.e.
 *   what is seperated by the path seperators)
 * 
 * For example:
 *   'my/file.txt' has root=none, parts=('my', 'file.txt')
 *   '/path/to/my/file.txt' has root='/', parts=('path', 'to', 'my', 'file.txt')
 * 
 * This also means that you can deal with Windows-style path on *NIX and vice-versa
 * 
 * 
 */
typedef struct ksos_path_s {
    KSO_BASE

    /* The root of the path, which is 'none' for relative paths, and can vary among OS-es
     */
    kso root;

    /* Tuple of (string) path components, which are (implicitly) joined with the path seperator
     */
    ks_tuple parts;

    /* String that the path turns into. May be NULL 
     * INTERNAL USE ONLY! USED FOR CACHING
     */
    ks_str str_;

}* ksos_path;



/* os.walk - Iterator over a directory tree structure
 *
 */
typedef struct ksos_walk_s {
    KSO_BASE

    /* If 'true', then generate directories, files, and then yield subdirectories recursively 
     * If 'false', then generate subdirectories recursively, directories, and then files
     */
    bool is_topdown;

    /* If 'true', then only yield directories
     */
    bool dirs_only;


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
    



    /* Internal flag */
    bool first_0;

}* ksos_walk;







#endif /* KS_OS_H */
