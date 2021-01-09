/* ucd.h - header for the 'ucd' (Unicode database) module in kscript
 *
 * Defines functions to look up information based on codepoint and Unicode
 *   name
 *
 * @author: Cade Brown <cade@kscript.org>
 */

#pragma once
#ifndef KSUCD_H__
#define KSUCD_H__

#ifndef KS_H__
#include <ks/ks.h>
#endif

/* For now, in Windows, do ASCII only because the compiler maxes out at 16k string literals... */
#ifdef WIN32
  #define KSUCD_ASCII_ONLY
#endif


/** Types **/

/* Category of a Unicode character */
typedef char ksucd_cat;

/* Unicode categories */
enum {
    ksucd_cat_Lu = 1,
    ksucd_cat_Ll = 2,
    ksucd_cat_Lt = 3,
    ksucd_cat_LC = 4,
    ksucd_cat_Lm = 5,
    ksucd_cat_Lo = 6,
    ksucd_cat_L  = 7,
    
    ksucd_cat_Mn = 8,
    ksucd_cat_Mc = 9,
    ksucd_cat_Me = 10,
    ksucd_cat_M  = 11,
    
    ksucd_cat_Nd = 12,
    ksucd_cat_Nl = 13,
    ksucd_cat_No = 14,
    ksucd_cat_N  = 15,
    
    ksucd_cat_Pc = 16,
    ksucd_cat_Pd = 17,
    ksucd_cat_Ps = 18,
    ksucd_cat_Pe = 19,
    ksucd_cat_Pi = 20,
    ksucd_cat_Pf = 21,
    ksucd_cat_Po = 22,
    ksucd_cat_P  = 23,
    
    ksucd_cat_Sm = 24,
    ksucd_cat_Sc = 25,
    ksucd_cat_Sk = 26,
    ksucd_cat_So = 27,
    ksucd_cat_S  = 28,
    
    ksucd_cat_Zs = 29,
    ksucd_cat_Zl = 30,
    ksucd_cat_Zp = 31,
    ksucd_cat_Z  = 32,

    ksucd_cat_Cc = 33,
    ksucd_cat_Cf = 34,
    ksucd_cat_Cs = 35,
    ksucd_cat_Co = 36,
    ksucd_cat_Cn = 37,
    ksucd_cat_C  = 38,
};

struct ksucd_info {

    /* (0)
     * Codepoint index, the UCS-4/UTF-32 value
     */
    ks_ucp cp;

    /* (1)
     * Human readable name, may or may not be in '<>'
     * The string is NUL-terminated
     */
    const char* name;

    /* (2) 
     * General category, check 'ksucd_cat_*' values
     */
    ksucd_cat cat_gen;

    /* (3) 
     * Canonical combining classes
     */
    ksucd_cat cat_com;

    /* (4) 
     * Bidirectional category
     */
    ksucd_cat cat_bidi;

    /* (12, 13, 14) 
     * Cases for upper, lower, and title case
     */
    ks_ucp case_upper, case_lower, case_title;
};

/** Globals **/

/* Global array of character names, which is NUL-terminated per each,
 *   and double-NUL-terminated at the end of all names
 */
extern const char* ksucd_namestr;


/** Functions **/

/* Query for information about a codepoint, and store into '*info'
 */
ks_ucp ksucd_get_info(struct ksucd_info* info, ks_ucp cp);

/* Look up and search the database for a string. You can use 'len==-1' for a NUL-terminated string
 * EXAMPLE:
 * ksucd_lookup(&info, -1, "LATIN SMALL LETTER A") == 'a';
 */
ks_ucp ksucd_lookup(struct ksucd_info* info, int len, const char* name);


#endif /* KSUCD_H__ */
