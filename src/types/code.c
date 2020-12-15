/* types/code.c - 'code' type
 *
 *
 * @author: Cade Brown <cade@kscript.org>
 */
#include <ks/impl.h>
#include <ks/compiler.h>

#define T_NAME "code"



/* C-API */

ks_code ks_code_new(ks_str fname, ks_str src) {
    ks_code self = KSO_NEW(ks_code, kst_code);

    KS_INCREF(fname);
    self->fname = fname;
    KS_INCREF(src);
    self->src = src;

    self->n_meta = 0;
    self->meta = NULL;

    self->vc = ks_list_new(0, NULL);
    self->vc_map = ks_dict_new(NULL);

    self->bc = ksio_BytesIO_new();

    return self;
}

ks_code ks_code_from(ks_code from) {
    ks_code self = KSO_NEW(ks_code, kst_code);

    KS_INCREF(from->fname);
    self->fname = from->fname;
    KS_INCREF(from->src);
    self->src = from->src;

    self->n_meta = 0;
    self->meta = NULL;

    /* Add a reference to the parent */
    KS_INCREF(from->vc);
    self->vc = from->vc;
    KS_INCREF(from->vc_map);
    self->vc_map = from->vc_map;

    self->bc = ksio_BytesIO_new();

    return self;
}

int ks_code_addconst(ks_code self, kso ob) {
    int i;
    for (i = 0; i < self->vc->len; ++i) {
        kso v = self->vc->elems[i];

        /* Determine whether the objects are equal in both type and value 
         * This is neccessary because we don't want 'true' to map to '1', since
         *   they are technically different
         */
        bool eq = ob == v;
        if (!eq) {
            eq = ob->type == v->type;
            if (eq) {
                if (!kso_eq(v, ob, &eq)) {
                    kso_catch_ignore();
                    eq = false;
                }
            }
        }

        if (eq) {
            /* Found a match */
            return i;
        }
    }


    /* Not found, so push it and return the last index */
    i = self->vc->len;
    ks_list_push(self->vc, ob);

    return i;
}

void ks_code_emit(ks_code self, ksb op) {
    ksio_addbuf((ksio_BaseIO)self->bc, sizeof(op), (const char*)&op);
}
void ks_code_emiti(ks_code self, ksb op, int arg) {
    ksba o;
    o.op = op;
    o.arg = arg;
    ksio_addbuf((ksio_BaseIO)self->bc, sizeof(o), (const char*)&o);
}
void ks_code_emito(ks_code self, ksb op, kso arg) {
    ks_code_emiti(self, op, ks_code_addconst(self, arg));
}

void ks_code_meta(ks_code self, ks_tok tok) {
    if (self->n_meta > 0 && self->meta[self->n_meta - 1].bc_n == self->bc->len_b) {
        return;
    }
    int i = self->n_meta++;
    self->meta = ks_zrealloc(self->meta, sizeof(*self->meta), self->n_meta);

    self->meta[i].bc_n = self->bc->len_b;
    self->meta[i].tok = tok;

    if (i > 0) {
        /* TODO: allow other cases? */
        assert(self->meta[i - 1].bc_n < self->meta[i].bc_n);
    }
}


bool ks_code_get_meta(ks_code self, int offset, struct ks_code_meta* meta) {
    if (offset < 0 || offset > self->bc->len_b) {
        KS_THROW(kst_Error, "Code meta requested for invalid offset %i", offset);
        return false;
    }
    if (self->n_meta < 1) {
        KS_THROW(kst_Error, "Code meta requested for code which had no meta", offset);
        return false;
    }

    /* TODO: binary search here */
    int fi = -1, i;
    for (i = 0; i < self->n_meta; ++i) {
        if (offset <= self->meta[i].bc_n) {
            fi = i;
            break;
        }
    }

    if (fi >= 0) {
        *meta = self->meta[fi];
        return true;
    } else {
        KS_THROW(kst_Error, "Code meta not found for offset %i", offset);
        return false;
    }
}


/* Type Functions */

static KS_TFUNC(T, free) {
    ks_code self;
    KS_ARGS("self:*", &self, kst_code);

    KS_DECREF(self->fname);
    KS_DECREF(self->src);

    KS_DECREF(self->vc);
    KS_DECREF(self->vc_map);

    KS_DECREF(self->bc);

    ks_free(self->meta);

    KSO_DEL(self);

    return KSO_NONE;
}

static KS_TFUNC(T, str) {
    ks_code self;
    KS_ARGS("self:*", &self, kst_code);

    return (kso)ks_fmt("<'%T' (%R) @ %p>", self, self->fname, self);    
}

static KS_TFUNC(T, dis) {
    ks_code self;
    KS_ARGS("self:*", &self, kst_code);

    ksio_StringIO sio = ksio_StringIO_new();

    ksio_add((ksio_BaseIO)sio, "# code \n# vc: %R\n", self->vc);

    int i = 0, sz = self->bc->len_b;
    ksb* bc = self->bc->data;
    while (i < sz) {
        ksba op;
        int p = i;
        if (i + sizeof(ksba) <= sz) {
            op = *(ksba*)(bc + i);
        } else {
            op.op = *(bc + i);
        }
        int o = op.op;
        int v = op.arg;

        #define OP(_o) else if (o == _o) { \
            i += sizeof(op.op); \
            ksio_add((ksio_BaseIO)sio, "%04i: %s\n", p, #_o + 4); \
        }
        #define OPI(_o) else if (o == _o) { \
            i += sizeof(op); \
            ksio_add((ksio_BaseIO)sio, "%04i: %s %i\n", p, #_o + 4, v); \
        }
        #define OPT(_o) else if (o == _o) { \
            i += sizeof(op); \
            ksio_add((ksio_BaseIO)sio, "%04i: %s %i # to %i\n", p, #_o + 4, v, i + v); \
        }
        #define OPV(_o) else if (o == _o) { \
            i += sizeof(op); \
            ksio_add((ksio_BaseIO)sio, "%04i: %s %i # %R\n", p, #_o + 4, v, self->vc->elems[v]); \
        }


        if (false) {} 

        OP(KSB_NOOP)
        OPV(KSB_PUSH)
        OP(KSB_POPU)
        OP(KSB_DUP)
        
        OPV(KSB_LOAD)
        OPV(KSB_STORE)
        
        OPV(KSB_GETATTR)
        OPV(KSB_SETATTR)
        OPI(KSB_GETELEMS)
        OPI(KSB_SETELEMS)
        OPI(KSB_CALL)
        OP(KSB_CALLV)

        OP(KSB_SLICE)
        OPI(KSB_LIST)
        OPI(KSB_LIST_PUSHN)
        OP(KSB_LIST_PUSHI)
        OPI(KSB_TUPLE)
        OPI(KSB_TUPLE_PUSHN)
        OP(KSB_TUPLE_PUSHI)

        OPI(KSB_SET)
        OPI(KSB_SET_PUSHN)
        OP(KSB_SET_PUSHI)
        OPV(KSB_FUNC)
        OPI(KSB_FUNC_DEFA)
        OPV(KSB_TYPE)

        OPT(KSB_JMP)
        OPT(KSB_JMPT)
        OPT(KSB_JMPF)

        OP(KSB_RET)
        OP(KSB_THROW)

        OP(KSB_FOR_START)
        OPT(KSB_FOR_NEXTT)
        OPT(KSB_FOR_NEXTF)
        OPT(KSB_TRY_START)
        OPT(KSB_TRY_CATCH)
        OPT(KSB_TRY_CATCH_ALL)
        OPT(KSB_TRY_END)
        OP(KSB_FINALLY_END)

        OP(KSB_BOP_IN)

        OP(KSB_BOP_EEQ)
        OP(KSB_BOP_EQ)
        OP(KSB_BOP_NE)
        OP(KSB_BOP_LT)
        OP(KSB_BOP_LE)
        OP(KSB_BOP_GT)
        OP(KSB_BOP_GE)

        OP(KSB_BOP_IOR)
        OP(KSB_BOP_XOR)
        OP(KSB_BOP_AND)
        OP(KSB_BOP_LSH)
        OP(KSB_BOP_RSH)
        OP(KSB_BOP_ADD)
        OP(KSB_BOP_SUB)
        OP(KSB_BOP_MUL)
        OP(KSB_BOP_DIV)
        OP(KSB_BOP_FLOORDIV)
        OP(KSB_BOP_MOD)
        OP(KSB_BOP_POW)

        OP(KSB_UOP_POS) 
        OP(KSB_UOP_NEG)
        OP(KSB_UOP_SQIG)
        OP(KSB_UOP_NOT)
            
        else {
            ksio_add((ksio_BaseIO)sio, "%4i: <err>\n", i);
            i += sizeof(op.op);
        }

    }

    return (kso)ksio_StringIO_getf(sio);
}



/* Export */

static struct ks_type_s tp;
ks_type kst_code = &tp;

void _ksi_code() {
    _ksinit(kst_code, kst_object, T_NAME, sizeof(struct ks_code_s), -1, "Bytecode object, which can be executed by the kscript virtual machine (vm)\n\n    This is an implementation detail, and the specifics of this type are dependent on the exact implementation and version", KS_IKV(
        {"__free",               ksf_wrap(T_free_, T_NAME ".__free(self)", "")},
        {"__str",                ksf_wrap(T_str_, T_NAME ".__str(self)", "")},
        {"__repr",               ksf_wrap(T_str_, T_NAME ".__repr(self)", "")},

        {"dis",                  ksf_wrap(T_dis_, T_NAME ".dis(self)", "Return a dis-assembled string")},
    ));
}
