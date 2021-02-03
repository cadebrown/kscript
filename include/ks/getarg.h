/* ks/getarg.h - builtin 'getarg' module for getting/parsing commandline argumnts
 *
 * Meant as a simple replacement of 'getopt', which supports things like type checking,
 *   conversions, and repeated arguments
 * 
 *
 * Useful as a general tool, but not overly extensible (cough cough, Python's argparse). I
 *   find that a tool that covers 98% of the use cases and is much simpler is a better design.
 *   In truth, adding overly complicated usecases that barely anyone uses over complicates everyones
 *   uses. I have to google everytime I want to parse arguments in Python. I think kscript should be different
 * 
 * Here's a basic example:
 * 
 * ```
 * #!/usr/bin/env ks
 * # ex1.ks - example 1
 * import getarg
 * 
 * p = getarg.Parser("ex1", "0.0.1", "Example 1, prints a single argument, optionally multiplied, optionally in hex", ["Cade Brown <cade@kscript.org>"])
 * 
 * # pos(name, doc, num=1, trans=str, defa=none)
 * # NOTE: this means we are accepting 1 'int', and the conversion will be automatic
 * p.pos("arg", "Single argument", 1, int)
 * 
 * # opt(name, opts, doc, trans=str, defa=none)
 * p.opt("fact", ["-m", "--mul"], "Number to multiply by", int, 1)
 * 
 * # flag(name, opts, doc, action=none)
 * p.flag("hex", ["--hex"], "If given, then output in hex")
 * 
 * # Now, parse, and throw an exception if required
 * args = p.parse()
 * 
 * # Notice we use '.arg', since we gave "arg" to the 'p.pos()' call
 * res = args.arg * args.fact
 * if args.hex {
 *     print (str(res, 16))
 * } else {
 *     print (res)
 * }
 * ```
 * 
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

KS_API_DATA ks_type
    ksgat_Parser
;

#endif /* KSGETARG_H__ */
