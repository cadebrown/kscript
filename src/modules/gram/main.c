/* gram/main.c - source code for the built-in 'gram' module
 *
 * 
 * @author: Cade Brown <brown.cade@gmail.com>
 */

#include <ks/impl.h>
#include <ks/gram.h>

#define M_NAME "gram"

/* Module Functions */


ks_module _ksi_gram() {
    _ksi_gram_Lexer();
    _ksi_gram_Token();
    ks_module res = ks_module_new(M_NAME, KS_BIMOD_SRC, "'gram' - computer grammar utilities", KS_IKV(
        /* Types */
        {"Lexer",                  KS_NEWREF(ksgramt_Lexer)},
        {"Token",                  KS_NEWREF(ksgramt_Token)},

        /* Functions */


    ));

    return res;
}
