#!/usr/bin/env ks
""" build.ks - build system

@author: Cade Brown <cade@kscript.org>
"""

import os
import kpm.cext

# Create a project to build 'ksm_system.so'
proj = kpm.cext.Project("ksm_system.so")

# Add source files
proj.SRC_C += os.glob('src/*.c')

# Build the project
proj.build()
