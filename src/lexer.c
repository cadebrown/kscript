/* lexer.c - Lexer/scanner/tokenizer implementation for kscript
 *
 * This file is effectively a regex lexer, but hand coded for maximum speed
 * 
 * @author: Cade Brown <cade@kcript.org>
 */
#include <ks/impl.h>
#include <ks/compiler.h>


/** Internals/Utilities **/

/* Test if a character is a valid digit in base 'b' */
static bool is_digit(ks_ucp c, int b) {
    /**/ if (b == 2) return '0' <= c && c <= '1';
    else if (b == 8) return '0' <= c && c <= '7';
    else if (b == 10) return '0' <= c && c <= '9';
    else if (b == 16) return ('0' <= c && c <= '9') || ('a' <= c && c <= 'f') || ('A' <= c && c <= 'F');
    else {
        assert(false);
    }
}

/* Test if a character is a valid start of an identifier */
static bool is_name_s(ks_ucp c) {
    if (c < 128) {
        if (('a' <= c && c <= 'z') || ('A' <= c && c <= 'Z') || c == '_') return true;
    } else {
        /* TODO: unicode */
        /* 
        struct ksucd_info info;
        ksucd_cp cp = ksucd_get_info(&info, c);
        if (cp != KSUCD_ERR) {
            if (info.cat_gen >= 0 && info.cat_gen <= ksucd_cat_L) return true;
        }*/

    }
    return false;
}

/* Test if a character is a valid middle of an identifier */
static bool is_name_m(ks_ucp c) {
    if (is_name_s(c)) return true;
    if (is_digit(c, 10)) return true;

    return false;
}


/** Token utilities **/


ks_tok ks_tok_combo(ks_tok a, ks_tok b) {
    if (a.spos < 0) return a;
    else if (b.spos < 0) return b;
    if (a.spos > b.spos) return ks_tok_combo(b, a);

    /* Now, merge them together */
    ks_tok r;
    r.kind = KS_TOK_MANY;

    r.sline = a.sline;
    r.scol = a.scol;
    r.spos = a.spos;

    r.eline = b.eline;
    r.epos = b.epos;
    r.ecol = b.ecol;

    return r;
}


ks_str ks_tok_str(ks_str src, ks_tok tok) {
    return ks_str_new(tok.epos - tok.spos, src->data + tok.spos);
}

void ks_tok_add(ksio_BaseIO self, ks_str fname, ks_str src, ks_tok tok, bool inc_at) {

    /* Subidivide line */
    if (tok.sline < 0 || tok.scol < 0 || tok.spos < 0) {
        /* Invalid token/not real file */
        if (inc_at) ksio_add(self, "@ <EOF> in %R", fname);

    } else {
        /* Real token, so add context */
        const char* s = src->data;
        int pl = tok.spos;

        /* Whether the token spans multiple lines */
        bool is_multi = tok.sline != tok.eline;

        /* Find start of line */
        while (pl >= 0) {
            if (s[pl] == '\n') {
                pl++;
                break;
            } else {
                pl--;
            }
        }
        if (pl < 0) pl = 0;

        /* Calculate end of line */
        int el = pl + 1;
        while (s[el] && s[el] != '\n') el++;

        /* Add context */
        ksio_add(self, KS_COL_RESET
                  "%.*s" KS_COL_RED KS_COL_BOLD "%.*s" KS_COL_RESET "%.*s\n"
                  "%.*c" KS_COL_RED KS_COL_BOLD "^%.*c%s" KS_COL_RESET,
            tok.spos - pl, s + pl,
            is_multi ? el - tok.spos : tok.epos - tok.spos, s + tok.spos,
            is_multi ? 0 : el - tok.epos, s + tok.epos,

            tok.scol, ' ',
            (is_multi ? el - pl - tok.scol - 14 : tok.ecol - tok.scol) - 1, '~',
            is_multi ? "(continued on next line)" : ""
        );
        
        if (inc_at) ksio_add(self, "\n@ Line %i, Col %i in %R", tok.sline + 1, tok.scol + 1, fname);
    }

}


ks_ssize_t ks_lex(ks_str fname, ks_str src, ks_tok** toksp) {
    /* Position */
    int line = 0, col = 0, pos = 0, lline = 0, lcol = 0, lpos = 0;

    int sz = src->len_b;
    ks_ucp c = 0;

    /* Output (along with '*toks') */
    ks_ssize_t n_toks = 0, max_n_toks = 0;

    ks_tok bad;

    #define len (pos - lpos)
    #define toks (*toksp)

    /* Create a token at the current position */
    #define MAKE(_kind) ((ks_tok){ _kind, lline, lcol, lpos, line, col, pos })

    /* Add a token to the output */
    #define EMIT(_tok) do { \
        int _i = n_toks++; \
        if (n_toks > max_n_toks) { \
            max_n_toks = ks_nextsize(max_n_toks, n_toks); \
            toks = ks_zrealloc(toks, sizeof(*toks), max_n_toks); \
        } \
        toks[_i] = _tok; \
    } while (0)

    /* Advance one character */
    #define ADV() do { \
        if (pos >= sz) break; \
        pos++; \
        if (c == '\n') { \
            line++; \
            col = 0; \
        } else { \
            col++; \
        } \
        _UPDATEC(); \
    } while (0)

    #define _UPDATEC() do { \
        c = src->data[pos]; \
    } while (0)

    /* Get whether the next part of the input is a given C string */
    #define NEXTIS(_cstr) (strncmp(src->data + pos, _cstr, sizeof(_cstr) - 1) == 0)


    _UPDATEC();
    while (pos < sz) {
        /* Strip whitespace */
        while (pos < sz && (c == ' ' || c == '\t' || c == '\v' || c == '\f' || c == '\r')) {
            ADV();
        }

        if (pos >= sz) break;

        lline = line;
        lcol = col;        
        lpos = pos;

        if (c == '\n') {
            /* Newline */
            ADV();
            EMIT(MAKE(KS_TOK_N));
        } else if (NEXTIS("\\\n")) {
            /* Line continuation, so skip */
            ADV();
            ADV();
        } else if (c == '#') {
            /* Comment, so skip */
            ADV();

            while (pos < sz) {
                if (NEXTIS("\\\n")) {
                    /* Continue the comment through the next line */
                    ADV();
                    ADV();
                } else if (c == '\n') {
                    ADV();
                    break;
                } else {
                    ADV();
                }
            }
        } else if (is_digit(c, 10) || (c == '.' && is_digit(src->data[pos+1], 10))) {
            /* Numeric constant of some kind */
            int base = 0;
            if (c == '0') {
                ADV();
                /**/ if (c == 'd' || c == 'D') base = 10;
                else if (c == 'b' || c == 'B') base = 2;
                else if (c == 'o' || c == 'O') base = 8;
                else if (c == 'x' || c == 'X') base = 16;
                if (base) ADV();
            }
            if (!base) base = 10;

            while (pos < sz && is_digit(c, base)) {
                ADV();
            }
            bool is_flt = c == '.';

            if (is_flt) {
                ADV();
                while (pos < sz && is_digit(c, base)) {
                    ADV();
                }
            }
            if (c == 'e' || c == 'E') {
                is_flt = true;
                ADV();
                if (c == '+' || '-') ADV();
                while (pos < sz && is_digit(c, 10)) {
                    ADV();
                }
            }
            if (c == 'i' || c == 'I') {
                is_flt = true;
                ADV();
            }

            EMIT(MAKE((is_flt?KS_TOK_FLOAT:KS_TOK_INT)));
        } else if (is_name_s(c)) {
            /* Name */
            do {
                ADV();
            } while (pos < sz && is_name_m(c));

            #define CASE_KW(_tokk, _str) else if (len == (sizeof(_str) - 1) && strncmp(src->data + lpos, _str, len) == 0) { \
                EMIT(MAKE(_tokk)); \
            }
            if (false) {}
            CASE_KW(KS_TOK_IN, "in")
            CASE_KW(KS_TOK_AS, "as")
            
            CASE_KW(KS_TOK_IMPORT, "import")
            CASE_KW(KS_TOK_ASSERT, "assert")
            CASE_KW(KS_TOK_THROW, "throw")
            CASE_KW(KS_TOK_RET, "ret")
            CASE_KW(KS_TOK_BREAK, "break")
            CASE_KW(KS_TOK_CONT, "cont")
            CASE_KW(KS_TOK_IF, "if")
            CASE_KW(KS_TOK_ELSE, "else")
            CASE_KW(KS_TOK_ELIF, "elif")
            CASE_KW(KS_TOK_WHILE, "while")
            CASE_KW(KS_TOK_FOR, "for")
            CASE_KW(KS_TOK_TRY, "try")
            CASE_KW(KS_TOK_CATCH, "catch")
            CASE_KW(KS_TOK_FINALLY, "finally")

            else {
                EMIT(MAKE(KS_TOK_NAME));
            }
        } else if (c == '"' || c == '\'') {
            /* String constant */

            /* detect triple quoted strings */
            char c3[4] = { c, c, c, '\0' };
            bool is_c3 = strncmp(src->data + pos, c3, 3) == 0;
            if (is_c3) {
                ADV();
                ADV();
                ADV();
            } else {
                ADV();
            }

            while (pos < sz && strncmp(src->data + pos, c3, is_c3 ? 3 : 1) != 0) {
                if (c == '\\') ADV();
                ADV();
            }

            if (strncmp(src->data + pos, c3, is_c3 ? 3 : 1) == 0) {
                if (is_c3) {
                    ADV();
                    ADV();
                    ADV();
                } else {
                    ADV();
                }
                EMIT(MAKE(KS_TOK_STR));
            } else {
                bad = MAKE(KS_TOK_MANY);
                KS_THROW_SYNTAX(fname, src, bad, "No end to string");
                return -1;
            }
        } else if (c == '`') {
            /* Regex constant */

            /* detect triple quoted strings */
            char c3[4] = { c, c, c, '\0' };
            bool is_c3 = strncmp(src->data + pos, c3, 3) == 0;
            if (is_c3) {
                ADV();
                ADV();
                ADV();
            } else {
                ADV();
            }

            while (pos < sz && strncmp(src->data + pos, c3, is_c3 ? 3 : 1) != 0) {
                if (c == '\\') ADV();
                ADV();
            }

            if (strncmp(src->data + pos, c3, is_c3 ? 3 : 1) == 0) {
                if (is_c3) {
                    ADV();
                    ADV();
                    ADV();
                } else {
                    ADV();
                }
                EMIT(MAKE(KS_TOK_STR));
            } else {
                bad = MAKE(KS_TOK_MANY);
                KS_THROW_SYNTAX(fname, src, bad, "No end to string");
                return -1;
            }
        }

        #define CASEU_OP(_kind, _opstr, _numc) else if (NEXTIS(_opstr)) { \
            int left = _numc; \
            while (left-- > 0) { ADV(); } \
            EMIT(MAKE(_kind)); \
        }

        /* Math Operators (Unicode) */
        CASEU_OP(KS_TOK_IN, "\xE2\x88\x88", 1)
        CASEU_OP(KS_TOK_NAME, "\xE2\x88\x9E", 1)
        CASEU_OP(KS_TOK_FOR, "\xE2\x88\x80", 1)

        CASEU_OP(KS_TOK_ANDAND, "\xE2\x88\xA7", 1)
        CASEU_OP(KS_TOK_OROR, "\xE2\x88\xA8", 1)

        /* Arrows/Misc. Aliases */
        CASEU_OP(KS_TOK_RARW, "\xE2\x86\x92", 1)

        /* IMPORTANT: symbols that contain other symbols must be before them in these
         *   lines
         */
        CASEU_OP(KS_TOK_DOTDOTDOT, "...", 3)
        CASEU_OP(KS_TOK_DOT, ".", 1)
        CASEU_OP(KS_TOK_COM, ",", 1)
        CASEU_OP(KS_TOK_COL, ":", 1)
        CASEU_OP(KS_TOK_SEMI, ";", 1)
        CASEU_OP(KS_TOK_LPAR, "(", 1)
        CASEU_OP(KS_TOK_RPAR, ")", 1)
        CASEU_OP(KS_TOK_LBRC, "{", 1)
        CASEU_OP(KS_TOK_RBRC, "}", 1)
        CASEU_OP(KS_TOK_LBRK, "[", 1)
        CASEU_OP(KS_TOK_RBRK, "]", 1)
        CASEU_OP(KS_TOK_LARW, "<-", 2)
        CASEU_OP(KS_TOK_RARW, "->", 2)

        CASEU_OP(KS_TOK_AADD, "+=", 2)
        CASEU_OP(KS_TOK_ASUB, "-=", 2)
        CASEU_OP(KS_TOK_AMUL, "*=", 2)
        CASEU_OP(KS_TOK_ADIV, "/=", 2)
        CASEU_OP(KS_TOK_AFLOORDIV, "//=", 3)
        CASEU_OP(KS_TOK_AMOD, "%=", 2)
        CASEU_OP(KS_TOK_APOW, "**=", 3)
        CASEU_OP(KS_TOK_ALSH, "<<=", 3)
        CASEU_OP(KS_TOK_ARSH, ">>=", 3)
        CASEU_OP(KS_TOK_AIOR, "|=", 2)
        CASEU_OP(KS_TOK_AXOR, "^=", 2)
        CASEU_OP(KS_TOK_AAND, "&=", 2)

        CASEU_OP(KS_TOK_ADDADD, "++", 2)
        CASEU_OP(KS_TOK_ADD, "+", 1)
        CASEU_OP(KS_TOK_SUBSUB, "--", 2)
        CASEU_OP(KS_TOK_SUB, "-", 1)
        CASEU_OP(KS_TOK_POW, "**", 2)
        CASEU_OP(KS_TOK_MUL, "*", 1)
        CASEU_OP(KS_TOK_FLOORDIV, "//", 2)
        CASEU_OP(KS_TOK_DIV, "/", 1)
        CASEU_OP(KS_TOK_MOD, "%", 1)
        CASEU_OP(KS_TOK_LSH, "<<", 2)
        CASEU_OP(KS_TOK_RSH, ">>", 2)
        CASEU_OP(KS_TOK_OROR, "||", 2)
        CASEU_OP(KS_TOK_IOR, "|", 1)
        CASEU_OP(KS_TOK_XOR, "^", 1)
        CASEU_OP(KS_TOK_ANDAND, "&&", 2)
        CASEU_OP(KS_TOK_AND, "&", 1)

        CASEU_OP(KS_TOK_LE, "<=", 2)
        CASEU_OP(KS_TOK_LT, "<", 1)
        CASEU_OP(KS_TOK_GE, ">=", 2)
        CASEU_OP(KS_TOK_GT, ">", 1)
        CASEU_OP(KS_TOK_EQ, "==", 2)
        CASEU_OP(KS_TOK_NE, "!=", 2)

        CASEU_OP(KS_TOK_ASSIGN, "=", 1)

        CASEU_OP(KS_TOK_NOT, "!", 1)
        CASEU_OP(KS_TOK_SQIG, "~", 1)
        CASEU_OP(KS_TOK_QUESQUES, "??", 2)
        CASEU_OP(KS_TOK_QUES, "?", 1)

        else {
            /* unrecognized unicode character, report as an error */
            ADV();
            bad = MAKE(KS_TOK_MANY);
            KS_THROW_SYNTAX(fname, src, bad, "Unexpected character");
            return -1;
        }
    }

    lline = line;
    lcol = col;        
    lpos = pos;
    EMIT(MAKE(KS_TOK_EOF));

    /* Debug tokens */

    /*
    int i;
    for (i = 0; i < n_toks; ++i) {
        printf("toks[%i]: '%.*s'\n", i, toks[i].epos - toks[i].spos, src->data + toks[i].spos);
    }
    */

    return n_toks;

    #undef MAKE
    #undef EMIT
    #undef ADV
    #undef len
    #undef toks
}


