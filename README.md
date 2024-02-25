# OpenKODE Core implementation
[![Zlib License](https://img.shields.io/:license-zlib-blue.svg)](https://opensource.org/licenses/Zlib)
[![Git Flow](https://img.shields.io/:standard-gitflow-green.svg)](http://nvie.com/git-model)

##
[![Build](https://img.shields.io/github/actions/workflow/status/h-s-c/libKD/ctest.yml?branch=develop)](https://github.com/h-s-c/libKD/actions)  
[![Coverage](https://img.shields.io/codecov/c/github/h-s-c/libKD/develop.svg?label=coverage)](https://codecov.io/gh/h-s-c/libKD)  
[![License Scan](https://app.fossa.com/api/projects/git%2Bgithub.com%2Fh-s-c%2FlibKD.svg?type=shield)](https://app.fossa.com/projects/git%2Bgithub.com%2Fh-s-c%2FlibKD?ref=badge_shield)

### About
-   Cross-platform abstraction layer and application framework implemented in a clean, simple and maintainable manner. Written in C; build and tested with CMake and Github Actions; developed for Windows, Linux, Android and the Web.
-   [Specification](https://www.khronos.org/registry/kode/)

### Details
-   Threads and synchronization
-   Events
-   Application startup and exit
-   Memory allocation
-   Mathematical functions
-   String and memory functions
-   Time functions
-   File system
-   Network sockets
-   Input/output
-   Windowing
-   Assertions and logging

### Examples
-   [Binaries (Windows, Android, Web)](https://h-s-c.github.io/libkd/)

```C
#include <KD/kd.h>

KDint KD_APIENTRY kdMain(KDint argc, const KDchar *const *argv)
{
    KDboolean run = KD_TRUE;
    while(run)
    {
        const KDEvent *event = kdWaitEvent(-1);
        if(event)
        {
            switch(event->type)
            {
                case(KD_EVENT_QUIT):
                {
                    run = KD_FALSE;
                }
                default:
                {
                    kdDefaultEvent(event);
                }
            }
        }
    }
    return 0;
}
```

### Platforms
-   Windows, Android, Linux, Web support
-   Experimental OSX/iOS support (needs an [EGL implementation](https://github.com/davidandreoletti/libegl/))

### Compilers
-   Visual C++ (2013 and up)
-   GCC (4.7 and up)
-   Clang/Xcode, Intel C++, Mingw-w64, Tiny C, Emscripten

## Build from source
```bash
cmake -G Ninja -Bbuild -H.
ninja -C build
```