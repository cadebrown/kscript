#!/usr/bin/env ks
""" ffi/basic.ks - Basic example showing the Foreign Function Interface (FFI) module


@author: Cade Brown <cade@kscript.org>
"""

import ffi

# C library
lib = ffi.open('libc.so.6')

# Load the 'puts' function, with the given signature
cputs = lib.load('puts', ffi.func[ffi.int, (ffi.ptr[ffi.char], )])

# Now, you can call it just like a normal function
cputs("hello, world")


# Load 'printf', which is variadic (thus the '...')
cprintf = lib.load('printf', ffi.func[ffi.int, (ffi.ptr[ffi.char], ...)])
cprintf("hello, world: %i:%i, and also %5.2f\n", 3, 5, 3.2)


# Now, some memory allocation functions
# You can also use `ffi.malloc` and `ffi.free` (which are included in the module)
cmalloc = lib.load('malloc', ffi.func[ffi.ptr, (ffi.size_t,)])
cfree = lib.load('free', ffi.func[none, (ffi.ptr,)])

N = 2 ** 4

# Allocate an array (and type cast it with 'as')
array = cmalloc(ffi.sizeof(ffi.int) * N) as ffi.ptr[ffi.int]
# Check for failure
assert array

# Populate array
for i in range(N) {
    array[i] = i ** 2
}

print (array[5])

print (array)
print (hex(array))

# Free it
cfree(array)

"""

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

"""