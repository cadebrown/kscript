/* site-hljs-kscript.js - kscript language support
 *
 * Adds kscript language support to highlight.js, as the 'ks' language
 * 
 * SEE: https://highlightjs.readthedocs.io/en/latest/language-guide.html
 * 
 * @author: Cade Brown <cade@kscript.org>
 */

hljs.registerLanguage("ks", function(e) {

/* Language keywords */
const KEYWORDS = [
    'with',
    'del',
    'import',
    'from',
    'as', 
    'in',

    'assert',
    'break',
    'cont',

    'type',
    'func',
    'extends',
    
    'if',
    'elif',
    'else',

    'while',

    'for',

    'try',
    'catch',
    'finally',

    'ret',
    'throw',
];

/* Literal values (similar to keywords) */
const LITERALS = [
    '...',
    'false', 'true',
    'none',
    'undefined',
    'inf', 'nan',
]


/* Builtin names */
const BUILTINS = [
    '__argv',
    '__stdin',
    '__stdout',
    '__stderr',

    'object',
    'type',
    'number',
    'int',
    'enum',
    'bool',
    'float',
    'complex',
    'str',
    'bytes',
    'range',
    'slice',
    'tuple',
    'list',
    'set',
    'dict',

    'abs',
    'pow',
    'min',
    'max',
    'sum',

    'bin',
    'oct',
    'hex',

    'ord',
    'chr',

    'compile',
    'eval',
    'exec',

    'enumerate',
    'filter',
    'map',
    'zip',
    'all',
    'any',

    'iter',
    'next',
    'len',
    'hash',
    'id',
    'repr',
    'print',
    'printf',

    'isinst',
    'issub',

    'input',
    'exit',
]

/* Interpreter prompts */
const PROMPT = [
{
    className: 'meta', begin: /^(>>>|\.\.\.) /
},
{
    className: 'meta', begin: /^<(\w+)/
},
{
    className: 'meta', begin: />$/
}
]

/* Numeric helpers */
const _EXP_E = '([eE][+-]?[0-9]+)';
const _EXP_P = '([pP][+-]?[0-9]+)';
const _I = '([iI])';

/* Any number sequence */
const NUMBER = {
    className: 'number', relevance: 0,
    variants: [
        { begin: `\\b0[bB]([0-1]+\\.[0-1]*)${_EXP_P}?${_I}?\\b` },
        { begin: `\\b0[bB]([0-1]*\\.[0-1]+)${_EXP_P}?${_I}?\\b` },
        { begin: `\\b0[bB]([0-1]+)${_EXP_P}?${_I}?\\b` },
        { begin: `\\b0[oO]([0-7]+\\.[0-7]*)${_EXP_P}?${_I}?\\b` },
        { begin: `\\b0[oO]([0-7]*\\.[0-7]+)${_EXP_P}?${_I}?\\b` },
        { begin: `\\b0[oO]([0-7]+)${_EXP_P}?${_I}?\\b` },
        { begin: `\\b0[xX]([0-9a-fA-F]+\\.[0-9a-fA-F]*)${_EXP_P}?${_I}?\\b` },
        { begin: `\\b0[xX]([0-9a-fA-F]*\\.[0-9a-fA-F]+)${_EXP_P}?${_I}?\\b` },
        { begin: `\\b0[xX]([0-9a-fA-F]+)${_EXP_P}?${_I}?\\b` },


        { begin: `\\b(0[dD])?([0-9]+\\.[0-9]*)${_EXP_E}?${_I}?\\b` },
        { begin: `\\b(0[dD])?([0-9]*\\.[0-9]+)${_EXP_E}?${_I}?\\b` },
        { begin: `\\b(0[dD])?([0-9]+)${_EXP_E}?${_I}?\\b` },
    ]
};

/* String literals */
const STRING = {
    className: 'string',
    contains: [hljs.BACKSLASH_ESCAPE],
    variants: [
      {
        begin: /'''/, end: /'''/,
        contains: [hljs.BACKSLASH_ESCAPE, PROMPT],
        relevance: 10
      },
      {
        begin: /"""/, end: /"""/,
        contains: [hljs.BACKSLASH_ESCAPE, PROMPT],
        relevance: 10
      },
      {
        begin: /'/, end: /'/
      },
      {
        begin: /"/, end: /"/
      },

      hljs.APOS_STRING_MODE,
      hljs.QUOTE_STRING_MODE
    ]
};

    /* Return language description */
    return {
        name: 'kscript',
        aliases: [ "ks", "kscript" ],
        keywords: {
            keyword: KEYWORDS.join(' '),
            literal: LITERALS.join(' '),
            built_in: BUILTINS.join(' '),
        },
        contains: [
            PROMPT,
            NUMBER,
            STRING,
            hljs.HASH_COMMENT_MODE,
            {
                variants: [
                    { className: 'function', beginKeywords: 'func' },
                    { className: 'class',    beginKeywords: 'type' }
                ],
                
                end: /{/,
                illegal: /[${;\n,]/,
                contains: [
                    hljs.UNDERSCORE_TITLE_MODE,
                ]
            },
        ],
    };
});
        
