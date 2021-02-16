/* types/type.c - 'type' type
 *
 * @author: Cade Brown <cade@kscript.org>
 */
#include <ks/impl.h>

#define T_NAME "type"


/* C-API */



/* Initialize a type that has already been allocated or has memory */
void type_init(ks_type self, ks_type base, const char* name, int sz, int attr, const char* doc, struct ks_ikv* ikv, bool is_new) {
    if (is_new) {

    } else {
        /* May have been allocated in constant storage, so initialize that memory */
        memset(self, 0, sizeof(*self));
        self->refs = 1;
        KS_INCREF(kst_type);
        self->type = kst_type;

        self->attr = ks_dict_new(NULL);
    }

    #define ACT(_attr) self->i##_attr = base->i##_attr;
    _KS_DO_SPEC(ACT)
    #undef ACT

    /* Now, actually set up type  */

    self->num_obs_del = self->num_obs_new = 0;
    self->ob_sz = sz == 0 ? base->ob_sz : sz;
    self->ob_attr = attr == 0 ? base->ob_attr : attr;
    ks_type_set(self, _ksva__base, (kso)base);

    kso tmp = (kso)ks_str_new(-1, name);
    ks_type_set(self, _ksva__name, tmp);
    ks_type_set(self, _ksva__fullname, tmp);
    KS_DECREF(tmp);

    tmp = (kso)ks_str_new(-1, doc);
    ks_type_set(self, _ksva__doc, tmp);
    KS_DECREF(tmp);


    /* Add to 'subs' */
    if (self != base) {
        int idx = base->n_subs++;
        base->subs = ks_zrealloc(base->subs, sizeof(*base->subs), base->n_subs);
        base->subs[idx] = self;
    }

    /* Add attributes */
    if (ikv) {
        while (ikv->key) {
            ks_str k = ks_str_new(-1, ikv->key);
            ks_type_set(self, k, ikv->val);
            KS_DECREF(k);
            ikv++;
        }
    }

}

void _ksinit(ks_type self, ks_type base, const char* name, int sz, int attr, const char* doc, struct ks_ikv* ikv) {
    type_init(self, base, name, sz, attr, doc, ikv, false);
}


ks_type ks_type_new(const char* name, ks_type base, int sz, int attr_pos, const char* doc, struct ks_ikv* ikv) {
    ks_type self = KSO_NEW(ks_type, kst_type);

    type_init(self, base, name, sz, attr_pos, doc, ikv, true);

    return self;
}

ks_type ks_type_template(ks_type base, int nargs, kso* args) {
    if (base->i__template == NULL || base->i__template->len == 0) {
        KS_THROW(kst_TemplateError, "'%R' cannot be templated", base);
        return NULL;
    } else if (nargs == 0) {
        KS_THROW(kst_TemplateError, "'%R' cannot be templated, no template parameters given", base);
        return NULL;
    } else if (base->i__template->len != nargs) {
        KS_THROW(kst_TemplateError, "'%R' cannot be templated, wrong number of parameters (expected %i, but got %i)", base, (int)base->i__template->len, nargs);
        return NULL;
    }

    bool any_not_none = false;
    int i;
    for (i = 0; i < base->i__template->len; ++i) {
        if (base->i__template->elems[i] == KSO_NONE) {
        } else {
            any_not_none = true;
        }
    }

    if (any_not_none) {
        KS_THROW(kst_TemplateError, "'%R' cannot be templated, it is already a template (try getting '.__base' for the template class)", base);
        return NULL;
    }


    /* Template cache */
    static ks_dict my_tcache = NULL;
    if (my_tcache == NULL) my_tcache = ks_dict_new(NULL);

    /* Create template arguments */
    ks_tuple ta = ks_tuple_new(nargs, args);

    /* Try cache for key */
    ks_tuple key = ks_tuple_new(2, (kso[]){ (kso)base, (kso)ta });
    
    ks_type cv = (ks_type)ks_dict_get(my_tcache, (kso)key);
    if (cv) {
        return cv;
    }

    kso_catch_ignore();

    /* Now, create it */
    ks_str name = ks_fmt("%R[%J]", base, ", ", nargs, args);
    cv = ks_type_new(name->data, base, 0, 0, "", KS_IKV(
        {"__template", (kso)ta},
    ));
    KS_DECREF(name);

    /* Store in cache */
    ks_dict_set(my_tcache, (kso)key, (kso)cv);

    KS_DECREF(key);

    return cv;
}


kso ks_type_get(ks_type self, ks_str attr) {
    kso res = ks_dict_get_ih(self->attr, (kso)attr, attr->v_hash);
    if (res) return res;

    if (self->i__base != self) return ks_type_get(self->i__base, attr);
    else {
        KS_THROW_ATTR(self, attr);
        return NULL;
    }
}

bool ks_type_set(ks_type self, ks_str attr, kso val) {
    if (attr->len_b > 2 && attr->data[0] == '_' && attr->data[1] == '_') {
        /* Handle special names */
        #define ACT(_attr) else if (ks_str_eq_c(attr, #_attr, sizeof(#_attr) - 1)) { \
            *(kso*)&self->i##_attr = val; \
        }
        if (false) {}
        _KS_DO_SPEC(ACT)
        #undef ACTss
    }

    ks_dict_set_h(self->attr, (kso)attr, attr->v_hash, val);
    return true;
}

bool ks_type_set_c(ks_type self, const char* attr, kso val) {
    ks_str k = ks_str_new(-1, attr);
    bool res = ks_type_set(self, k, val);
    KS_DECREF(k);
    return res;
}

/* Type Functions */

static KS_TFUNC(T, free) {
    ks_type self;
    KS_ARGS("self:*", &self, kst_type);

    KSO_DEL(self);

    return KSO_NONE;
}

static KS_TFUNC(T, getelem) {
    ks_type self;
    int nargs;
    kso* args;
    KS_ARGS("self:* *args", &self, kst_type, &nargs, &args);

    return (kso)ks_type_template(self, nargs, args);
}

static KS_TFUNC(T, getattr) {
    ks_type self;
    ks_str attr;
    KS_ARGS("self:* attr:*", &self, kst_type, &attr, kst_str);

    return ks_type_get(self, attr);
}
static KS_TFUNC(T, setattr) {
    ks_type self;
    ks_str attr;
    kso val;
    KS_ARGS("self:* attr:* val", &self, kst_type, &attr, kst_str, &val);

    if (!ks_type_set(self, attr, val)) return NULL;

    return KS_NEWREF(val);
}



static bool buildgraph(ks_type self, ks_graph res) {
    if (!ks_graph_add_node(res, (kso)self, false)) {
        return false;
    }
    int i;
    for (i = 0; i < self->n_subs; ++i) {
        if (!buildgraph(self->subs[i], res)) return false;

        if (!ks_graph_add_edge(res, (kso)self, (kso)self->subs[i], KSO_NONE, false)) {
            return false;
        }
    }

    return true;
}

static KS_TFUNC(T, graph) {
    ks_type self;
    KS_ARGS("self:*", &self, kst_type);

    ks_graph res = ks_graph_new(kst_graph);
    if (!res) return NULL;

    /* Generate dependency graph from type hierarchy */

    if (!buildgraph(self, res)) {
        KS_DECREF(res);
        return NULL;
    }

    /* Build a path up to object */
    ks_type it = self, lit;
    do {
        lit = it;
        it = it->i__base;

        ks_graph_add_node(res, (kso)it, false);
        ks_graph_add_edge(res, (kso)it, (kso)lit, KSO_NONE, false);

    }  while (it != kst_object);

    return (kso)res;
}


/* Export */

static struct ks_type_s tp;
ks_type kst_type = &tp;

void _ksi_type() {
    _ksinit(kst_type, kst_object, T_NAME, sizeof(struct ks_type_s), offsetof(struct ks_type_s, attr), "Represents a type, which is a descriptor of objects which are instances of the type", KS_IKV(
        {"__free",                 ksf_wrap(T_free_, T_NAME ".__free(self)", "")},
        {"__getelem",              ksf_wrap(T_getelem_, T_NAME ".__getelem(self, *args)", "")},
        {"__graph",                ksf_wrap(T_graph_, T_NAME ".__graph(self)", "")},
        {"__getattr",              ksf_wrap(T_getattr_, T_NAME ".__getattr(self, attr)", "")},
        {"__setattr",              ksf_wrap(T_setattr_, T_NAME ".__setattr(self, attr, val)", "")},


    ));
}
