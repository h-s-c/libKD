**OpenKODE Core implementation.**

###License
[![Zlib License](http://img.shields.io/:license-zlib-blue.svg)](http://opensource.org/licenses/Zlib)

###Status
[![Travis Status](https://img.shields.io/travis/h-s-c/libKD.svg)](https://travis-ci.org/h-s-c/libKD)
[![AppVeyor Status](https://img.shields.io/badge/build-unknown-lightgrey.svg)](https://ci.appveyor.com/project/h-s-c/libkd)
[![Drone Status](https://drone.io/github.com/h-s-c/libKD/status.png)](https://drone.io/github.com/h-s-c/libKD/latest)
[![Coveralls Status](https://img.shields.io/coveralls/h-s-c/libKD.svg)](https://coveralls.io/r/h-s-c/libKD)
[![Coverity Status](https://scan.coverity.com/projects/3798/badge.svg)](https://scan.coverity.com/projects/3798)

###Notes
-   [Specification](https://www.khronos.org/registry/kode/)
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