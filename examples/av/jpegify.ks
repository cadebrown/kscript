#!/usr/bin/env ks
""" jpegify.ks - simple utility to repeatedly apply JPEG compression to an image

The purpose of this file is fun, as well as demonstration of in-memory streams,
  so intermediate results don't need to be written to file


Examples:

```
$ ./examples/av/jpegify.ks -h
usage: jpegify [opts] input output

    input                       Input image file
    output                      Output image file

opts:
    -h,--help                   Prints this help/usage message and then exits
    --version                   Prints the version information and then exits
    -n,--num[=int]              Number of iterations of JPEG compression to apply (default: 1)

authors:
    Cade Brown <cade@kscript.org>
version: 0.0.1
$ ./examples/av/jpegify.ks assets/image/monarch.png bad_monarch.jpg -n 50
```

@author: Cade Brown <cade@kscript.org>
"""

import av
import io
import getarg

p = getarg.Parser("jpegify", "0.0.1", "Repeatedly applies JPEG compression to an image", ["Cade Brown <cade@kscript.org>"])

p.pos("input", "Input image file")
p.pos("output", "Output image file")

p.opt("num", ["-n", "--num"], "Number of iterations of JPEG compression to apply", int, 1)

args = p.parse()

# Returns an image that has JPEG compression applied to it
func jpegify(img, n=1) {

    # Create an in-memory buffer stream
    buf = io.BytesIO()
    for i in range(n) {
        # First, clear the buffer
        buf.trunc()
        # Now, write it as if it was a file
        # "jpeg" must be given, since there is no output extension and the format can't be inferred
        av.imwrite(buf, img, "jpeg")

        # Read it back, simulating JPEG compression
        img = av.imread(buf)
    }
    # Return the image
    ret img
}

# Actually perform conversion
av.imwrite(args.output, jpegify(av.imread(args.input), args.num))
