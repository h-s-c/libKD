** OpenKODE Core implementation.**

###Notes
-   [Specification](https://www.khronos.org/registry/kode/)
-   zlib licensed
-   Only kd.c + headers are needed in case you don't use CMake.

###Dependencies
-   POSIX, C11 (C11 threads alternative implementation is included; C11 Annex K is optional)
-   X11, EGL

###Todo:
-   Android support (Bionic libc does not have POSIX messages queues; see 'sourcecode/thirdparty/mqueue')
-   Next-Gen window server support (Wayland/Mir)
-   Improve input handling
-   Networking
-   OS X/iOS support (no test devices)