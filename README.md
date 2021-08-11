# OpenKODE Core implementation
[![Zlib License](https://img.shields.io/:license-zlib-blue.svg)](https://opensource.org/licenses/Zlib)
[![Git Flow](https://img.shields.io/:standard-gitflow-green.svg)](http://nvie.com/git-model)

##
[![Build](https://img.shields.io/github/workflow/status/h-s-c/libKD/build%20and%20run%20tests)](https://github.com/h-s-c/libKD/actions)  
[![Coverage](https://img.shields.io/codecov/c/github/h-s-c/libKD/develop.svg?label=coverage)](https://codecov.io/gh/h-s-c/libKD)  
[![License Scan](https://app.fossa.com/api/projects/git%2Bgithub.com%2Fh-s-c%2FlibKD.svg?type=shield)](https://app.fossa.com/projects/git%2Bgithub.com%2Fh-s-c%2FlibKD?ref=badge_shield)

### About
-   Cross-platform system API similar to SDL created by Khronos (OpenGL etc.)
-   [Specification](https://www.khronos.org/registry/kode/)

### Examples
-   [Via WebAssembly running in your browser or Win64 binaries](https://h-s-c.github.io/libkd/)

### Platforms
-   Windows, Android, Linux, Web support
-   Experimental OSX/iOS support (needs an [EGL implementation](https://github.com/davidandreoletti/libegl/))

### Compilers
-   Visual C++ (2013 and up)
-   GCC (4.7 and up)
-   Clang/Xcode, Intel C++, Mingw-w64, Tiny C, Emscripten

## Linux Repositories
### Ubuntu/Debian
```bash
curl -s https://packagecloud.io/install/repositories/h-s-c/libKD/script.deb.sh | sudo bash
apt-get install libkd
```

### Fedora
```bash
curl -s https://packagecloud.io/install/repositories/h-s-c/libKD/script.rpm.sh | sudo bash
dnf install libkd
```

## Build from source
```bash
cmake -G Ninja -Bbuild -H.
ninja -C build
```