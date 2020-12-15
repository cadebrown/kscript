/* ks/getarg.h - builtin 'getarg' module for getting/parsing commandline argumnts
 *
 * Meant as a simple replacement of 'getopt', which supports things like type checking,
 *   conversions, and repeated arguments
 * 
 * @author: Cade Brown <brown.cade@gmail.com>
 */

#pragma once
#ifndef KSGETARG_H__
#define KSGETARG_H__

#ifndef KS_H__
#include <ks/ks.h>
#endif /* KS_H__ */

/* Types */

/* 'getarg.Parser' - parses commandline elements
 *
 * This is the default parser that should be used in most 
 */
typedef struct ksga_Parser_s {
    KSO_BASE

    /* Meta data */
    ks_str name, version, doc;
    ks_list authors;

    /* Help message. If NULL, it will be generated automatically when it is needed */
    ks_str help;

    /* If true, then stop parsing options at the first positional argument */
    bool stop_at_pos;


    /* Number of flags */
    int n_flag;

    /* Flag arguments, which don't take an argument, and just record
     *   the number of times they were encountered
     */
    struct ksga_flag {
        ks_str name;
        ks_str doc;

        /* Options that match the flag:
         * ['-f', '--flag']
         */
        ks_list opts;

        /* Action to take when this flag is encountered 
         * It will be called like a function, like 'action(parser, name, opt)'
         */
        kso action;

    }* flag;

    /* Number of options */
    int n_opt;


    /* Option arguments, which take an option string prefixing them
     *
     */
    struct ksga_opt {
        ks_str name;
        ks_str doc;

        /* Strings that match the option:
         * ['-o', '--opt']
         */
        ks_list opts;

        /* Transformation applied to the argument, which can be a type (most commonly) */
        kso trans;

        /* Default, or NULL for required */
        kso defa;

    }* opt;


    /* Number of positional arguments */
    int n_pos;

    /* Positional arguments, which don't have 
     *
     */
    struct ksga_pos {
        ks_str name;
        ks_str doc;

        /* Transformation applied to the argument, which can be a type (most commonly) */
        kso trans;

        /* The number of times it should appear in the arguments 
         * If negative, it should be the last positional argument, and it will absorb all the rest.
         * Otherwise, a positive number telling how many to consume
         */
        int num;

    }* pos;


}* ksga_Parser;

/* Create a new 'getarg.Parser'
 */
KS_API ksga_Parser ksga_Parser_new(const char* name, const char* doc, const char* version, const char* authors);

/* Add a flag to the parser
 */
KS_API void ksga_flag(ksga_Parser self, const char* name, const char* doc, const char* opts, kso action);

/* Add a option argument to the parser
 */
KS_API void ksga_opt(ksga_Parser self, const char* name, const char* doc, const char* opts, kso trans, kso defa);

/* Add a positional argument to the parser
 */
KS_API void ksga_pos(ksga_Parser self, const char* name, const char* doc, kso trans, int num);

/* Parse a list of (string) arguments, and return the resulting dictionary of arguments
 * if 'NULL' is returned, an error is thrown (which should contain usage information)
 */
KS_API ks_dict ksga_parse(ksga_Parser self, ks_list args);

/* Generates a help string from a parser
 */
KS_API ks_str ksga_help(ksga_Parser self);

/* Exported */

KS_API extern ks_type
    ksgat_Parser
;

#endif /* KSGETARG_H__ */
