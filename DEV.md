# Developer Guide

This document describes how to develop kscript, contribute to the repository, and so on. 

## Setup

To set up, make sure you've cloned the repository (`git clone https://github.com/ChemicalDevelopment/kscript.git`), and then can run `./configure` and `make` to build it.

From there, you are set! Just edit some files, and then re-run `make` to update the program. For example:

```bash
$ make -j16 && ./bin/ks -e 'print (1, 2, 3)'
1 2 3 
```

(the `-j16` builds using 16 cores, so rebuilds are faster)

To develop on your own branch, follow a tutorial such as: [https://git-scm.com/book/en/v2/Git-Branching-Basic-Branching-and-Merging](https://git-scm.com/book/en/v2/Git-Branching-Basic-Branching-and-Merging).


## Code Organization

The kscript C-API and library are large, but are organized as such:

  * Main C-API, general functionality
    * Documented in `include/ks/ks.h`, `include/ks/types.h` (builtin types)
    * Main code is `src/*.c`, and `src/types/*.c` (builtin types)
    * Some other functionality is in `include/ks`
    * Builtin functions are in `src/funcs.c`
  * Standard modules, which can be imported via the `ks_import` function (although they are bundled in the standard distribution)
    * Documented in `include/ks/<modulename>.h`
    * Code is in `src/modules/<modulename>`
  * Extra modules, which can also be imported via the `ks_import` function (although they are loaded as DLLs/shared libraries)
    * Documented in `include/ks/<modulename>.h`
    * Code is in `src/extmodules/<modulename>`
    * Output is `ksm_<modulename>.so` (or `.dll`, `.dylib` on some platforms)

Other than the actual C-API, there are also the following areas:
 
  * There is a folder, `winbuild`, which contains projects for compiling on Windows. It is sometimes not up to date, however
  * Within `tools`, there are code generation tools, packaging utilities, and other helpful scripts to automate tasks
  * Within `extras`, there are extra projects which are not central to kscript. For example, editor extensions may go in there, as well as logos/branding
  * Within `docs`, there are documentation resources for a formal manual (this is currently a WIP, and I am planning to write my own documentation generator soon, which will replace the texinfo)
  * Within `tests`, there are a number of test cases (`.ks` are written in kscript, and should run successfully)


## Changing Code

Say we want to change code -- we'll use a simple example -- what needs to be done?

For changing behavior of one of the builtin types (say, `int`), the file responsible is typically `src/types/<name>.c`, (so, `src/types/int.c`). From there, you can search or scroll for your particular functions.

For changing the builtin modules, check `src/modules/<name>` (i.e. `src/modules/os`) and see which files look relevant. Typically, there will be a `module.c` which contains the exported function definitions. However, if a module defines extra types, they may be in extra files, within the subtree (for example, `src/modules/os/path.c` for the `path` type)

For changing the general kscript API and utilities, those files are typically located in `src/` (i.e. `src/init.c` for initialization routines).


## Adding a function to a standard module

1. Ensure that any required libraries are included in the `configure` script
2. In the module header (`include/ks/<module>.h`), create a function prototype using `KS_API` macro; for example, `KS_API bool ksos_setenv(ks_str key, ks_str val);`
3. In the relevant module src file (example: `src/modules/<module>/main.c`), add dictionary entry to `KS_IKV` with string representing function as key, and `ksf_wrap(M_<func>_, M_NAME ".<func>(parameters)", "function description")`
4. Write the function in the src file, using the `static KS_TFUNC(M, <func>) { ... }` code. See other modules for an example of how to do this

