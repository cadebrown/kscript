# kscript (ks)

kscript ([https://kscript.org](https://kscript.org)) is a dynamic programming language with expressive syntax, cross platform support, and a rich standard library. Its primary aim is to allow developers to write platform agnostic programs that can run anywhere, and require little or no platform- or os- specific code.

Documentation is available at [https://kscript.org](https://kscript.org), which provides examples, tutorials, and coverage of the standard library. Formal specifications are available at [https://docs.kscript.org](https://docs.kscript.org). kscript is described under the Kscript Public License (KPL).


## Current Status

kscript is currently a work-in-progress, aiming at a 1.0.0 release by the end of 2021. The most important parts have been prototyped, but a unified design is needed to target extensions and modules. With 1.0.0, we also are designing a distribution system, and standard guides for developers

Some current areas being worked on:

  * Transparent AST format (for manipulation)
  * Annotations, wrappers
  * Unified access rules
  * Standard library modules
  * Standardized C API, C types, C extensions
  * Should we use C or C++?
  * Standard module format (folder/directory), and import path resolution


## Building

See [BUILD.md](/BUILD.md)


## Installing

Right now, it is in an early alpha stage, so installers aren't ready. Check out the section below, on building it


## Running

See [RUN.md](/RUN.md)

