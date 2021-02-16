#!/usr/bin/env ks
""" prog.ks - progress bar example


@author: Cade Brown <cade@kscript.org>
"""

import time
import os

type ProgressBar {

    func __init(self, objs, w=50, out=os.stdout) {
        self.objs = objs as list
        self.i = 0
        self.w = w
        self.out = out
    }

    func __next(self) {
        if self.i >= len(self.objs) {
            throw OutOfIterException()
        }

        # Get current position
        i = self.i
        self.i += 1

        t = (i + 1) / len(self.objs)

        num = int(t * self.w) 

        self.out.printf("-" * num + " " * int(self.w - num))
        self.out.printf("%-5.1f%%\r", 100 * t)
        self.out.flush() ?? none

        ret self.objs[i]
    }

}


for i in ProgressBar(range(123)) {
    time.sleep(0.02)
}

