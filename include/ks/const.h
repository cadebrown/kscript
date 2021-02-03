/* ks/const.h - Constants used in kscript
 * 
 * This file should contain all the magic-numbers, and hand-wavey explanations for why
 *   certain constants are using
 * 
 * @author:    Cade Brown <cade@kscript.org>
 */

#pragma once
#ifndef KS_CONST_H__
#define KS_CONST_H__

#ifndef KS_H__
#include <ks/ks.h>
#endif


/** Hashing Constants (TODO: detect 32 v 64 bit version) **/

/* Special values */
#define KS_HASH_INF    101
#define KS_HASH_NEGINF 102
#define KS_HASH_NAN    103

/* KS_HASH_P (also as 'import ks; print(ks.params.HASH_P)')
 * 
 * This is a modulus on which many types (including all numeric types) have their hash reduced via.
 * 
 * For example, the the hash of any rational number 'x' that can be written as 'a / b' is defined as:
 * 
 * hash(x) := (a * modinv(b, KS_HASH_P)) % KS_HASH_P
 * 
 * 
 * Obviously, it is very advantageous that these fit in a 'ks_hash_t' (and required for datatypes which calculate
 *   an actual index based on hash), but also advantageous to use one which allows a lot of of unique hashes.
 * 
 * I am very fond of Mersenne primes, so I am choosing the largest Mersenne prime that fits in a 'ks_hash_t'.
 *   So, on 32 bit platforms, 'M_31 = 2**31-1 = 2147483647' is used, and on 64 bit platforms, 
 *   'M_61 = 2**61-1 = 2305843009213693951' is used.
 *
 */
#define KS_HASH_P                  ((ks_hash_t)2147483647ULL)

/* Used in hashing sequence types,  
 * Formula:
 * hash(a[:]) = func {
 *   r = hash(type(a))
 *   for e in a {
 *     r = r * KS_HASH_MUL + KS_HASH_ADD + hash(e);
 *   }
 *   ret r
 * }()
 */
#define KS_HASH_ADD                ((ks_hash_t)3628273133LL)
#define KS_HASH_MUL                ((ks_hash_t)3367900313LL)


/** Implementation Constants **/

/* Maximum recursive call depth for a thread. If this is hit, then an error is thrown */
/* #define KS_MAX_CALL_DEPTH          16 */
#define KS_MAX_CALL_DEPTH          1024


/** Misc. Constants **/

/* Number to reset the reference count to for singletons that should never be freed */
#define KS_REFS_INF 0x10FFFFFFULL

/* The string to replace recursive 'repr()' contents when it is found to be self-recursive */
#define KS_REPR_SELF "..."

/* String for builtin modules to report as their source */
#define KS_BIMOD_SRC "<builtin>"

/* Symbol name expected for each C extension */
#define _KS_CEXTINIT_SYMBOL _ks_cextinit_this

/* Stringified version of the ext symbol */
#define _KS_CEXTINIT_SYMBOL_STR "_ks_cextinit_this"

/* Control+LineFeed */
#define KS_CRLF "\r\n"


#endif /* KS_CONST_H__ */
