# cext-system

This C-style kscript extension shows how to wrap C code and make it callable from kscript.


## Building

To build, simply run `make` in the current path. It should build `lib/libksm_system.so`

## Running

Once built, run kscript and import `system`. For example:

```ks
>>> import system
>>> system.system("echo hello")
hello
0
```
