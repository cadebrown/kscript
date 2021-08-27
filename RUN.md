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