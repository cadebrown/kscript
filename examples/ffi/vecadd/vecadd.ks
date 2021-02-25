#!/usr/bin/env ks
""" vecadd.ks - Adds vectors using a C library

This references the code writtein in 'vecadd.c'. So, you'll have to compile that before running this

Here's how to run this example:

```
$ make
cc -fPIC -c vecadd.c -o vecadd.o
cc -shared vecadd.o -o libvecadd.so
$ ks vecadd.ks
success!
```

@author: Cade Brown <cade@kscript.org>
"""

# FFI module, for loading shared libraries dynamically
import ffi

import m


# Load the library we compiled
# If this fails, you need to run `make` inside the `vecadd` directory
libvecadd = ffi.open('libvecadd.so')

# vec3f - Vector type (defined in 'vecadd.c')
type vec3f extends ffi.struct[
        (ffi.float, 'x'),
        (ffi.float, 'y'),
        (ffi.float, 'z'),
    ] {

}

# Load the vecadd function
cvecadd = libvecadd.load('vecadd', ffi.func[none, (ffi.int, ffi.ptr[vec3f], ffi.ptr[vec3f], ffi.ptr[vec3f])])



# Number of elements
N = 50

# Now, create some arrays
A = ffi.malloc(ffi.sizeof(vec3f) * N) as ffi.ptr[vec3f]
B = ffi.malloc(ffi.sizeof(vec3f) * N) as ffi.ptr[vec3f]
C = ffi.malloc(ffi.sizeof(vec3f) * N) as ffi.ptr[vec3f]

# Populate
for i in range(N) {
    B[i] = vec3f(3 * i + 0, 3 * i + 1, 3 * i + 2)
    C[i] = vec3f(3 * i + 0, 3 * i + 1, 3 * i + 2)
}

# Compute vecadd
cvecadd(N, A, B, C)

# Double check results
for i in range(N) {
    assert m.isclose(A[i].x, B[i].x + C[i].x) && m.isclose(A[i].y, B[i].y + C[i].y) && m.isclose(A[i].z, B[i].z + C[i].z)
}

# Remember to free arrays
ffi.free(A)
ffi.free(B)
ffi.free(C)


print ("success!")
