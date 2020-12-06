/* ks/compiler.h - kscript compiler
 * 
 * This module and C-API is meant to transform kscript source (in strings) into
 *   Tokens, ASTs (Abstract Syntax Trees), and also bytecodes
 * 
 * @author:    Cade Brown <cade@kscript.org>
 */

#pragma once
#ifndef KS_COMPILER_H__
#define KS_COMPILER_H__

#ifndef KS_H__
#include <ks/ks.h>
#endif


/* Types */

/* Kinds of tokens */
enum {
    /* EOF, end of file/end of input */
    KS_TOK_EOF = 0,

    /* Combination of other tokens */
    KS_TOK_MANY,

    /* Name/identifier (as defined by Unicode)
     *
     * regex:
     *   [[:alpha:]_][[:alpha:]_[:digit:]]*
     * 
     */
    KS_TOK_NAME,

    /* Integer constant
     *
     * This will match any integer with a prefix (optional) and a non-zero number of 
     *   digits afterwards
     * 
     * It also does not match right before or after a '.', which would cause a 'KS_TOK_FLOAT'
     *   match anyway
     * 
     * regex:
     *   (0[bB])?[0-1]+
     *   (0[oO])?[0-7]+
     *   (0[dD])?[0-9]+
     *   (0[xX])?[0-9a-fA-F]+
     * 
     */
    KS_TOK_INT,

    /* Floating point constant
     *
     * Does not match integers, only matches those with an explicit point ('.') somewhere
     *   in the constant. Also allows an imaginary suffix ('i' or 'I'), which turns it into
     *   a complex (imaginary-only) constant.
     * 
     * regex:
     *   (0[bB])?(\.[0-1]+|[0-1]+\.[0-1]*)[iI]?
     *   (0[oO])?(\.[0-7]+|[0-7]+\.[0-7]*)[iI]?
     *   (0[dD])?(\.[0-9]+|[0-9]+\.[0-9]*)[iI]?
     *   (0[xX])?(\.[0-9a-fA-F]+|[0-9a-fA-F]+\.[0-9a-fA-F]*)[iI]?
     * 
     */
    KS_TOK_FLOAT,


    /* String literal, beginning with """, ''', or a single quote (and ending with it)
     *
     * Escape codes:
     *  \\
     *  \'
     *  \"
     *  \a
     *  \b
     *  \f
     *  \n
     *  \r
     *  \t
     *  \v
     *  \xXX - literal byte (in hex)
     *  \uXXXX - 16bit Unicode codepoint (in hex)
     *  \UXXXXXXXX - 32bit Unicode codepoint (in hex)
     *  \N[XX...X] - Unicode name lookup, like: '\N[LATIN CAPITAL LETTER A]' == 'A'
     */
    KS_TOK_STR,

    /* Regular expression (regex) literal, beginning and ending with '`'. It allows the
     *   same
     *
     * 
     */
    KS_TOK_REGEX,

    /** Keywords **/

    KS_TOK_IMPORT,
    KS_TOK_ASSERT,
    KS_TOK_THROW,
    KS_TOK_RET,
    KS_TOK_BREAK,
    KS_TOK_CONT,
    KS_TOK_IF,
    KS_TOK_ELIF,
    KS_TOK_ELSE,
    KS_TOK_WHILE,
    KS_TOK_FOR,
    KS_TOK_TRY,
    KS_TOK_CATCH,

    /** Literal Characters **/

    KS_TOK_N, /* <newline> */
    KS_TOK_DOTDOTDOT, /* . */
    KS_TOK_DOT, /* ... */
    KS_TOK_COM, /* , */
    KS_TOK_COL, /* : */
    KS_TOK_SEMI, /* ; */

    KS_TOK_LPAR, /* ( */
    KS_TOK_RPAR, /* ) */
    KS_TOK_LBRK, /* [ */
    KS_TOK_RBRK, /* ] */
    KS_TOK_LBRC, /* { */
    KS_TOK_RBRC, /* } */
    KS_TOK_LARW, /* <- */
    KS_TOK_RARW, /* -> */

    KS_TOK_ASSIGN, /* = */
    KS_TOK_AIOR, /* |= */
    KS_TOK_AXOR, /* ^= */
    KS_TOK_AAND, /* &= */
    KS_TOK_ALSH, /* <<= */
    KS_TOK_ARSH, /* >>= */
    KS_TOK_AADD, /* += */
    KS_TOK_ASUB, /* -= */
    KS_TOK_AMUL, /* *= */
    KS_TOK_ADIV, /* /= */
    KS_TOK_AFLOORDIV, /* //= */
    KS_TOK_AMOD, /* %= */
    KS_TOK_APOW, /* **= */

    KS_TOK_QUESQUES, /* ?? */

    KS_TOK_OROR, /* || */
    
    KS_TOK_ANDAND, /* && */

    KS_TOK_IN, /* in */

    KS_TOK_EEQ, /* === */
    KS_TOK_EQ,  /* == */
    KS_TOK_NE,  /* != */
    KS_TOK_LT,  /* < */
    KS_TOK_GT,  /* > */
    KS_TOK_LE,  /* <= */
    KS_TOK_GE,  /* >= */

    KS_TOK_IOR, /* | */

    KS_TOK_XOR, /* ^ */

    KS_TOK_AND, /* & */

    KS_TOK_LSH, /* << */
    KS_TOK_RSH, /* >> */

    KS_TOK_ADD, /* + */
    KS_TOK_SUB, /* - */

    KS_TOK_MUL, /* * */
    KS_TOK_DIV, /* / */
    KS_TOK_FLOORDIV, /* // */
    KS_TOK_MOD, /* % */

    KS_TOK_POW, /* ** */


    KS_TOK_ADDADD, /* ++ */
    KS_TOK_SUBSUB, /* -- */
    KS_TOK_SQIG, /* ~ */
    KS_TOK_NOT, /* ! */
    KS_TOK_QUES, /* ? */

};

/* Source token, representing a span of text 
 *
 *
 */
typedef struct {

    /* Kind of token */
    int kind;

    /* Starting position */
    int sline, scol, spos;

    /* Ending position */
    int eline, ecol, epos;

} ks_tok;

/* Empty/invalid token */
#define KS_TOK_MAKE_EMPTY() ((ks_tok){ 0, -1, -1, -1, -1, -1, -1 })


enum {
    /* Constant expression, of any type. The value is stored in '.val' */
    KS_AST_CONST = 1,

    /* Name/identifier. The string name is stored in '.val' */
    KS_AST_NAME,

    /** Constructors **/
    KS_AST_LIST,
    KS_AST_TUPLE,
    KS_AST_SET,
    KS_AST_DICT, /* args = [k, v, k, v, ...] */

    /** Operations **/
    KS_AST_CALL, /* args = [func, *callargs] */
    KS_AST_ATTR, /* args = [self, attr] */
    KS_AST_ELEM, /* args = [self, *idxs] */

    /* Conditional expression
     * a if b else c
     */
    KS_AST_COND, /* args = [cond, iftrue, iffalse] */

    /* Rich comparison, involving comparison operators
     *
     * args = [*vals]
     * val = (*ops,)
     * len(args) == len(val) + 1
     */
    KS_AST_RICHCMP,

    /* Function constructor
     *
     * args = [name, (*params), body]
     */
    KS_AST_FUNC,

    /* Type constructor
     *
     * args = [name, extends, body]
     */
    KS_AST_TYPE,

    /* Import statement
     *
     * If len(args) == 1, then it is just 'import name'
     * TODO: add others
     */
    KS_AST_IMPORT,

    /** Control Flow **/

    /* Continue statement (cont) */
    KS_AST_CONT,

    /* Break statement (break) */
    KS_AST_BREAK,

    /* Return statement (ret args[0]) */
    KS_AST_RET,

    /* Throw statement (throw args[0]) */
    KS_AST_THROW,


    /** Blocks **/

    /* Block statement
     * 
     * {
     *   *args
     * }
     * 
     */
    KS_AST_BLOCK,

    /* 'if' statement
     *
     * 'elif' clauses should be folded into the 'else' branch
     *
     * if args[0] {
     *   args[1]
     * } else {
     *   args[2]
     * }
     */
    KS_AST_IF,

    /* 'while' statement
     *
     * while args[0] {
     *   args[1]
     * } else {
     *   args[2]
     * }
     */
    KS_AST_WHILE,

    /* 'for' statement
     *
     * for args[0] in args[1] {
     *   args[1]
     * }
     */
    KS_AST_FOR,

    /* 'try' statement
     * 
     * try {
     *   args[0]
     * } catch args[1] as args[2] {
     *   args[3]
     * ...
     * } catch args[3*i+1] as args[3*i+2] {
     *   args[3*i+3]
     * ...
     * } finally {
     *   args[-1]
     * }
     * 
     */
    KS_AST_TRY,


    /** Binary Operators **/
    
    KS_AST_BOP_ASSIGN = 50, /* x=y */
    KS_AST_BOP_AIOR, /* x|=y */
    KS_AST_BOP_AXOR, /* x^=y */
    KS_AST_BOP_AAND, /* x&=y */
    KS_AST_BOP_ALSH, /* x<<=y */
    KS_AST_BOP_ARSH, /* x>>=y */
    KS_AST_BOP_AADD, /* x+=y */
    KS_AST_BOP_ASUB, /* x-=y */
    KS_AST_BOP_AMUL, /* x*=y */
    KS_AST_BOP_ADIV, /* x/=y */
    KS_AST_BOP_AFLOORDIV, /* x//=y */
    KS_AST_BOP_AMOD, /* x%=y */
    KS_AST_BOP_APOW, /* x**=y */

    KS_AST_BOP_QUESQUES, /* x ?? y */

    KS_AST_BOP_OROR, /* x || y */
    
    KS_AST_BOP_ANDAND, /* x && y */

    KS_AST_BOP_IN, /* x in y */

    KS_AST_BOP_EEQ, /* x===y */
    KS_AST_BOP_EQ, /* x==y */
    KS_AST_BOP_NE, /* x!=y */
    KS_AST_BOP_LT, /* x<y */
    KS_AST_BOP_LE, /* x<=y */
    KS_AST_BOP_GT, /* x>y */
    KS_AST_BOP_GE, /* x>=y */


    KS_AST_BOP_IOR, /* x|y */

    KS_AST_BOP_XOR, /* x^y */

    KS_AST_BOP_AND, /* x&y */


    KS_AST_BOP_LSH, /* x<<y */
    KS_AST_BOP_RSH, /* x>>y */

    KS_AST_BOP_ADD, /* x+y */
    KS_AST_BOP_SUB, /* x-y */

    KS_AST_BOP_MUL, /* x(y */
    KS_AST_BOP_DIV, /* x/y */
    KS_AST_BOP_FLOORDIV, /* x//y */
    KS_AST_BOP_MOD, /* x%y */

    KS_AST_BOP_POW, /* x**y */


    /** Unary Operators **/

    KS_AST_UOP_POS = 100, /* +x */ 
    KS_AST_UOP_NEG, /* -x */
    KS_AST_UOP_SQIG, /* ~x */
    KS_AST_UOP_POSPOS, /* ++x */
    KS_AST_UOP_POSPOS_POST, /* x++ */
    KS_AST_UOP_NEGNEG, /* --x */
    KS_AST_UOP_NEGNEG_POST, /* x-- */
    KS_AST_UOP_NOT, /* !x */
    KS_AST_UOP_QUES, /* *x */
    KS_AST_UOP_STAR /* ?x */

};

#define KS_AST_BOP__FIRST KS_AST_BOP_ASSIGN
#define KS_AST_BOP__LAST KS_AST_BOP_POW

/* 'ast' - Abstract Syntax Tree, representing a program in abstract terms
 *
 */
typedef struct ks_ast_s {
    KSO_BASE

    /* Kind of AST, see 'KS_AST_*' */
    int kind;

    /* List of sub nodes. All elements are ASTs here */
    ks_list args;

    /* Value, which is dependent on the specific type of AST */
    kso val;

    /* Token describing the part of the input which the AST represents (from the parser) */
    ks_tok tok;

}* ks_ast;


/* Bytecode Instructions
 *
 * Bytecode instructions in kscript (for the virtual machine) are meant to be fast and efficient,
 *   and also easy to generate. Therefore, we have two variants: those which take no arguments (opcode
 *   only) and those which take a single integer (opcode+argument)
 *
 * Bytecode can reference the 'code' object it was created within, for example the const.
 * 
 * The bytecode definitions reference 'vc', which is the code's 'vc' object, which is a constants array
 *   (i.e. value cache). They also reference 'stk', which is the program stack
 * 
 * 
 */

enum {
    /* NOOP
     *
     * Do nothing
     */
    KSB_NOOP = 0,

    /* PUSH idx
     * 
     * Pushes 'vc[idx]' on to 'stk'
     */
    KSB_PUSH,

    /* POPU
     *
     * Pops (and removes reference) an element off of 'stk'
     */
    KSB_POPU,

    /* LOAD idx
     * 
     * Loads 'vc[idx]' (as a dynamic name) and pushes the result on to 'stk'
     */
    KSB_LOAD,

    /* STORE idx
     *
     * Takes 'top(stk)' (but does not pop it) and stores it into 'vc[idx]' (as a dynamic
     *   name)
     */
    KSB_STORE,

    /* CALL num
     *
     * Takes the top 'num' elements, and performs a function call, the bottom-most element
     *   is the function being called and the rest are the arguments (so num should be at least
     *   1)
     */
    KSB_CALL,


    /** Control Flow **/

    /* RET
     *
     * Pop off the last item on 'stk', and return it from the function
     */
    KSB_RET,


    /** Binary Operators **/

    _KSB_BOP_ASSIGN = 50,
    _KSB_BOP_AIOR,
    _KSB_BOP_AXOR,
    _KSB_BOP_AAND,
    _KSB_BOP_ALSH,
    _KSB_BOP_ARSH,
    _KSB_BOP_AADD,
    _KSB_BOP_ASUB,
    _KSB_BOP_AMUL,
    _KSB_BOP_ADIV,
    _KSB_BOP_AFLOORDIV,
    _KSB_BOP_AMOD,
    _KSB_BOP_APOW,
    _KSB_BOP_QUESQUES,
    _KSB_BOP_OROR,
    _KSB_BOP_ANDAND,

    KSB_BOP_IN,

    KSB_BOP_EEQ,
    KSB_BOP_EQ,
    KSB_BOP_NE,
    KSB_BOP_LT,
    KSB_BOP_LE,
    KSB_BOP_GT,
    KSB_BOP_GE,

    KSB_BOP_IOR,

    KSB_BOP_XOR,

    KSB_BOP_AND,

    KSB_BOP_LSH,
    KSB_BOP_RSH,

    KSB_BOP_ADD,
    KSB_BOP_SUB,

    KSB_BOP_MUL,
    KSB_BOP_DIV,
    KSB_BOP_FLOORDIV,
    KSB_BOP_MOD,

    KSB_BOP_POW,

    KSB_UOP_POS = 100, 
    KSB_UOP_NEG,
    KSB_UOP_SQIG,
    _KSB_UOP_POSPOS,
    _KSB_UOP_POSPOS_POST,
    _KSB_UOP_NEGNEG,
    _KSB_UOP_NEGNEG_POST,
    KSB_UOP_NOT,
    _KSB_UOP_QUES,
    _KSB_UOP_STAR

};



/* Attempt to define bytecode as being aligned to a single byte bounary, so that they
 *   are as small as possible
 */
#pragma pack(push, 1)

/* Represents a single operand, no argument */
typedef unsigned char ksb;

/* Represents an operand that takes a single (integer) argument */
typedef struct {

    /* Operation */
    ksb op;

    /* Argument */
    ks_sint32_t arg;

} ksba;

#pragma pack(pop)


/* 'code' - compiled bytecode object which can be executed
 *
 * 
 * 
 */
typedef struct ks_code_s {
    KSO_BASE

    /* File name and source code which it was generated from */
    ks_str fname, src;

    /* List of constants that that bytecode object references */
    ks_list vc;

    /* Mapping of constants to their index into 'vc' */
    ks_dict vc_map;

    /* Actual instructions are stored here */
    ksio_BytesIO bc;

    
    /* Number of meta-entries  */
    ks_ssize_t n_meta;

    /* Array of metadata entries,
     * Kept in sorted order, on the key 'bc_n', which is the length of the bytecode array
     *   at which that metadata is relevant
     */
    struct ks_code_meta {

        /* Length of bytecode when the meta was added */
        int bc_n;

        /* Token representing this operation */
        ks_tok tok;

    }* meta;

}* ks_code;


/* Functions */

/* Throw a syntax error */
#define KS_THROW_SYNTAX(_fname, _src, _tok, ...) do { \
    ks_Exception _se = ks_syntax_error(_fname, _src, _tok, __VA_ARGS__); \
    kso_throw(_se); \
} while (0)


/* Return a token containing both of the other tokens */
KS_API ks_tok ks_tok_combo(ks_tok a, ks_tok b);

/* Convert a token to the string contents */
KS_API ks_str ks_tok_str(ks_str src, ks_tok tok);

/* Generate a syntax error
 */
KS_API ks_Exception ks_syntax_error(ks_str fname, ks_str src, ks_tok tok, const char* fmt, ...);

/* Add token context to an IO stream
 *
 * Adds string in the form:
 * ```
 * <><token><rest>
 *   ^~~~~~~
 * @ <fname>:<pos>
 * ```
 * 
 */
KS_API void ks_tok_add(ksio_AnyIO self, ks_str fname, ks_str src, ks_tok tok);

/* Create a new AST */
KS_API ks_ast ks_ast_new(int kind, int n_args, ks_ast* args, kso val, ks_tok tok);
/* Create a new AST, but absorb the references to 'args[]' and 'val' (i.e. you transfer the references) */
KS_API ks_ast ks_ast_newn(int kind, int n_args, ks_ast* args, kso val, ks_tok tok);


/* Create a new (empty) code object with the given metadata
 */
KS_API ks_code ks_code_new(ks_str fname, ks_str src);

/* Creates a new code object, with all the same reference that 'from' uses, except that it is empty (i.e. no instructions yet)
 */
KS_API ks_code ks_code_from(ks_code from);

/* Add a constant to the bytecode, and return the index into 'self->vc' at which it was added
 * This may return an index other than the last, for example if it was found within 'self->vc_map'
 */
KS_API int ks_code_addconst(ks_code self, kso ob);

/* Emit instructions and append to the 'bc' variable in 'self'
 *
 * emiti emits an opcode that takes an integer argument
 * emito emits an opcode with an object argument, but first adds it to the constants array and uses its index
 *   as the argument
 */
KS_API void ks_code_emit(ks_code self, ksb op);
KS_API void ks_code_emiti(ks_code self, ksb op, int arg);
KS_API void ks_code_emito(ks_code self, ksb op, kso arg);

/* Adds a token as meta to the current position
 */
KS_API void ks_code_meta(ks_code self, ks_tok tok);


/* Pushes an AST onto the 'args' list, and merges the tokens
 */
KS_API void ks_ast_push(ks_ast self, ks_ast sub);
/* Like 'ks_ast_push', but absorbs a reference */
KS_API void ks_ast_pushn(ks_ast self, ks_ast sub);

/* Lex (i.e. tokenize) the given source into an array of tokens, returning
 *   the length of the token array (which should be allocated via 'ks_malloc'/friends
 *   at '*toksp'), or a negative number if an error was thrown
 * 
 * NOTE: Even if a negative size is returned, '*toksp' may be allocated, and should be freed
 *   regardless with 'ks_free()'
 */
KS_API ks_ssize_t ks_lex(ks_str fname, ks_str src, ks_tok** toksp);

/* Parse an AST from source code which has already been tokenized (via 'ks_lex')
 * 
 */
KS_API ks_ast ks_parse_prog(ks_str fname, ks_str src, ks_ssize_t n_toks, ks_tok* toks);
KS_API ks_ast ks_parse_expr(ks_str fname, ks_str src, ks_ssize_t n_toks, ks_tok* toks);


/* Compiles an AST and returns a bytecode object
 *
 * 'prog' is the program being compiled
 * 'from' is either NULL, in which case a new code object is returned, or an existing code object
 *   which the newly compiled object should reference and share (i.e. share the constants array)
 * 
 */
KS_API ks_code ks_compile(ks_str fname, ks_str src, ks_ast prog, ks_code from);



/** Internal methods **/

/* Execute on the current thread's last frame (see 'vm.c' for semantics) */
KS_API kso _ks_exec(ks_code bc);

#endif /* KS_COMPILER_H__ */
