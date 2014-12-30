**OpenKODE Core implementation.**

[![Travis](https://img.shields.io/travis/h-s-c/libKD.svg)](https://travis-ci.org/h-s-c/libKD)
[![AppVeyor](https://img.shields.io/badge/build-unknown-lightgrey.svg)](https://ci.appveyor.com/project/h-s-c/libkd)
[![Coveralls](https://img.shields.io/coveralls/h-s-c/libKD.svg)](https://coveralls.io/r/h-s-c/libKD)
[![Coverity](https://scan.coverity.com/projects/3798/badge.svg)](https://scan.coverity.com/projects/3798)

###Notes
-   [Specification](https://www.khronos.org/registry/kode/)
-   zlib licensed (examples are public domain)
-   Only kd.c + headers are needed in case you don't use CMake.

###Dependencies
-   POSIX, C11 (C11 threads alternative implementation is included; 
    C11 Annex K is optional, BSD safe string functions can be used instead)
-   X11, EGL

###Todo:
-   Android support (Bionic libc does not have POSIX messages queues, 
    see 'sourcecode/thirdparty/mqueue')
-   Next-Gen window server support (Wayland/Mir)
-   Improve input handling
-   Networking
-   OS X/iOS support (no test devices)