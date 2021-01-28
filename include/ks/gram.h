/* ks/gram.h - computer grammar module, for lexical analysis, and parsing
 *
 * Essentially, this module combines flex, yacc, and other utilities, and is runtime
 *   specified, so it allows arbitrary grammars and is cross platform.
 * 
 * @author: Cade Brown <brown.cade@gmail.com>
 */

#pragma once
#ifndef KSGRAM_H__
#define KSGRAM_H__

#ifndef KS_H__
#include <ks/ks.h>
#endif /* KS_H__ */


/* 'gram.Token' - represents a token of text
 * 
 */
typedef struct ksgram_Token_s {
    KSO_BASE

    /* The kind of token, which is just a magic number (either int or enum value) */
    kso kind;

    /* The string value the token represents */
    ks_str val;

    /* The line and column at which it started */
    int sline, scol;
    int eline, ecol;

    /* The position (in bytes and characters) from the beginning of the stream */
    int pos_b, pos_c;

    /* The length (in bytes and characters) of the token */
    int len_b, len_c;

}* ksgram_Token;

/* 'gram.Lexer' - transforms a character stream into tokenized stream,
 *                       according to regex rules
 * 
 * All regexes are checked, and the longest match is the one that is used. If multiple
 *   matches are of the same length, then the one that appeared in the earlier rule takes
 *   precedence.
 * 
 * When a match is found, the corresponding action is ran. The return value is interpreted as:
 *   * 'none': skip this text
 *   * int/enum value: this is a token type, so construct a token from it
 *   * token: yield that token value directly
 *   * other: these will cause an error; don't return them
 * 
 * This class basically implements all the functionality that GNU Flex implements, but is reentrant,
 *   robust, handles unicode, and can change at runtime (instead of static code generation) 
 * 
 */
typedef struct ksgram_Lexer_s {
    KSO_BASE

    /* List of tuples (regex, action) of rules the lexer has */
    ks_list rules;

    /* The character input source for the Lexer 
     * May be a FileStream
     * TODO: add string lexing support
     */
    kso src;

    /* Number of characters read, but not yet claimed */
    int n_queue;
    /* Maximum size the queue has been allocated for */
    int _max_n_queue;

    /* List of characters being matched currently */
    char* queue;

    /* The line, column (in characters from start of line), and position (in bytes and characters from start of stream) */
    int line, col, pos_b, pos_c;


    /* Internal use, for simulating the Regex */
    ks_regex_sim0 _sim;

    int _max_sim_len;


}* ksgram_Lexer;



/* Functions */


/* Create a new token
 */
KS_API ksgram_Token ksgram_Token_new(ks_type tp, kso kind, ks_str val, ks_cint sline, ks_cint scol, ks_cint eline, ks_cint ecol, ks_cint pos_b, ks_cint pos_c, ks_cint len_b, ks_cint len_c);

/* Create a new Lexer
 */
KS_API ksgram_Lexer ksgram_Lexer_new(kso src);

/* Add a new lexer rule
 */
KS_API bool ksgram_Lexer_addrule(ksgram_Lexer self, ks_regex regex, kso action);



/* Types */

KS_API extern ks_type
    ksgramt_Token,
    ksgramt_Lexer
;



#endif /* KSGRAM_H__ */
