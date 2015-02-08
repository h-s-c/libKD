**OpenKODE Core implementation.**

###License
[![Zlib License](http://img.shields.io/:license-zlib-blue.svg)](http://opensource.org/licenses/Zlib)

###Status
[![Travis Status](https://img.shields.io/travis/h-s-c/libKD.svg)](https://travis-ci.org/h-s-c/libKD)
#[![AppVeyor Status](https://img.shields.io/appveyor/ci/h-s-c/libKD.svg)](https://ci.appveyor.com/project/h-s-c/libkd)
#[![Drone Status](https://drone.io/github.com/h-s-c/libKD/status.png)](https://drone.io/github.com/h-s-c/libKD/latest)
[![Coveralls Status](https://img.shields.io/coveralls/h-s-c/libKD.svg)](https://coveralls.io/r/h-s-c/libKD)
#[![Coverity Status](https://scan.coverity.com/projects/3798/badge.svg)](https://scan.coverity.com/projects/3798)

[![Waffle Ready](https://badge.waffle.io/h-s-c/libKD.png?label=ready&title=Ready)](https://waffle.io/h-s-c/libKD)
[![Waffle In Progress](https://badge.waffle.io/h-s-c/libKD.png?label=In%20Progress&title=In%20Progress )](https://waffle.io/h-s-c/libKD)

###Notes
-   [Specification](https://www.khronos.org/registry/kode/)
-   Only kd.c + headers are needed in case you don't want to use CMake
-   Code is NOT production ready

###Dependencies
-   POSIX, C11 (C11 atomics NOT optional; C11 threads optional; C11 Annex K optional)
-   EGL, X11
