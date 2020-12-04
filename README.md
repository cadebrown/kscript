# kscript (ks)

kscript is a dynamic, duck typed, easy-to-use language with a comprehensive standard library, including maths, numerical tools, GUI toolkits, networking packages, and more! Learn more here: [https://kscript.org](https://kscript.org)

kscript is a work-in-progress, but most of the important functionality is at least partially complete

Current Efforts:

  * Standard library (functions, types, modules)
  * Module and Package system
  * Online 

## About

kscript is a programming language meant to help developers solve problems quickly, efficiently, and in a reusable fashion. It includes a large standard library, with many functions, modules, types, and algorithms to perform common programming tasks quickly and without the headache of searching for third party libraries (including: graphs, networking, servers, GUI toolkits, media, math libraries, and more).


## Current TODO

The largest needs for kscript at the moment are:

  * `Path`, `Stream`, `FileStream`, etc. types for dealing with wrapping OS- and FS-specific details (including operations)
  * Standard modules:
    * `nx`, `net`, `getarg`, `libc`, `m` (extended methods past C's `libm`), `media`
  * Importing modules and other source files
  * C-extension dynamically loaded support (mostly done -- test edge cases)
  * Packaging, installing packages (i.e. similar to `pip`)
  * Documentation site


## Running

To run kscript, you should have either installed or built kscript. These examples will use `ks` as the binary (if it is only built locally, you may have to replace that with `./bin.ks` or `./bin/ks.exe` on Windows)

```bash
$ ks -h
Usage: ks [options] FILE [args...]
       ks [options] -e 'EXPR' [args...]
       ks [options] - [args...]

Options:
  -h,--help             Prints this help/usage message
  -e,--expr [EXPR]      Run an inline expression, instead of a file
  -                     Start an interactive REPL shell
  -v[vv]                Increase verbosity (use '-vvv' for 'TRACE' level)
  -V,--version          Print out just the version information for kscript

kscript v0.0.1 release Mar 22 2020 14:12:31
Cade Brown <brown.cade@gmail.com>
```

To execute and display an expression, run:

```bash
$ ks -e '1 + 2**4'
17
```

## Installing

Right now, it is in an early alpha stage, so installers aren't ready. Check out the section below, on building it

## Building

This is a quick description that will work for most people and platforms, but check out [BUILD.md](./BUILD.md) for a more complete and customizable installation process

To build kscript, you first need to get a copy of the source code. To get the latest commit, use `git clone https://github.com/chemicaldevelopment/kscript`. This will clone the repo into a folder called `kscript`. You can also download a `.zip` from the [GitHub repo](https://github.com/chemicaldevelopment/kscript), and unzip it. 

Then, open a shell (`bash`, `zsh`, etc.) in the directory with the source code and follow the instructions below for your platform


### On Linux/Unix/MacOS

Requirements:

  1. A C compiler that supports `C99`, although most compilers will work even if they don't fully support `C99`
  2. A `make`-based build system

Optional Dependencies:

  1. The Posix threading library (`pthread`)
    * If this is not present, then threading support is emulated
  2. The GNU Multiple Precision library (`gmp`)
    * If this is not present, then kscript will use an implementation of a subset of GMP routines. This means operations with large integers will be slower
  3. The Fastest Fourier Transform in the West (`fftw`)
    * If this is not present, then the implementation of FFT plans may be slower


For example, you can install these on various platforms:

Debian/Ubuntu/etc.: `sudo apt install libpthread-stubs0-dev libgmp-dev libfftw3-dev`

Now, once you have installed the dependencies you want, you can build the library via:

```bash
./configure # give it '--help' to display options
make
make check # runs the standard tests
```


### On Windows

TODO: I've gotten it to build on Windows, but I need to make it more complete


## Examples

See the `examples/` folder for examples

### Hello World

This is the classic Hello World example

```kscript
print ("Hello World")
```

