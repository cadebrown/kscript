#!/usr/bin/env ks
""" ffi/basic.ks - Basic example showing how to use the 'ffi' module to load from DLLs


@author: Cade Brown <cade@kscript.org>
"""

import ffi

# C standard library
dll_c = ffi.open("libc.so.6")

# Load the 'puts' function, with the given signature
puts = dll_c.puts as ffi.func[ffi.sint, (ffi.ptr[ffi.schar],)]

# Now, call just like a normal function
puts("hello, world")


# Load 'printf', which is variadic (thus the '...')
c_printf = dll_c["printf"] as ffi.func[ffi.sint, (ffi.ptr[ffi.schar], ...)]
c_printf("hello, world: %i:%i, and also %5.2f\n", 3, 5, 3.2)

# Malloc/free for memory 
malloc = dll_c.malloc as ffi.func[ffi.ptr, (ffi.size_t,)]
free = dll_c.free as ffi.func[none, (ffi.ptr,)]

N = 2 ** 4
array = malloc(ffi.sizeof(ffi.sint) * N) as ffi.ptr[ffi.sint]
assert array

for i in range(N), array[i] = i ** 2
print(array[5])

free(array)
