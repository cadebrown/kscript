# kscript (ks)

kscript ([https://kscript.org](https://kscript.org)) is a dynamic programming language with expressive syntax, cross platform support, and a rich standard library. Its primary aim is to allow developers to write platform agnostic programs that can run anywhere, and require little or no platform- or os- specific code.

Documentation is available at [kscript.org](https://kscript.org), which provides examples, tutorials, and coverage of the standard library. Formal specifications are available at the GitHub repository ([https://github.com/ChemicalDevelopment/kscript](https://github.com/ChemicaldDvelopment/kscript)), within the `docs` folder.



## Current Status

kscript is currently a work-in-progress, aiming at an alpha release soon-ish. The most important internals are done (i.e. standard types, most standard functions). However, there are needs in a few areas:

  * Distribution to popular platforms (Linux, MacOS, Windows)
  * 


## Running

To run kscript, you should have either installed or built kscript. These examples will use `ks` as the binary (if it is only built locally, you may have to replace that with `./bin.ks` or `./bin/ks.exe` on Windows)

```shell
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

```shell
$ ks -e '1 + 2**4'
17
```

You can also run the interactive interpreter by running with no arguments (`ks`), or with `-` as the filename (`ks -`):

```shell
$ ks
>>> 1 + 2 ** 4
17
>>>
```

## Installing

Right now, it is in an early alpha stage, so installers aren't ready. Check out the section below, on building it

## Building

This is a quick description that will work for most people and platforms, but check out [BUILD.md](./BUILD.md) for a more complete and customizable installation process

To build kscript, you first need to get a copy of the source code. To get the latest commit, use `git clone https://github.com/chemicaldevelopment/kscript`. This will clone the repo into a folder called `kscript`. You can also download a `.zip` from the [GitHub repo](https://github.com/chemicaldevelopment/kscript), and unzip it. 

Then, open a shell (`bash`, `zsh`, etc.) in the directory with the source code and follow the instructions below for your platform

### On Linux/UNIX Systems

Requirements:

  1. A C compiler that supports `C99`, although most compilers will work even if they don't fully support `C99`
  2. A `make`-based build system

Optional Dependencies:

  1. The Posix threading library (`pthread`)
    * If this is not present, then threading support is emulated and some problems may arise with multi-threaded code
  2. The GNU Multiple Precision library (`gmp`)
    * If this is not present, then kscript will use an implementation of a subset of GMP routines. This means operations with large integers will be slower
  3. The GNU Readline library (`readline`)
    * If this is not present, then the kscript interpreter will not have auto-completion and advanced line-editing features
  4. The Fastest Fourier Transform in the West (`fftw3`)
    * If this is not present, then the implementation of FFT plans may be slower
  5. Libav (`libav`)
    * If this is not present, then `mm` won't support nearly as many media formats


For example, you can install these on various platforms:

Debian/Ubuntu/etc.: `sudo apt install libpthread-stubs0-dev libgmp-dev libffi-dev libreadline-dev libfftw3-dev libavcodec-dev libavformat-dev libavutil-dev libswscale-dev`

Now, once you have installed the dependencies you want, you can build the library via:

```shell
$ ./configure # give it '--help' to display options
$ make
$ make check # runs the standard tests
```

### On MacOS Systems

Due to some library incompatibilities, the surefire way to build kscript on MacOS is:

```shell
$ ./configure --with-ffi off --with-readline off
$ make bin/ks -j16
```

Some builds (external shared libraries, extra modules) may not work directly (TODO/WIP), but the basic functionality and the standard library should work just fine

### On Windows

TODO: I've gotten it to build on Windows, but I need to make it more complete so others can reproduce it

## Examples

See the `examples/` folder for examples

### Hello World

This is the classic Hello World example:

```ks
print ("Hello World")
```
