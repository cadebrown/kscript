/* os/main.c - 'os' module
 *
 * 
 * @author:    Cade Brown <cade@kscript.org>
 *             Gregory Croisdale <greg@kscript.org>
 */
#include <ks/impl.h>

#define M_NAME "os"

/* C-API */

kso ksos_getenv(ks_str name, kso defa) {
#ifdef KS_HAVE_getenv
    char* res = getenv(name->data);
    if (res) {
        return (kso)ks_str_new(-1, res);
    } else {
        if (defa) {
            return KS_NEWREF(defa);
        } else {
            KS_THROW(kst_KeyError, "Key %R not present in the environment", name);
            return NULL;
        }
    }
#else
    KS_THROW(kst_OSError, "Failed to getenv %R: platform did not provide a 'getenv()' function", name);
    return false;
#endif
}

bool ksos_setenv(ks_str name, ks_str val) {
#ifdef KS_HAVE_putenv
    ks_ssize_t sl = name->len_b + val->len_b + 4;
    char* tmp = malloc(sl);

    snprintf(tmp, sl - 1, "%s=%s", name->data, val->data);


    int rc = putenv(tmp);
    if (rc != 0) {
        KS_THROW(kst_OSError, "Failed to set %R in environment: %s", name, strerror(errno));
        return false;
    } else {
        return true;
    }
#else
    KS_THROW(kst_OSError, "Failed to getenv %R: platform did not provide a 'putenv()' function", name);
    return false;
#endif
}

ksos_path ksos_getcwd() {
#ifdef KS_HAVE_getcwd
    char buf[KSOS_PATH_MAX + 1];
    
    if (getcwd(buf, sizeof(buf) - 1) == NULL) {
        KS_THROW(kst_OSError, "Failed to getcwd: %s", strerror(errno));
        return NULL;
    } else {
        return ksos_path_new(-1, buf, KSO_NONE);
    }

#else
    KS_THROW(kst_OSError, "Failed to getcwd: platform did not provide a 'getcwd()' function");
    return false;
#endif
}

bool ksos_pstat(struct ksos_cstat* self, kso path) {
#ifdef WIN32
    ks_str sp = ksos_path_str(path);
	if (!sp) return NULL;

	int rs = _stat(sp->data, &self->v_stat);
	if (rs != 0) {
		KS_THROW(kst_OSError, "Failed to stat %R: %s", sp, strerror(errno));
		KS_DECREF(sp);
		return NULL;
	}

	KS_DECREF(sp);
	return true;
#elif defined(KS_HAVE_stat)
	ks_str sp = ksos_path_str(path);
	if (!sp) return NULL;

    int rs = stat(sp->data, &self->val);
    if (rs != 0) {
        KS_THROW(kst_OSError, "Failed to stat %R: %s", sp, strerror(errno));
        KS_DECREF(sp);
        return NULL;
    }

    KS_DECREF(sp);
    return true;
#else
    KS_THROW(kst_PlatformWarning, "Failed to stat %R: The platform had no 'stat()' function", sp);
    return NULL;
#endif
}

bool ksos_lstat(struct ksos_cstat* self, kso path) {
#ifdef WIN32
	/* On Windows, treat 'lstat' like 'stat' */
	return ksos_stat(self, path);
#elif defined(KS_HAVE_lstat)
	ks_str sp = ksos_path_str(path);
	if (!sp) return NULL;

    int rs = lstat(sp->data, &self->val);
    if (rs != 0) {
        KS_THROW(kst_OSError, "Failed to lstat %R: %s", sp, strerror(errno));
        KS_DECREF(sp);
        return NULL;
    }
    KS_DECREF(sp);
    return true;
#else
    KS_THROW(kst_PlatformWarning, "Failed to lstat %R: The platform had no 'lstat()' function", sp);
    return NULL;
#endif
}

bool ksos_fstat(struct ksos_cstat* self, int fd) {
#ifdef WIN32
	int rs = _fstat(fd, &out->val);
	if (rs != 0) {
		KS_THROW(kst_OSError, "Failed to fstat %i: %s", fd, strerror(errno));
		return NULL;
	}
	return true;
#elif defined(KS_HAVE_fstat)
    int rs = fstat(fd, &self->val);
    if (rs != 0) {
        KS_THROW(kst_OSError, "Failed to fstat %i: %s", fd, strerror(errno));
        return NULL;
    }
    return true;
#else
    KS_THROW(kst_PlatformWarning, "Failed to fstat %i: The platform had no 'fstat()' function", fd);
    return NULL;
#endif
}

bool ksos_listdir(kso path, ks_list* dirs, ks_list* files) {
#ifdef WIN32
	/* Convert to string */
	ks_str sp = ksos_path_str(path);
	if (!sp) return false;

	/* Collect Entries */
	if (*dirs) {
		ks_list_clear(*dirs);
	} else {
		*dirs = ks_list_new(0, NULL);
	}
	if (*files) {
		ks_list_clear(*files);
	} else {
		*files = ks_list_new(0, NULL);
	}

	/* Iterate over directory */
	HANDLE hit;
	WIN32_FIND_DATA hitdata;

	if ((hit = FindFirstFile(sp->data, &hitdata)) != INVALID_HANDLE_VALUE) {
		do {
			/* Have valid file */
			int sl = strlen(hitdata.cFileName);
			if ((hitdata.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
				if (!((sl == 1 && hitdata.cFileName[0] == '.') || (sl == 2 && hitdata.cFileName[0] == hitdata.cFileName[1] == '.'))) {
					/* Valid directory */
					ks_str sv = ks_str_new(sl, hitdata.cFileName);
					ks_list_pushu(*dirs, (kso)sv);

				}
			} else {
				/* Valid file */
				ks_str sv = ks_str_new(sl, hitdata.cFileName);
				ks_list_pushu(*files, (kso)sv);
			}
			
		} while (FindNextFile(hit, &hitdata));
		FindClose(hit);
	} else {
		KS_THROW(kst_Error, "Failed to open %R", sp);
		KS_DECREF(sp);
		return NULL;
	}
	return true;
#elif defined(KS_HAVE_opendir)
	ks_str sp = ksos_path_str(path);
    if (!sp) return false;

    /* TODO: check OS encoding */
    DIR* dp = opendir(sp->data);
    KS_DECREF(sp);

    if (!dp) {
        KS_THROW(kst_OSError, "Failed to open %R: %s", sp, strerror(errno));
		KS_DECREF(sp);
		return false;
    }

    /* Collect Entries */
    if (*dirs) {
        ks_list_clear(*dirs);
    } else {
        *dirs = ks_list_new(0, NULL);
    }
    if (*files) {
        ks_list_clear(*files);
    } else {
        *files = ks_list_new(0, NULL);
    }

    struct dirent* ent;
    while ((ent = readdir(dp)) != NULL) {
        /* Filter out '.' and '..' */
        char* ent_name = ent->d_name;
        int sl = strlen(ent_name);
        if ((sl == 1 && ent_name[0] == '.') || (sl == 2 && ent_name[0] == '.' && ent_name[1] == '.')) continue;

        /* Convert entry */
        ks_str name = ks_str_new(sl, ent_name);
        ksos_path e_path = ksos_path_join((kso[]){ (kso)path, (kso)name }, 2);

        struct ksos_cstat st;
        if (!ksos_pstat(&st, (kso)e_path)) {
            KS_DECREF(name);
            KS_DECREF(e_path);
            KS_DECREF(*dirs);
            KS_DECREF(*files);
            closedir(dp);
            return false;
        }

        KS_DECREF(e_path);

        /* Now, add to appropriate array */
        if (KSOS_CSTAT_ISDIR(st)) {
            ks_list_push(*dirs, (kso)name);
        } else {
            ks_list_push(*files, (kso)name);
        }
        KS_DECREF(name);
    }

    closedir(dp);
    return true;
#else
	KS_THROW(kst_Error, "Failed to listdir: platform had no 'opendir()' function");
	return false;
#endif
}

bool ksos_chdir(kso path) {
    ks_str sp = ksos_path_str(path);
    if (!sp) return false;

#ifdef KS_HAVE_chdir
    int rc = chdir(sp->data);
    int rcerr = errno;
    if (rc == 0) {
        return true;
    } else {
        KS_THROW(kst_OSError, "Failed to chdir %R: %s", sp, strerror(rcerr));
        return false;
    }
#else
    KS_THROW(kst_OSError, "Failed to chdir %R: platform did not provide a 'chdir()' function", sp);
    KS_DECREF(sp);
    return false;
#endif
}


bool ksos_mkdir(kso path, int mode, bool parents) {
    ks_str sp = ksos_path_str(path);
    if (!sp) return false;

#if defined(WIN32)
    int rc = 0, rcerr = 0;
    if (parents) {
        /* Build parents */
        char* tmp = ks_malloc(sp->len_b + 1);
        memcpy(tmp, sp->data, sp->len_b + 1);

        if (tmp[sp->len_b - 1] == '/') tmp[sp->len_b - 1] = '\0';
        char* p;
        for (p = tmp + 1; *p; p++) {
            if (*p == '/') {
                *p = '\0';

                if ((rc = _mkdir(tmp)) != 0) {
                    if (errno != EEXIST) {
                        /* Had other error, so stop*/
                        rcerr = errno;
                        break;
                    }
                }

                *p = '/';
            }
        }

        ks_free(tmp);
        if (rc == 0) {
            /* No error, try final mkdir */
            rc = _mkdir(sp->data);
            rcerr = errno;
        }

    } else {
        /* Just attempt the one */
        rc = _mkdir(sp->data);
        rcerr = errno;
    }

    if (rc == 0) {
        KS_DECREF(sp);
        return true;
    } else {
        KS_THROW(kst_OSError, "Failed to mkdir %R: %s", sp, strerror(rcerr));
        KS_DECREF(sp);
        return false;
    }
#elif defined(KS_HAVE_mkdir)
    int rc = 0, rcerr = 0;
    if (parents) {
        /* Build parents */
        char* tmp = ks_malloc(sp->len_b + 1);
        memcpy(tmp, sp->data, sp->len_b + 1);

        if (tmp[sp->len_b - 1] == '/') tmp[sp->len_b - 1] = '\0';
        char* p;
        for (p = tmp + 1; *p; p++) {
            if (*p == '/') {
                *p = '\0';

                if ((rc = mkdir(tmp, mode)) != 0) {
                    if (errno != EEXIST) {
                        /* Had other error, so stop*/
                        rcerr = errno;
                        break;
                    }
                }

                *p = '/';
            }
        }

        ks_free(tmp);
        if (rc == 0) {
            /* No error, try final mkdir */
            rc = mkdir(sp->data, mode);
            rcerr = errno;
        }

    } else {
        /* Just attempt the one */
        rc = mkdir(sp->data, mode);
        rcerr = errno;
    }

    if (rc == 0) {
        return true;
    } else {
        KS_THROW(kst_OSError, "Failed to mkdir %R: %s", sp, strerror(rcerr));
        return false;
    }

#else
    KS_THROW(kst_OSError, "Failed to mkdir %R: platform did not provide a 'mkdir()' function", sp);
    KS_DECREF(sp);
    return NULL;
#endif
}

bool ksos_rm(kso path, bool children) {
    ks_str sp = ksos_path_str(path);
    if (!sp) return false;

#ifdef KS_HAVE_remove
    int rc = 0, rcerr = 0;
    if (children) {
        /* Remove children */

        kso walk = kso_call((kso)ksost_walk, 1, (kso[]){ (kso)sp });
        if (!walk) {
            KS_DECREF(sp);
            return NULL;
        }
        /* Iterate through elements of the walk */
        ks_cit it = ks_cit_make(walk);
        kso ob;
        while ((ob = ks_cit_next(&it)) != NULL && !rc) {
            ks_tuple base_dirs_files = (ks_tuple)ob;
            assert(kso_issub(base_dirs_files->type, kst_tuple) && base_dirs_files->len == 3);

            kso base = base_dirs_files->elems[0];
            ks_list files = (ks_list)base_dirs_files->elems[2];
            assert(files->type == kst_list);

            /* Now, remove 'base/file' for each file in files */
            int i;
            for (i = 0; i < files->len; ++i) {
                ks_str tmps = ks_fmt("%S/%S", base, files->elems[i]);
                if (!tmps) {
                    it.exc = true;
                    break;
                }
                rc = remove(tmps->data);
                rcerr = errno;
                KS_DECREF(tmps);
                if (rc != 0) {
                    break;
                }
            }

            /* Now, remove the base */
            if (rc == 0 && !it.exc) {
                ks_str tmps = ks_fmt("%S", base);
                if (!tmps) {
                    it.exc = true;
                    break;
                }
                rc = remove(tmps->data);
                rcerr = errno;
                KS_DECREF(tmps);
                if (rc != 0) {
                    break;
                }
            }

            KS_DECREF(ob);
        }

        KS_DECREF(walk);
        ks_cit_done(&it);
        if (it.exc) {
            KS_DECREF(sp);
            return NULL;
        }

    } else {
        rc = remove(sp->data);
        rcerr = errno;
    }

    if (rc == 0) {
        return true;
    } else {
        KS_THROW(kst_OSError, "Failed to rm %R: %s", sp, strerror(rcerr));
        return false;
    }
#else
    KS_THROW(kst_OSError, "Failed to rm %R: platform did not provide a 'remove()' function", sp);
    KS_DECREF(sp);
    return NULL;
#endif
}



int ksos_exec(ks_str cmd) {
#ifdef KS_HAVE_system
    int res = system(cmd->data);

    if (res < 0) {
        KS_THROW(kst_OSError, "Failed to exec %R: %s", cmd, strerror(errno));
    }

    return res;
#else
    KS_THROW(kst_OSError, "Failed to exec %R: platform did not provide a 'system()' function", cmd);
    return -1;
#endif
}

int ksos_fork() {
#ifdef KS_HAVE_fork
    int res = fork();

    if (res < 0) {
        KS_THROW(kst_OSError, "Failed to fork: %s", strerror(errno));
    }

    return res;
#else
    KS_THROW(kst_OSError, "Failed to fork: platform did not provide a 'fork()' function");
    return -1;
#endif
}

int ksos_pipe(int* fd) {
#ifdef KS_HAVE_pipe
    int res = pipe(fd);

    if (res < 0) {
        KS_THROW(kst_OSError, "Failed to pipe: %s", strerror(errno));
    }

    return res;
#else
    KS_THROW(kst_OSError, "Failed to pipe: platform did not provide a 'pipe(int*)' function");
    return -1;
#endif
}

int ksos_dup2(int oldfd, int newfd) {
#ifdef KS_HAVE_dup2
    int res = dup2(oldfd, newfd);

    if (res < 0) {
        KS_THROW(kst_OSError, "Failed to dup2: %s", strerror(errno));
    }

    return res;
#else
    KS_THROW(kst_OSError, "Failed to dup2: platform did not provide a 'dup2(int, int)' function");
    return -1;
#endif
}


/* Module Functions */

static KS_TFUNC(M, getenv) {
    ks_str key;
    kso defa = NULL;
    KS_ARGS("key:* ?defa", &key, kst_str, &defa);

    return ksos_getenv(key, defa);
}

static KS_TFUNC(M, setenv) {
    ks_str key, val;
    KS_ARGS("key:* val:*", &key, kst_str, &val, kst_str);

    if (!ksos_setenv(key, val)) return NULL;

    return KSO_NONE;
}


static KS_TFUNC(M, fstat) {
    ks_cint fd;
    KS_ARGS("fd:cint", &fd);

    struct ksos_cstat val;
    if (!ksos_fstat(&val, fd)) return NULL;

    return (kso)ksos_stat_wrap(val);
}

static KS_TFUNC(M, lstat) {
    kso path;
    KS_ARGS("path", &path);

    struct ksos_cstat val;
    if (!ksos_lstat(&val, path)) return NULL;

    return (kso)ksos_stat_wrap(val);
}

static KS_TFUNC(M, getcwd) {
    KS_ARGS("");
    return (kso) ksos_getcwd();
}

static KS_TFUNC(M, listdir) {
    kso self;
    KS_ARGS("self", &self);

    ks_list dirs = NULL, files = NULL;
    bool res = ksos_listdir(self, &dirs, &files);

    if (!res) return NULL;
    else return (kso)ks_tuple_newn(2, (kso[]){
        (kso)dirs,
        (kso)files
    });
}

static KS_TFUNC(M, chdir) {
    kso self;
    KS_ARGS("self", &self);

    if (!ksos_chdir(self)) return NULL;

    return KSO_NONE;
}

static KS_TFUNC(M, mkdir) {
    kso self;
    ks_cint mode = 0775;
    bool parents = false;
    KS_ARGS("self ?mode:cint ?parents:bool", &self, &mode, &parents);

    if (!ksos_mkdir(self, mode, parents)) return NULL;
    else {
        return KSO_NONE;
    }
}

static KS_TFUNC(M, rm) {
    kso self;
    bool children = false;
    KS_ARGS("self ?children:bool", &self, &children);

    if (!ksos_rm(self, children)) return NULL;
    
    return KSO_NONE;
}






static KS_TFUNC(M, exec) {
    ks_str cmd;

    KS_ARGS("cmd:*", &cmd, kst_str);

    int res = ksos_exec(cmd);
    if (res < 0) return NULL;

    return (kso) ks_int_new(res);
}

static KS_TFUNC(M, fork) {
    int res = ksos_fork();
    if (res < 0) return NULL;

    return (kso) ks_int_new(res);
}





/* Export */

ksio_FileIO
    ksos_stdin,
    ksos_stdout,
    ksos_stderr
;
ks_list
    ksos_argv
;

ks_module _ksi_os() {
    _ksi_os_mutex();
    _ksi_os_thread();
    _ksi_os_path();
    _ksi_os_walk();
    _ksi_os_frame();
    _ksi_os_proc();
    _ksi_os_stat();

    ksos_argv = ks_list_new(0, NULL);

	ks_str tmp = ks_str_new(1, "-");
    ks_list_push(ksos_argv, (kso)tmp);
    KS_DECREF(tmp);

    ksos_stdin = ksio_FileIO_wrap(ksiot_FileIO, stdin, false, _ksv_stdin, _ksv_r);
    ksos_stdout = ksio_FileIO_wrap(ksiot_FileIO, stdout, false, _ksv_stdout, _ksv_w);
    ksos_stderr = ksio_FileIO_wrap(ksiot_FileIO, stderr, false, _ksv_stderr, _ksv_w);

    ks_module res = ks_module_new(M_NAME, KS_BIMOD_SRC, "Operating system interface", KS_IKV(

        /* Types */
        {"path",                   KS_NEWREF(ksost_path)},
        {"walk",                   KS_NEWREF(ksost_walk)},
        {"stat",                   KS_NEWREF(ksost_stat)},

        {"proc",                   KS_NEWREF(ksost_proc)},
        {"thread",                 KS_NEWREF(ksost_thread)},

        {"frame",                  KS_NEWREF(ksost_frame)},
        {"mutex",                  KS_NEWREF(ksost_mutex)},

        /* Variables */
        {"argv",                   KS_NEWREF(ksos_argv)},

        {"stdin",                  KS_NEWREF(ksos_stdin)},
        {"stdout",                 KS_NEWREF(ksos_stdout)},
        {"stderr",                 KS_NEWREF(ksos_stderr)},
    
        /* Functions */
        {"getenv",                 ksf_wrap(M_getenv_, M_NAME ".getenv(key, defa=none)", "Retrieves the environment entry indicated by 'key', or a default if it was not found\n\n    If 'defa' was not given, then an error is thrown")},
        {"setenv",                 ksf_wrap(M_setenv_, M_NAME ".setenv(key, val)", "Sets an environment entry to another string value")},
        {"getcwd",                 ksf_wrap(M_getcwd_, M_NAME ".getcwd()", "Returns current working directory")},
        {"listdir",                ksf_wrap(M_listdir_, M_NAME ".listdir(path)", "Return a tuple of '(dirs, files)' present in 'path'\n\n    Does not include '.' or '..'")},
        {"chdir",                  ksf_wrap(M_chdir_, M_NAME ".chdir(path)", "Change the current working directory to 'path'")},
        {"mkdir",                  ksf_wrap(M_mkdir_, M_NAME ".mkdir(path, mode=0o775, parents=false)", "Creates a new directory 'path', with the given mode\n\n    If 'parents' is 'true', then all parent directories required are also created, otherwise this function throws an error if a path would require multiple directories to be created")},
        {"rm",                     ksf_wrap(M_rm_, M_NAME ".mkdir(path, parents=false)", "Removes a file or directory from the filesystem\n\n    If 'parents' is 'true', and 'path' is a directory, then it is recursively deleted. Otherwise, if 'path' was a non-empty directory, an error is thrown")},


        {"fstat",                  ksf_wrap(M_fstat_, M_NAME ".fstat(fd)", "Query the open file descriptor 'fd' (which should be an 'int') and return an 'os.stat' object describing it")},
        {"lstat",                  ksf_wrap(M_lstat_, M_NAME ".lstat(path)", "Query the the file/directory 'path', but do not follow symbolic links\n\n    Useful if you want to get information about a link itself, rather than the file it points to")},


        {"exec",                   ksf_wrap(M_exec_, M_NAME ".exec(cmd)", "Attempts to execute a command as if typed in console - returns exit code")},
        {"fork",                   ksf_wrap(M_fork_, M_NAME ".fork()", "Creates a new process by duplicating the calling process - returns 0 in the child, PID > 0 in the parent, and -1 if there was an error")},
    
    
    ));

    return res;
}
