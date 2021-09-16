#!/usr/bin/env ks
""" examples/fib.ks - calculating Fibonacci numbers


NOTE: https://en.wikipedia.org/wiki/Fibonacci_number

@author: Cade Brown <cade@kscript.org>
"""

# Calculates the 'n'th Fibonacci number
func fib(n) {
    if n in (0, 1) {
        ret n
    } else {
        # Calling '...' causes recursion
        ret ...(n - 1) + ...(n - 2)
    }
}

# Print out some examples
for i in range(13) {
    print(fib(i))
}
