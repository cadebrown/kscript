#!/usr/bin/env ks
""" fib.ks - calculating Fibonacci numbers


NOTE: https://en.wikipedia.org/wiki/Fibonacci_number

@author: Cade Brown <cade@kscript.org>
"""
import getarg

# Create an argument parser
p = getarg.Parser('fib', '0.0.1', 'Calculates Fibonacci numbers', ['Cade Brown <cade@kscript.org>'])

# Add a positional argument, which is of type 'int'
p.pos('n', 'Which Fibonacci number to calculate', 1, int)

# Default arguments are 'os.argv', so we don't have to mention that
args = p.parse()

# Calculates the 'n'th Fibonacci number
func fib(n) {
    if n == 0 || n == 1 {
        ret n
    } else {
        # Calling '...' causes recursion 
        ret ...(n - 1) + ...(n - 2)
    }
}

# Now, we can use 'args.n' to directly reference it (it is already an 'int')
print (fib(args.n))
