/* Parser.c - implementation of the 'getarg.Parser' type in kscript
 *
 * 
 * @author:    Cade Brown <cade@kscript.org>
 */
#include <ks/impl.h>
#include <ks/getarg.h>

#define T_NAME "getarg.Parser"

/* Internals */


/* Builtin actions */
static kso action_help = NULL, action_version = NULL;

/* C-API */

ksga_Parser ksga_Parser_new(const char* name, const char* doc, const char* version, const char* authors) {
    ksga_Parser self = KSO_NEW(ksga_Parser, ksgat_Parser);

    self->name = ks_str_new(-1, name);
    self->doc = ks_str_new(-1, doc);
    self->version = ks_str_new(-1, version);
    self->authors = ks_str_split_c(authors, "\n");
    self->help = NULL;

    self->stop_at_pos = false;

    self->n_flag = self->n_opt = self->n_pos = 0;

    self->flag = NULL;
    self->opt = NULL;
    self->pos = NULL;

    /* Add defaults */
    ksga_flag(self, "_help", "Prints this help/usage message and then exits", "-h,--help", action_help);
    ksga_flag(self, "_version", "Prints the version information and then exits", "--version", action_version);

    return self;
}

void ksga_flag(ksga_Parser self, const char* name, const char* doc, const char* opts, kso action) {
    int i = self->n_flag++;
    self->flag = ks_zrealloc(self->flag, sizeof(*self->flag), self->n_flag);

    self->flag[i].name = ks_str_new(-1, name);
    self->flag[i].doc = ks_str_new(-1, doc);
    self->flag[i].opts =  ks_str_split_c(opts, ",");
    if (action) KS_INCREF(action);
    self->flag[i].action = action;
}

void ksga_opt(ksga_Parser self, const char* name, const char* doc, const char* opts, kso trans, kso defa) {
    if (!trans || trans == KSO_NONE) trans = (kso)kst_str; 
    int i = self->n_opt++;
    self->opt = ks_zrealloc(self->opt, sizeof(*self->opt), self->n_opt);

    self->opt[i].name = ks_str_new(-1, name);
    self->opt[i].doc = ks_str_new(-1, doc);
    self->opt[i].opts =  ks_str_split_c(opts, ",");

    KS_INCREF(trans);
    self->opt[i].trans = trans;

    if (defa) KS_INCREF(defa);
    self->opt[i].defa = defa;
}

void ksga_pos(ksga_Parser self, const char* name, const char* doc, kso trans, int num) {
    if (!trans || trans == KSO_NONE) trans = (kso)kst_str; 
    int i = self->n_pos++;
    self->pos = ks_zrealloc(self->pos, sizeof(*self->pos), self->n_pos);

    self->pos[i].name = ks_str_new(-1, name);
    self->pos[i].doc = ks_str_new(-1, doc);

    KS_INCREF(trans);
    self->pos[i].trans = trans;

    self->pos[i].num = num;
}

ks_dict ksga_parse(ksga_Parser self, ks_list args) {
    #define ARG(_i) ((ks_str)args->elems[_i])

    /* Result of names of all kinds */
    ks_dict res = ks_dict_new(NULL);
    
    /* List of arguments that were not consumed by flags and args, and thus must be positional arguments */
    ks_list pos = ks_list_new(0, NULL);

    if (args->len < 1) {
        KS_THROW(kst_Error, "List of arguments being parsed should have at least one element (the name of the program)");
        KS_DECREF(pos);
        KS_DECREF(res);
        return NULL;
    }

    int i, j, k;

    /* Initialize some things */
    int* flag_ct = ks_zmalloc(sizeof(*flag_ct), self->n_flag);
    for (j = 0; j < self->n_flag; ++j) {
        flag_ct[j] = 0;
    }

    /* Now, iterate until we get to the end, or a special character, and deal with this accordingly */
    i = 1;
    while (i < args->len) {
        if (ks_str_eq_c(ARG(i), "-", 1)) {
            /* Special case: a single '-' should be treated as a value, so it should be a positional argument */
            ks_list_push(pos, (kso)ARG(i++));
        } else if (ks_str_eq_c(ARG(i), "--", 2)) {
            /* Special case: '--' means to ignore all arguments after this and only treat them as positionals */
            i++;
            while (i < args->len) {
                ks_list_push(pos, (kso)ARG(i++));
            }
        } else if (ARG(i)->data[0] == '-') {
            /* Now, anything that begins with '-' is supposed to be some sort of argument */
            bool had = false;
            for (j = 0; j < self->n_flag && !had; ++j) {
                struct ksga_flag a = self->flag[j];
                for (k = 0; k < a.opts->len && !had; ++k) {
                    ks_str key = (ks_str)a.opts->elems[k];
                    if (ks_str_eq(key, ARG(i))) {
                        flag_ct[j]++;
                        if (a.action != KSO_NONE) {
                            kso t = kso_call(a.action, 3, (kso[]){ (kso)self, (kso)a.name, (kso)ARG(i) });
                            if (!t) {
                                KS_DECREF(res);
                                KS_DECREF(pos);
                                ks_free(flag_ct);
                                return NULL;
                            }
                            KS_DECREF(t);
                        }
                        i++;
                        had = true;
                    }
                }
            }
            for (j = 0; j < self->n_opt && !had; ++j) {
                struct ksga_opt a = self->opt[j];
                ks_str s = NULL;
                for (k = 0; k < a.opts->len && !had; ++k) {
                    ks_str key = (ks_str)a.opts->elems[k];
                    if (ks_str_eq(key, ARG(i))) {
                        /* Seperate arguments */
                        i++;
                        if (i < args->len) {
                            s = ARG(i++);
                            KS_INCREF(s);
                        }
                        had = true;
                    } else if (ARG(i)->len_b > key->len_b && ks_str_eq_c(key, ARG(i)->data, key->len_b) && ARG(i)->data[key->len_b] == '=') {
                        /* Joined argument with '=' */
                        s = ks_str_new(ARG(i)->len_b - key->len_b - 1, ARG(i)->data + key->len_b + 1);
                        i++;
                        had = true;
                    } else if (ARG(i)->len_b > key->len_b && ks_str_eq_c(key, ARG(i)->data, key->len_b)) {
                        /* Joined argument without anything */
                        s = ks_str_new(ARG(i)->len_b - key->len_b, ARG(i)->data + key->len_b);
                        i++;
                        had = true;
                    }
                }
                if (had) {
                    if (!s) {
                        KS_THROW(kst_Error, "Option %R (%R) requires an argument after it", a.name, a.opts);
                        KS_DECREF(res);
                        KS_DECREF(pos);
                        ks_free(flag_ct);
                        return NULL;
                    } else {
                        /* Actually process argument */
                        if (a.trans != KSO_NONE) {
                            kso ns = kso_call(a.trans, 1, (kso[]){ (kso)s });
                            if (!ns) {
                                KS_DECREF(res);
                                KS_DECREF(pos);
                                KS_DECREF(s);
                                ks_free(flag_ct);
                                return NULL;
                            }
                            KS_DECREF(s);
                            s = (ks_str)ns;
                        }
                        bool has;
                        if (!ks_dict_has_h(res, (kso)a.name, a.name->v_hash, &has)) {
                            assert(false);
                        }
                        if (has && (a.trans == KSO_NONE || kso_issub(a.trans->type, kst_type))) {
                            KS_THROW(kst_Error, "Option %R (%R) was given multiple times (only allowed once)", a.name, a.opts);
                            KS_DECREF(res);
                            KS_DECREF(pos);
                            KS_DECREF(s);
                            ks_free(flag_ct);
                            return NULL;
                        }

                        ks_dict_set_h(res, (kso)a.name, a.name->v_hash, (kso)s);
                        KS_DECREF(s);
                    }
                }
            }
            
            if (!had) {
                KS_THROW(kst_Error, "Unknown option %R", ARG(i));
                KS_DECREF(res);
                KS_DECREF(pos);
                ks_free(flag_ct);
                return NULL;
            }

        } else {
            /* Assume a positional argument */
            ks_list_push(pos, (kso)ARG(i++));

            /* Add the rest to positional arguments */
            if (self->stop_at_pos) {
                while (i < args->len) {
                    ks_list_push(pos, (kso)ARG(i++));
                }
            }
        }
    }


    /* Set all the flag counts */
    for (j = 0; j < self->n_flag; ++j) {
        struct ksga_flag a = self->flag[j];
        ks_int v = ks_int_new(flag_ct[j]);
        ks_dict_set_h(res, (kso)a.name, a.name->v_hash, (kso)v);
        KS_DECREF(v);
    }

    ks_free(flag_ct);

    /* Fill in options */
    for (j = 0; j < self->n_opt; ++j) {
        struct ksga_opt a = self->opt[j];

        bool has;
        if (!ks_dict_has_h(res, (kso)a.name, a.name->v_hash, &has)) {
            assert(false);
        }

        if (!has) {
            if (a.defa) {
                ks_dict_set_h(res, (kso)a.name, a.name->v_hash, a.defa);
            } else {
                KS_THROW(kst_Error, "Required option %R (%R) was not given", a.name, a.opts);
                KS_DECREF(res);
                KS_DECREF(pos);
                return NULL;
            }
        }
    }

    /* Now, parse positional arguments */
    
    /* First, detect the star argument, which can parse whatever */
    int va_idx = -1, min_pos = 0, num_before = 0;
    for (j = 0; j < self->n_pos; ++j) {
        struct ksga_pos a = self->pos[j];
        if (a.num < 0) {
            if (va_idx >= 0) {
                KS_THROW(kst_Error, "Positional arguments %R and %R both take any number of arguments, which is invalid", self->pos[va_idx].name, a.name);
                KS_DECREF(res);
                KS_DECREF(pos);
                return NULL;
            }
            va_idx = j;
        } else {
            min_pos += a.num;
            if (va_idx < 0) num_before += a.num;
        }
    }
    ks_list tmp = NULL;

    if (va_idx < 0) {
        /* No variadic arguments, so just parse directly */

        /* 'i' tracks position within 'pos' */
        for (i = j = 0; j < self->n_pos; ++j) {
            struct ksga_pos a = self->pos[j];
            int si = i;
            i += a.num;
            if (i > pos->len) {
                KS_THROW(kst_Error, "Not enough of positional argument %R given", a.name);
                KS_DECREF(res);
                KS_DECREF(pos);
                return NULL;
            }

            tmp = ks_list_new(a.num, pos->elems + si);
            if (a.trans != KSO_NONE && a.trans != (kso)kst_str) {
                /* Transform in place */
                for (k = 0; k < tmp->len; ++k) {
                    kso ob = tmp->elems[k];
                    kso obt = kso_call(a.trans, 1, &ob);
                    if (!obt) {
                        KS_DECREF(tmp);
                        KS_DECREF(res);
                        KS_DECREF(pos);
                        return NULL;
                    }
                    KS_DECREF(ob);
                    tmp->elems[k] = obt;
                }
            }

            if (a.num == 1) {
                ks_dict_set_h(res, (kso)a.name, a.name->v_hash, tmp->elems[0]);
            } else {
                ks_dict_set_h(res, (kso)a.name, a.name->v_hash, (kso)tmp);
            }

            KS_DECREF(tmp);
        }

        if (i < pos->_max_len) {
            KS_THROW(kst_Error, "Extra positional arguments given");
            KS_DECREF(res);
            KS_DECREF(pos);
            return NULL;
        }
    } else {
        /* There were some variadic ones, so we need to claim from the left and right, then handle the rest */
        if (pos->len < min_pos) {
            KS_THROW(kst_Error, "Not enough positional arguments given (need at least %i)", min_pos);
            KS_DECREF(res);
            KS_DECREF(pos);
            return NULL;
        }

        i = 0;
        for (j = 0; j < va_idx; ++j) {
            struct ksga_pos a = self->pos[j];
            int si = i;
            i += a.num;
            assert (i <= pos->len);

            tmp = ks_list_new(a.num, pos->elems + si);

            if (a.trans != KSO_NONE && a.trans != (kso)kst_str) {
                /* Transform in place */
                for (k = 0; k < tmp->len; ++k) {
                    kso ob = tmp->elems[k];
                    kso obt = kso_call(a.trans, 1, &ob);
                    if (!obt) {
                        KS_DECREF(tmp);
                        KS_DECREF(res);
                        KS_DECREF(pos);
                        return NULL;
                    }
                    KS_DECREF(ob);
                    tmp->elems[k] = obt;
                }
            }

            if (a.num == 1) {
                ks_dict_set_h(res, (kso)a.name, a.name->v_hash, tmp->elems[0]);
            } else {
                ks_dict_set_h(res, (kso)a.name, a.name->v_hash, (kso)tmp);
            }
            KS_DECREF(tmp);
        }
        for (j = va_idx; j < va_idx+1; ++j) {
            struct ksga_pos a = self->pos[j];
            int si = i;
            int num = pos->len - min_pos;
            i += num;
            assert (i <= pos->len);

            tmp = ks_list_new(num, pos->elems + si);
            if (a.trans != KSO_NONE && a.trans != (kso)kst_str) {
                /* Transform in place */
                for (k = 0; k < tmp->len; ++k) {
                    kso ob = tmp->elems[k];
                    kso obt = kso_call(a.trans, 1, &ob);
                    if (!obt) {
                        KS_DECREF(tmp);
                        KS_DECREF(res);
                        KS_DECREF(pos);
                        return NULL;
                    }
                    KS_DECREF(ob);
                    tmp->elems[k] = obt;
                }
            }

            ks_dict_set_h(res, (kso)a.name, a.name->v_hash, (kso)tmp);
            KS_DECREF(tmp);
        }

        for (j = va_idx+1; j < self->n_pos; ++j) {
            struct ksga_pos a = self->pos[j];
            int si = i;
            i += a.num;
            assert (i <= pos->len);

            tmp = ks_list_new(a.num, pos->elems + si);
            if (a.trans != KSO_NONE && a.trans != (kso)kst_str) {
                /* Transform in place */
                for (k = 0; k < tmp->len; ++k) {
                    kso ob = tmp->elems[k];
                    kso obt = kso_call(a.trans, 1, &ob);
                    if (!obt) {
                        KS_DECREF(tmp);
                        KS_DECREF(res);
                        KS_DECREF(pos);
                        return NULL;
                    }
                    KS_DECREF(ob);
                    tmp->elems[k] = obt;
                }
            }

            if (a.num == 1) {
                ks_dict_set_h(res, (kso)a.name, a.name->v_hash, tmp->elems[0]);
            } else {
                ks_dict_set_h(res, (kso)a.name, a.name->v_hash, (kso)tmp);
            }
            KS_DECREF(tmp);
        }
        assert(i == pos->len);
    }

    KS_DECREF(pos);
    return res;
    #undef ARG
}


ks_str ksga_help(ksga_Parser self) {
    ksio_StringIO sio = ksio_StringIO_new();
    ksio_BaseIO aio = (ksio_BaseIO)sio;

    /* Generate usage message */
    int i, j, k, col;

    /* The 2 indentation levels it should generate at */
    int ind0 = 4, ind1 = 32;

    ksio_add(aio, "usage: %S [opts]", self->name);

    for (j = 0; j < self->n_pos; ++j) {
        struct ksga_pos a = self->pos[j];
        if (a.num < 0) {
            ksio_add(aio, " %S...", a.name);
        } else {
            ksio_add(aio, " ");
            for (k = 0; k < a.num; ++k) {
                if (k > 0) ksio_add(aio, " ");
                ksio_add(aio, "%S", a.name);
            }
            ksio_add(aio, "");
        }
    }

    ksio_add(aio, "\n\n");

    for (j = 0; j < self->n_pos; ++j) {
        struct ksga_pos a = self->pos[j];
        ksio_add(aio, "%.*c", ind0, ' ');
        col = ind0;

        ksio_add(aio, "%S", a.name);
        col += a.name->len_c;

        if (ind1 > col) ksio_add(aio, "%.*c", ind1 - col, ' ');

        ksio_add(aio, "%S\n", a.doc);
    }
    ksio_add(aio, "\nopts:\n");

    for (j = 0; j < self->n_flag; ++j) {
        struct ksga_flag a = self->flag[j];
        ksio_add(aio, "%.*c", ind0, ' ');
        col = ind0;
        for (k = 0; k < a.opts->len; ++k) {
            if (k > 0) {
                ksio_add(aio, ",");
                col++;
            }
            ks_str o = (ks_str)a.opts->elems[k];
            assert(kso_issub(o->type, kst_str));
            ksio_add(aio, "%S", o);
            col += o->len_c;
        }
        if (ind1 > col) ksio_add(aio, "%.*c", ind1 - col, ' ');

        ksio_add(aio, "%S\n", a.doc);
    }
    for (j = 0; j < self->n_opt; ++j) {
        struct ksga_opt a = self->opt[j];
        ksio_add(aio, "%.*c", ind0, ' ');
        col = ind0;
        for (k = 0; k < a.opts->len; ++k) {
            if (k > 0) {
                ksio_add(aio, ",");
                col++;
            }
            ks_str o = (ks_str)a.opts->elems[k];
            assert(kso_issub(o->type, kst_str));
            ksio_add(aio, "%S", o);
            col += o->len_c;
        }
        if (kso_issub(a.trans->type, kst_type)) {
            ksio_add(aio, "[=%S]", ((ks_type)a.trans)->i__name);
            col += 3 + ((ks_type)a.trans)->i__name->len_c;
        } else {
            ksio_add(aio, "[=str]");
            col += 6;
        }

        if (ind1 > col) ksio_add(aio, "%.*c", ind1 - col, ' ');

        ksio_add(aio, "%S", a.doc);

        if (a.defa) {
            ksio_add(aio, " (default: %S)", a.defa);
        }

        ksio_add(aio, "\n");
    }

    if (self->authors->len > 0) {
        ksio_add(aio, "\nauthors:\n");
        for (j = 0; j < self->authors->len; ++j) {
            ksio_add(aio, "%.*c%S\n", ind0, ' ', self->authors->elems[j]);
        }
    }

    ksio_add(aio, "version: %S\n", self->version);


    return ksio_StringIO_getf(sio);
}

/* Type functions */

static KS_TFUNC(T, free) {
    ksga_Parser self;
    KS_ARGS("self:*", &self, ksgat_Parser);

    KS_DECREF(self->name);
    KS_DECREF(self->doc);
    KS_DECREF(self->version);
    KS_DECREF(self->authors);

    int j;
    for (j = 0; j < self->n_flag; ++j) {
        struct ksga_flag a = self->flag[j];
        KS_DECREF(a.name);
        KS_DECREF(a.doc);
        KS_DECREF(a.opts);
        KS_DECREF(a.action);
    }
    for (j = 0; j < self->n_opt; ++j) {
        struct ksga_opt a = self->opt[j];
        KS_DECREF(a.name);
        KS_DECREF(a.doc);
        KS_DECREF(a.opts);
        KS_DECREF(a.trans);
        if (a.defa) KS_DECREF(a.defa);
    }
    for (j = 0; j < self->n_pos; ++j) {
        struct ksga_pos a = self->pos[j];
        KS_DECREF(a.name);
        KS_DECREF(a.doc);
        KS_DECREF(a.trans);
    }

    ks_free(self->opt);
    ks_free(self->pos);
    ks_free(self->flag);

    KSO_DEL(self);

    return KSO_NONE;
}

static KS_TFUNC(T, new) {
    ks_type tp;
    ks_str name, version, doc;
    kso authors;
    KS_ARGS("tp:* name:* version:* doc:* authors", &tp, kst_type, &name, kst_str, &version, kst_str, &doc, kst_str, &authors);
    
    ks_list la = NULL;
    if (kso_issub(authors->type, kst_str)) {
        la = ks_list_new(0, NULL);
        ks_list_push(la, authors);
    } else {
        la = ks_list_newi(authors);
    }
    if (!la) return NULL;

    ksga_Parser self = KSO_NEW(ksga_Parser, tp);
    self->stop_at_pos = false;

    self->n_pos = self->n_opt = self->n_flag = 0;
    self->pos = NULL;
    self->opt = NULL;
    self->flag = NULL;

    KS_INCREF(name);
    self->name = name;
    KS_INCREF(version);
    self->version = version;
    KS_INCREF(doc);
    self->doc = doc;

    self->authors = la;
    
    /* Add defaults */
    ksga_flag(self, "_help", "Prints this help/usage message and then exits", "-h,--help", action_help);
    ksga_flag(self, "_version", "Prints the version information and then exits", "--version", action_version);

    return (kso)self;
}


static KS_TFUNC(T, flag) {
    ksga_Parser self;
    ks_str name;
    kso opts;
    ks_str doc;
    kso action = KSO_NONE;
    KS_ARGS("self:* name:* opts doc:* ?action", &self, ksgat_Parser, &name, kst_str, &opts, &doc, kst_str, &action);

    ks_list la = NULL;
    if (kso_issub(opts->type, kst_str)) {
        la = ks_list_new(0, NULL);
        ks_list_push(la, opts);
    } else {
        la = ks_list_newi(opts);
    }
    if (!la) return NULL;

    int i = self->n_flag++;
    self->flag = ks_zrealloc(self->flag, sizeof(*self->flag), self->n_flag);

    KS_INCREF(name);
    self->flag[i].name = name;
    KS_INCREF(doc);
    self->flag[i].doc = doc;

    self->flag[i].opts = la;

    KS_INCREF(action);
    self->flag[i].action = action;

    return KSO_NONE;
}

static KS_TFUNC(T, opt) {
    ksga_Parser self;
    ks_str name;
    kso opts;
    ks_str doc;
    kso trans = (kso)kst_str;
    kso defa = NULL;
    KS_ARGS("self:* name:* opts doc:* ?trans ?defa", &self, ksgat_Parser, &name, kst_str, &opts, &doc, kst_str, &trans, &defa);

    ks_list la = NULL;
    if (kso_issub(opts->type, kst_str)) {
        la = ks_list_new(0, NULL);
        ks_list_push(la, opts);
    } else {
        la = ks_list_newi(opts);
    }
    if (!la) return NULL;

    int i = self->n_opt++;
    self->opt = ks_zrealloc(self->opt, sizeof(*self->opt), self->n_opt);

    KS_INCREF(name);
    self->opt[i].name = name;
    KS_INCREF(doc);
    self->opt[i].doc = doc;

    self->opt[i].opts = la;

    KS_INCREF(trans);
    self->opt[i].trans = trans;
    if (defa) KS_INCREF(defa);
    self->opt[i].defa = defa;

    return KSO_NONE;
}

static KS_TFUNC(T, pos) {
    ksga_Parser self;
    ks_str name;
    ks_str doc;
    ks_cint num = 1;
    kso trans = (kso)kst_str;
    KS_ARGS("self:* name:* doc:* ?num:cint ?trans", &self, ksgat_Parser, &name, kst_str, &doc, kst_str, &num, &trans);

    int i = self->n_pos++;
    self->pos = ks_zrealloc(self->pos, sizeof(*self->pos), self->n_pos);

    KS_INCREF(name);
    self->pos[i].name = name;
    KS_INCREF(doc);
    self->pos[i].doc = doc;

    self->pos[i].num = num;

    KS_INCREF(trans);
    self->pos[i].trans = trans;

    return KSO_NONE;
}


static KS_TFUNC(T, parse) {
    ksga_Parser parser;
    ks_list args = ksos_argv;
    KS_ARGS("self:* ?args:*", &parser, ksgat_Parser, &args, kst_list);

    ks_dict vals = ksga_parse(parser, args);
    if (!vals) return NULL;

    ks_names res = ks_names_new(vals, false);
    KS_DECREF(vals);

    return (kso)res;
}

static KS_TFUNC(T, on_help) {
    ksga_Parser parser;
    ks_str name, opt;
    KS_ARGS("self:* name:* opt:*", &parser, ksgat_Parser, &name, kst_str, &opt, kst_str);

    if (parser->help) {
        ksio_add((ksio_BaseIO)ksos_stderr, "%S", parser->help);
    } else {
        ks_str v = ksga_help(parser);
        ksio_add((ksio_BaseIO)ksos_stderr, "%S", v);
        KS_DECREF(v);
    }

    /* Exit peacefully */
    exit(0);
    return KSO_NONE;
}

static KS_TFUNC(T, on_version) {
    ksga_Parser parser;
    ks_str name, opt;
    KS_ARGS("self:* name:* opt:*", &parser, ksgat_Parser, &name, kst_str, &opt, kst_str);

    ksio_add((ksio_BaseIO)ksos_stderr, "%S\n", parser->version);

    /* Exit peacefully */
    exit(0);
    return KSO_NONE;
}



/* Export */

static struct ks_type_s tp;
ks_type ksgat_Parser = &tp;

void _ksi_getarg_Parser() {
    _ksinit(ksgat_Parser, kst_object, T_NAME, sizeof(struct ksga_Parser_s), -1, "Parses commandline arguments, similar to the 'getopt()' function in C", KS_IKV(
        {"__free",               ksf_wrap(T_free_, T_NAME ".__free(self)", "")},
        {"__new",                ksf_wrap(T_new_, T_NAME ".__new(tp, name, version, doc, authors)", "")},

        {"flag",                 ksf_wrap(T_flag_, T_NAME ".flag(self, name, opts, doc, action=none)", "Create a new flag for the parser\n\n    'opts' should contain option arguments, which begin with '-'")},
        {"opt",                  ksf_wrap(T_opt_, T_NAME ".opt(self, name, opts, doc, trans=str, defa=none)", "Create a new option argument for the parser\n\n    'opts' should contain option arguments, which begin with '-'")},
        {"pos",                  ksf_wrap(T_pos_, T_NAME ".pos(self, name, doc, num=1, trans=str)", "Create a new positional argument for the parser")},

        {"parse",                ksf_wrap(T_parse_, T_NAME ".parse(self, args=os.argv)", "Parse a list of arguments, returning a mapping which can be used to retrieve arguments")},

        {"on_help",              (action_help = ksf_wrap(T_on_help_, T_NAME ".on_help(self, name, opt)", "Handles when the builtin help option is passed"))},
        {"on_version",           (action_version = ksf_wrap(T_on_version_, T_NAME ".on_version(self, name, opt)", "Handles when the builtin version option is passed"))},
    ));
}
