#!/usr/bin/env ks
""" threadprint.ks - a simple example utilizing multiple threads at once

@author: Cade Brown <cade@kscript.org>
"""


import os
import getarg

p = getarg.Parser("threadprint", "0.1.0", "Print out ranges using threads", ["Cade Brown <cade@kscript.org>"])

p.arg("ppt", ["-ppt", "--per-thread"], "Number of elements to print per thread", int, none, 10)
p.arg("N", ["-N", "--Nthreads"], "Number of threads to run", int, none, 2)

# parse the args, throwing an error if something incorrect was given
# or, if `-h` or `--help` are given, print out a usage message and exit successfully
args = p.parse()

# function called for each thread
func threadfunc(i, toprint) {
    for j in toprint {
        print (i + ": " + j)
    }
    print (i + ": DONE")
}

# create logger
LOG = logger('threadprint')
LOG.level = logger.INFO

# now, create threads
threads = []

LOG.info("Starting all threads...")

for i in range(args.N) {
    threads.push(t = os.thread(threadfunc, (i, range(args.ppt * i, args.ppt * (i + 1)))))
    t.start()
}

LOG.info("Joining all threads...")

for t in threads {
    t.join()
}

LOG.info("Done!")

