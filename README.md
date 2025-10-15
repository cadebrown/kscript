# kscript (ks) - ARCHIVED

**NOTE: This project is no longer being actively developed or maintained, and has been archived. A spiritual successor is planned...**

kscript ([ks.cade.io](https://ks.cade.io)) is a dynamic programming language with expressive syntax, cross platform support, and a rich standard library. It's like Python, but better 😉.

I wrote kscript mainly as a learning experience, to see if I could write a full programming language from scratch in C99. I did, but at what cost?

* [ks.cade.io/repl](https://ks.cade.io/repl) - online REPL with WASM
  * Run kscript in your browser!
* [ks.cade.io](https://ks.cade.io) - online documentation
  * A full documentation of the language and standard library, including examples

## Usage

### Online REPL via WASM

To quickly test out kscript, run it online at [ks.cade.io](https://ks.cade.io). This is a REPL (Read-Eval-Print Loop) that runs locally in your browser using [WebAssembly (WASM)](https://webassembly.org/).

### Running Locally

To run kscript locally, download a pre-built binary from the [releases page](https://github.com/cadebrown/kscript/releases) for your platform.

When you unarchive it, you should see a `bin/` folder with the `ks` executable inside. You can run it like so:

```shell
$ ./bin/ks -c 'print(1+2+3+4)'
10
```

## Setup

If you want to build it yourself, or check out how development works, follow these instructions for your platform:

### Setup: Linux

First, install the required dependencies for your system:

```shell
# on Debian/Ubuntu/apt-based Linux systems
$ sudo apt install libpthread-stubs0-dev libgmp-dev libreadline-dev libfftw3-dev libavcodec-dev libavformat-dev libavutil-dev libswscale-dev libffi-dev

# others: figure it out!
```

Now, configure the build system to find these dependencies:

```shell
# run with --help for more options
$ ./configure --with-readline --with-ffi --with-pthreads --with-libav --with-opengl --with-glfw --prefix $PWD/install
```

And finally, build the entire project with:

```shell
# use -j<N> to parallelize the build with N jobs
$ make -j16
```

Optionally, you can also "install" it to the location specified with `--prefix` above:

```shell
$ make install
```

### Setup: MacOS

First, install the required dependencies for your system using [Homebrew](https://brew.sh/) (install Homebrew if you haven't already):

```shell
$ brew install libffi ffmpeg glfw
```

Next, we need to add these libraries to our path, so they are found by our configuration system:

```shell
# add the library path, where dynamic libraries are found
$ export LIBRARY_PATH="$LIBRARY_PATH:/opt/homebrew/lib:/opt/homebrew/opt/libffi/lib"

# add the include path, where C/C++ header files are found
$ export C_INCLUDE_PATH="$C_INCLUDE_PATH:/opt/homebrew/include:/opt/homebrew/opt/libffi/include"
```

With those set, we can now configure kscript to find these libraries:

```shell
$ ./configure --with-readline --with-ffi --with-pthreads --with-libav --with-opengl --with-glfw
```

And finally, build the entire project with:

```shell
# use -j<N> to parallelize the build with N jobs
$ make -j16
```

Optionally, you can also "install" it to the location specified with `--prefix` above:

```shell
$ make install
```

### Setup: Emscripten (WASM)

You can also build WASM binaries using [Emscripten](https://emscripten.org/) (a great project, BTW). This allows you to run kscript in the browser, or anywhere that supports WebAssembly.

First, install Emscripten by following their instructions: [Emscripten: Getting Started](https://emscripten.org/docs/getting_started/downloads.html). You should install and activate their SDK.

For example, to install v4.0.15 (which is comfirmed to work with kscript), you can run:

```shell
$ git clone https://github.com/emscripten-core/emsdk.git
$ cd emsdk
$ ./emsdk install 4.0.15
$ ./emsdk activate 4.0.15
$ source ./emsdk_env.sh
```


Now, configure the build system to use Emscripten, which requires manually overriding some environment variables:

```shell
$ CC=emcc CFLAGS="-O3 -Wno-ignored-attributes -sASSERTIONS=1" LDFLAGS='-O3 -sWASM=1 -sASSERTIONS=1 -sERROR_ON_UNDEFINED_SYMBOLS=0 -sEXPORTED_RUNTIME_METHODS=["cwrap","ccall","stringToUTF8","UTF8ToString"]' PLATFORM="web" ./configure


Finally, you can build the same way as native builds:

```shell
# use -j<N> to parallelize the build with N jobs
$ make -j16
```


### Setup: Windows

Windows was never really fully supported, but can be built with the Visual Studio solution project in the [./winbuild](./winbuild) folder.

Good luck!

## Running

```shell

# the most basic program
$ ./bin/ks examples/greet.ks
hello, <user>

# compute a Fibonacci number (the 10th in this example)
$ ./bin/ks examples/fib.ks 10
55

```

## Making Documentation

Inside the `docs/` folder, you can run:

```shell
$ pandoc DOCS.md -o index.html --template template.html -s --toc --number-sections --lua-filter=pandoc/anchors.lua --syntax-highlighting=none
```

And, to generate a PDF copy:

```shell
$ pandoc DOCS.md -o kscript.pdf -s --toc --number-sections --pdf-engine=xelatex -V geometry:a4paper -V geometry:margin=2cm -V fontsize=12pt
```

## TODO for final release

* implement sum/min/max functions
* implement display in REPL with backslash-escaped printing
