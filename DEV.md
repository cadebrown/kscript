# Developer Guide

This document describes how to develop kscript, contribute to the repository, and so on. See [https://github.com/ChemicalDevelopment/kscript/issues/3](https://github.com/ChemicalDevelopment/kscript/issues/3) if you have any issues with this guide

## Work In Progress

The current areas that need to be worked on:
  * Implementing `nx` library efficiently without such a huge code bloat
  * Re-writing the parser for faster speed and less stack space (perhaps we should go back to a Shunting-Yard based approach)

## Setup

To set up, make sure you've cloned the repository (`git clone https://github.com/ChemicalDevelopment/kscript.git`), and then can run `./configure` and `make` to build it.

From there, you are set! Just edit some files, and then re-run `make` to update the program. For example:

```bash
$ make -j16 && ./bin/ks -e 'print (1, 2, 3)'
1 2 3 
```

(the `-j16` builds using 16 cores, so rebuilds are faster)

To develop on your own branch, follow a tutorial such as: [https://git-scm.com/book/en/v2/Git-Branching-Basic-Branching-and-Merging](https://git-scm.com/book/en/v2/Git-Branching-Basic-Branching-and-Merging).

## Changing Code

Say we want to change code -- we'll use a simple example -- what needs to be done?

For changing behavior of one of the builtin types (say, `int`), the file responsible is typically `src/types/<name>.c`, (so, `src/types/int.c`). From there, you can search or scroll for your particular functions.

For changing the builtin modules, check `src/modules/<name>` (i.e. `src/modules/os`) and see which files look relevant. Typically, there will be a `module.c` which contains the exported function definitions. However, if a module defines extra types, they may be in extra files, within the subtree (for example, `src/modules/os/path.c` for the `path` type)

For changing the general kscript API and utilities, those files are typically located in `src/` (i.e. `src/init.c` for initialization routines).

## Commiting Changes

To commit your changes, add any files you changed via `git add` (for example, `git add src/types/int.c`), then run `git commit` and enter your message (use `git commit -m "MESSAGE"` to not use an editor).

Finally, run `git push` to push it to the GitHub repo. Note that most of the time you'll be pushing it to another branch than `master` which you should have already checked out. You can check out (or fork the repo, and send a pull request) your own branch if you are working on code that has a lot of interop with other code.



