#!/usr/bin/env ks
""" fib.ks - calculating Fibonacci numbers


NOTE: https://en.wikipedia.org/wiki/Fibonacci_number

@author: Cade Brown <cade@kscript.org>
"""

import getarg

p = getarg.Parser("fib", "0.1.0", "Generate fibonacci numbers", ["Cade Brown <brown.cade@gmail.com>"])

p.flag("largest", ["--largest"], "If this flag is set, only print the largest number")
p.pos("N", "The number of fibonacci numbers to calculate", int, 1, none, 20)

args = p.parse()

if args.largest {
    # there is a faster method for calculating just the larger
    # If we recognize:
    #
    # F(2*N) = F(N) * (2 * F(N+1) - F(N))
    # F(2*N+1) = F(N)**2 + F(N+1)**2
    #
    func F(N) {
        if N < 3, ret [0, 1, 1][N]

        # floor down
        Nd = N // 2
        if N % 2 == 0 {
            # even
            F_Nd = F(Nd)
            ret F_Nd * (2 * F(Nd + 1) - F_Nd)
        } else {
            # odd
            ret F(Nd) ** 2 + F(Nd + 1) ** 2
        }
    }

    # calculate largest
    print (F(args.N))

} else {
    # first two numbers
    (a, b) = (0, 1)

    for i in range(args.N) {
        # calculate via recurrence
        (a, b) = (a + b, a)

        print (a)
    }
}
