#!/usr/bin/env python3
""" tools/impl_gamma.py - Python script to generate a C implementation of the Gamma function

To install requirements, run:

$ pip3 install gmpy2



It should be set up so that you can run `make src/modules/m/impl_gamma.c`, and it will re-run as needed

But, to run manually, simply call:

$ ./tools/impl_gamma.py --type double --prec 64 --utilprefix ksm_ --funcname ksm_gamma --include "ks/impl.h" > src/modules/m/impl_gamma.c


@author: Cade Brown <cade@kscript.org>
"""


import argparse

# requirement: gmpy2
from gmpy2 import mpfr, exp, sqrt, log, log10, const_pi, get_context
from math import factorial

# add commandline arguments
parser = argparse.ArgumentParser(formatter_class=lambda prog: argparse.HelpFormatter(prog, max_help_position=40))

parser.add_argument('--prec', help='Internal precision required, in number of bits', default=64, type=int)
parser.add_argument('--type', help='Datatype that the function is computed for', default='double')
parser.add_argument('--itype', help='Internal datatype used for computation', default=None)
parser.add_argument('--funcname', help='Function name to export', default='ksm_gamma')
parser.add_argument('--include', help='List of files to include', default=[], nargs='*')
parser.add_argument('--utilprefix', help='Prefix used for math functions (sqrt, log, exp, etc). Leave blank for math.h functions', default='')

args = parser.parse_args()

# Default internal type to interface type
if not args.itype:
    args.itype = args.type


# TODO: Use smarter hueristic?
get_context().precision = int(2 * args.prec)

# Compute the pi constant
pi = const_pi()

# Create unit-in-last-place (ulp) for comparisons
ulp = 2 ** -args.prec

# Helper function that computes the error for a given N-sized approximatino
def err(N):
    return N ** -0.5 * (2 * pi) ** (-N - 0.5)


# Generate a table for size-N approximation
def table(N):
    c = [None] * N

    c[0] = sqrt(2 * pi)

    # Alternating factorial computation
    sfk = 1
    for k in range(1, N):
        c[k] = exp(N - k) * pow(N - k, k - 0.5) / sfk
        sfk *= -k

    return c


# First, do ever increasing binary search
N = 1
while err(N) > ulp:
    N *= 2

# Now, isolate
step = N // 2
while step > 0:
    # Do binary search
    if err(N) > ulp:
        N += step
    else:
        N -= step
    
    # Binary search, so divide step up
    step //= 2


# Now, print out C code
for inc in args.include:
    print(f"#include <{inc}>")


print(f"""
{args.type} ksm_gamma({args.type} x) {{
    /* TODO: Reflection formulae? */

    /* Convert to internal type */
    {args.itype} ix = x;

    /* Generated table */
    static int N = {N};
    static {args.itype} c[{N}] = {{
""")

# Print out table values required
for c in table(N):
    print(f"        {str(c)}, ")

print(f"""
    }};
""")

# Finally, sum up over the table and apply some processing to the sum
print(f"""
    /* Now, compute sum */
    {args.itype} acc = c[0];
    int k;
    for (k = 1; k < N; ++k) {{
        acc += c[k] / (ix + k);
    }}

    acc *= {args.utilprefix}exp(N - ix) * {args.utilprefix}pow(N + ix, ix + 0.5);

    return ({args.type})(acc / ix);
}}
""")

