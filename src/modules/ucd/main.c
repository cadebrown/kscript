/* ucd/main.c - source code for the built-in 'ucd' module
 *
 * 
 * @author: Cade Brown <brown.cade@gmail.com>
 */

#include <ks/impl.h>
#include <ks/ucd.h>

#define M_NAME "ucd"

/* Constants */

static ks_str 
    cat_Lu,
    cat_Ll,
    cat_Lt,
    cat_LC,
    cat_Lm,
    cat_Lo,
    cat_L ,
    cat_Mn,
    cat_Mc,
    cat_Me,
    cat_M ,
    cat_Nd,
    cat_Nl,
    cat_No,
    cat_N ,
    cat_Pc,
    cat_Pd,
    cat_Ps,
    cat_Pe,
    cat_Pi,
    cat_Pf,
    cat_Po,
    cat_P ,
    cat_Sm,
    cat_Sc,
    cat_Sk,
    cat_So,
    cat_S ,
    cat_Zs,
    cat_Zl,
    cat_Zp,
    cat_Z ,
    cat_Cc,
    cat_Cf,
    cat_Cs,
    cat_Co,
    cat_Cn,
    cat_C
;
static ks_dict cat_map = NULL, cat_map_rev = NULL;

/* Utility Functions */

static KS_FUNC(lookup) {
    ks_str name;
    KS_ARGS("name:*", &name, kst_str);

    struct ksucd_info info;
    ks_ucp rcp = ksucd_lookup(&info, name->len_b, name->data);

    if (rcp < 0) {
        KS_THROW(kst_ValError, "Unable to find unicode character %R", rcp);
        return NULL;
    }

    return (kso)ks_str_chr(rcp);
}

static KS_FUNC(info) {
    ks_str c;
    KS_ARGS("chr:*", &c, kst_str);

    ks_cint cp = ks_str_ord(c);
    if (cp < 0) return NULL;

    struct ksucd_info info;
    ks_ucp rcp = ksucd_get_info(&info, cp);

    if (rcp < 0) {
        KS_THROW(kst_ValError, "Unable to find unicode data about %R", rcp);
        return NULL;
    }

    return (kso)ks_str_new(-1, info.name);
}

static KS_FUNC(name) {
    ks_str c;
    KS_ARGS("chr:*", &c, kst_str);

    ks_cint cp = ks_str_ord(c);
    if (cp < 0) return NULL;

    struct ksucd_info info;
    ks_ucp rcp = ksucd_get_info(&info, cp);

    if (rcp < 0) {
        KS_THROW(kst_ValError, "Unable to find unicode data about %R", rcp);
        return NULL;
    }

    return (kso)ks_str_new(-1, info.name);
}

static KS_FUNC(catgen) {
    ks_str c;
    KS_ARGS("chr:*", &c, kst_str);

    ks_cint cp = ks_str_ord(c);
    if (cp < 0) return NULL;

    struct ksucd_info info;
    ks_ucp rcp = ksucd_get_info(&info, cp);

    if (rcp < 0) {
        KS_THROW(kst_ValError, "Unable to find unicode data about %R", rcp);
        return NULL;
    }

    ks_int t = ks_int_new(info.cat_gen);
    kso res = ks_dict_get(cat_map, (kso)t);
    KS_DECREF(t);
    return res;    
}

ks_module _ksi_ucd() {

    cat_map = ks_dict_new(NULL);
    cat_map_rev = ks_dict_new(NULL);
    ks_int tmp;
    #define _CAT(_name) \
        cat_##_name = ks_str_new(-1, #_name); \
        tmp = ks_int_new(ksucd_cat_##_name); \
        ks_dict_set(cat_map, (kso)tmp, (kso)cat_##_name); \
        ks_dict_set(cat_map_rev, (kso)cat_##_name, (kso)tmp);

    _CAT(Lu)
    _CAT(Ll)
    _CAT(Lt)
    _CAT(LC)
    _CAT(Lm)
    _CAT(Lo)
    _CAT(L )
    _CAT(Mn)
    _CAT(Mc)
    _CAT(Me)
    _CAT(M )
    _CAT(Nd)
    _CAT(Nl)
    _CAT(No)
    _CAT(N )
    _CAT(Pc)
    _CAT(Pd)
    _CAT(Ps)
    _CAT(Pe)
    _CAT(Pi)
    _CAT(Pf)
    _CAT(Po)
    _CAT(P )
    _CAT(Sm)
    _CAT(Sc)
    _CAT(Sk)
    _CAT(So)
    _CAT(S )
    _CAT(Zs)
    _CAT(Zl)
    _CAT(Zp)
    _CAT(Z )
    _CAT(Cc)
    _CAT(Cf)
    _CAT(Cs)
    _CAT(Co)
    _CAT(Cn)
    _CAT(C)

    #ifdef KSUCD_ASCII_ONLY
    ks_warn("ucd", "kscript was compiled with support for ASCII only");
    #endif

    ks_module res = ks_module_new(M_NAME, KS_BIMOD_SRC, "'ucd' - unicode database ", KS_IKV(
        /* Types */

        /* Functions */

        {"lookup",                 ksf_wrap(lookup_, M_NAME ".lookup(name)", "Looks up a character by its Unicode name")},
        {"info",                   ksf_wrap(info_, M_NAME ".info(chr)", "Returns information about a unicode codepoint\n\n    'chr' should be a string of length 1")},
        {"name",                   ksf_wrap(name_, M_NAME ".name(chr)", "Returns the string name of a unicode character")},

        {"catgen",                 ksf_wrap(catgen_, M_NAME ".cat(chr)", "Returns the (string) general cateogory of a unicode character")},
        {"catmap",                 KS_NEWREF(cat_map)},
        {"catmaprev",              KS_NEWREF(cat_map_rev)},

    ));

    return res;
}
