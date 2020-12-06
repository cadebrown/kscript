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
usage: greet [opts] name

opts:
    -h,--help                       Print this help/usage msesage and exit
    -V,--version                    Print out the version information and exit
    -g,--greeting[='hello,']        The greeting message to use (str)

    name[='cade']                   What is your user name (str)

Prints a personalized greeting

author: Cade Brown <cade@kscript.org>
version: 0.1.0
```

@author: Cade Brown <cade@kscript.org>
"""

import os
import getarg

p = getarg.Parser("greet", "0.1.0", "Prints a personalized greeting", ["Cade Brown <cade@kscript.org>"])

# arg(name, opts, desc, trans=str, action=none, defa=none)
p.arg("greeting", ["-g", "--greeting"], "The greeting message to use", str, none, "hello,")
# pos(name, desc, num=1, trans=str, action=none, defa=none)
p.pos("name", "What is your user name", str, 1, none, os.getenv("USER", "<unknown>"))

# parse the args, throwing an error if something incorrect was given
# or, if `-h` or `--help` are given, print out a usage message and exit successfully
args = p.parse()

# print out customized message
print (args.greeting, args.name)

