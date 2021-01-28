#!/usr/bin/env ks
""" greet.ks - a simple program which prints a personalized greeting

This is a slightly-more advanced example which is reminiscent of 'hello_world.ks'

It uses argument parsing via the 'getarg' module, the 'os' module to get a default user name if none is provided

Examples:

```
$ ./examples/greet.ks
hello, cade
$ ./examples/greet.ks YourName
hello, YourName
$ ./examples/greet.ks YourName -gHey
Hey YourName
$ ./examples/greet.ks -h
usage: greet [opts] names...

    names                       List of names to greet

opts:
    -h,--help                   Prints this help/usage message and then exits
    --version                   Prints the version information and then exits
    -g,--greeting[=str]         The greeting message to use (default: hello,)

authors:
    Cade Brown <cade@kscript.org>
version: 0.1.0

```

@author: Cade Brown <cade@kscript.org>
"""

import os
import getarg

p = getarg.Parser("greet", "0.1.0", "Prints a personalized greeting", ["Cade Brown <cade@kscript.org>"])

# opt(name, opts, doc, trans=str, defa=none)
p.opt("greeting", ["-g", "--greeting"], "The greeting message to use", str, "hello,")
# pos(name, doc, num=1, trans=str, defa=none)
p.pos("names", "List of names to greet", -1)

# parse the args, throwing an error if something incorrect was given
# or, if `-h` or `--help` are given, print out a usage message and exit successfully
args = p.parse()

if !args.names, args.names = [os.getenv("USER", "<unknown>")]

# print out customized message
for name in args.names {
    print (args.greeting, name)
}

