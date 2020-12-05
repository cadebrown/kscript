/* types/ast.c - 'ast' type
 *
 *
 * TODO: add enum
 * 
 * @author: Cade Brown <cade@kscript.org>
 */
#include <ks/impl.h>
#include <ks/compiler.h>

#define T_NAME "ast"




/* Internals */

/* Enum of the kinds of ASTs */
static ks_type E_kind = NULL;


/* C-API */

ks_ast ks_ast_new(int kind, int n_args, ks_ast* args, kso val, ks_tok tok) {
    if (!val) val = KSO_NONE;

    ks_ast self = KSO_NEW(ks_ast, kst_ast);

    self->kind = kind;

    self->args = ks_list_new(n_args, (kso*)args);
    KS_INCREF(val);
    self->val = val;

    self->tok = tok;

    return self;
}

ks_ast ks_ast_newn(int kind, int n_args, ks_ast* args, kso val, ks_tok tok) {
    if (!val) val = KSO_NONE;
    ks_ast res = ks_ast_new(kind, n_args, args, val, tok);
    KS_DECREF(val);
    int i;
    for (i = 0; i < n_args; ++i) {
        KS_DECREF(args[i]);
    }

    return res;
}


void ks_ast_push(ks_ast self, ks_ast sub) {
    ks_list_push(self->args, (kso)sub);
    self->tok = ks_tok_combo(self->tok, sub->tok);
}
void ks_ast_pushn(ks_ast self, ks_ast sub) {
    ks_ast_push(self, sub);
    KS_DECREF(sub);
}

/* Type Functions */


static KS_TFUNC(T, free) {
    ks_ast self;
    KS_ARGS("self:*", &self, kst_ast);

    KS_DECREF(self->args);
    KS_DECREF(self->val);

    KSO_DEL(self);

    return KSO_NONE;
}

static KS_TFUNC(T, str) {
    ks_ast self;
    KS_ARGS("self:*", &self, kst_ast);


    ks_enum k = ks_enum_get(E_kind, self->kind);
    ks_str res = NULL;
    assert(k);
    if (self->args->len == 0 && !kso_issub(self->val->type, kst_list)) {
        res = ks_fmt("%T(%R.%S, %R)", self, E_kind, k->name, self->val);
    } else {
        if (self->val == KSO_NONE) {
            res = ks_fmt("%T(%R.%S, %R)", self, E_kind, k->name, self->args);
        } else {
            res = ks_fmt("%T(%R.%S, %R, %R)", self, E_kind, k->name, self->args, self->val);
        }
    }

    KS_DECREF(k);

    return (kso)res;
}

/* Export */

static struct ks_type_s tp;
ks_type kst_ast = &tp;

void _ksi_ast() {
    E_kind = ks_enum_make(T_NAME, KS_EIKV(
    {"CONST", KS_AST_CONST},
    {"NAME", KS_AST_NAME},
    {"LIST", KS_AST_LIST},
    {"TUPLE", KS_AST_TUPLE},
    {"SET", KS_AST_SET},
    {"DICT", KS_AST_DICT}, 
    {"CALL", KS_AST_CALL}, 
    {"ATTR", KS_AST_ATTR}, 
    {"ELEM", KS_AST_ELEM}, 
    {"COND", KS_AST_COND}, 
    {"FUNC", KS_AST_FUNC},
    {"TYPE", KS_AST_TYPE},
    {"IMPORT", KS_AST_IMPORT},
    {"CONT", KS_AST_CONT},
    {"BREAK", KS_AST_BREAK},
    {"RET", KS_AST_RET},
    {"THROW", KS_AST_THROW},
    {"BLOCK", KS_AST_BLOCK},
    {"IF", KS_AST_IF},
    {"WHILE", KS_AST_WHILE},
    {"FOR", KS_AST_FOR},
    {"TRY", KS_AST_TRY},
    {"BOP_ASSIGN", KS_AST_BOP_ASSIGN},
    {"BOP_AAND", KS_AST_BOP_AAND}, 
    {"BOP_AXOR", KS_AST_BOP_AXOR}, 
    {"BOP_AIOR", KS_AST_BOP_AIOR}, 
    {"BOP_ALSH", KS_AST_BOP_ALSH}, 
    {"BOP_ARSH", KS_AST_BOP_ARSH}, 
    {"BOP_AADD", KS_AST_BOP_AADD}, 
    {"BOP_ASUB", KS_AST_BOP_ASUB}, 
    {"BOP_AMUL", KS_AST_BOP_AMUL}, 
    {"BOP_ADIV", KS_AST_BOP_ADIV}, 
    {"BOP_AFLOORDIV", KS_AST_BOP_AFLOORDIV}, 
    {"BOP_AMOD", KS_AST_BOP_AMOD}, 
    {"BOP_APOW", KS_AST_BOP_APOW}, 
    {"BOP_QUESQUES", KS_AST_BOP_QUESQUES}, 
    {"BOP_OROR", KS_AST_BOP_OROR}, 
    {"BOP_ANDAND", KS_AST_BOP_ANDAND}, 
    {"BOP_IN", KS_AST_BOP_IN}, 
    {"BOP_EEQ", KS_AST_BOP_EEQ}, 
    {"BOP_EQ", KS_AST_BOP_EQ}, 
    {"BOP_NE", KS_AST_BOP_NE}, 
    {"BOP_LT", KS_AST_BOP_LT}, 
    {"BOP_LE", KS_AST_BOP_LE}, 
    {"BOP_GT", KS_AST_BOP_GT}, 
    {"BOP_GE", KS_AST_BOP_GE}, 
    {"BOP_IOR", KS_AST_BOP_IOR}, 
    {"BOP_XOR", KS_AST_BOP_XOR}, 
    {"BOP_AND", KS_AST_BOP_AND}, 
    {"BOP_LSH", KS_AST_BOP_LSH}, 
    {"BOP_RSH", KS_AST_BOP_RSH}, 
    {"BOP_ADD", KS_AST_BOP_ADD}, 
    {"BOP_SUB", KS_AST_BOP_SUB}, 
    {"BOP_MUL", KS_AST_BOP_MUL}, 
    {"BOP_DIV", KS_AST_BOP_DIV}, 
    {"BOP_FLOORDIV", KS_AST_BOP_FLOORDIV}, 
    {"BOP_MOD", KS_AST_BOP_MOD}, 
    {"BOP_POW", KS_AST_BOP_POW}, 
    {"UOP_POS", KS_AST_UOP_POS}, 
    {"UOP_NEG", KS_AST_UOP_NEG}, 
    {"UOP_SQIG", KS_AST_UOP_SQIG}, 
    {"UOP_POSPOS", KS_AST_UOP_POSPOS}, 
    {"UOP_POSPOS_POST", KS_AST_UOP_POSPOS_POST}, 
    {"UOP_NEGNEG", KS_AST_UOP_NEGNEG}, 
    {"UOP_NEGNEG_POST", KS_AST_UOP_NEGNEG_POST}, 
    {"UOP_NOT", KS_AST_UOP_NOT}, 
    {"UOP_QUES", KS_AST_UOP_QUES}, 
    {"UOP_STAR", KS_AST_UOP_STAR}, 
    ));

    _ksinit(kst_ast, kst_object, T_NAME, sizeof(struct ks_ast_s), -1, KS_IKV(
        {"__free",               ksf_wrap(T_free_, T_NAME ".__free(self)", "")},
        {"__str",                ksf_wrap(T_str_, T_NAME ".__str(self)", "")},
        {"__repr",               ksf_wrap(T_str_, T_NAME ".__repr(self)", "")},
    ));
}
