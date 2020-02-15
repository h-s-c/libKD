# OpenKODE Core implementation
[![Zlib License](https://img.shields.io/:license-zlib-blue.svg)](https://opensource.org/licenses/Zlib)
[![Git Flow](https://img.shields.io/:standard-gitflow-green.svg)](http://nvie.com/git-model)
[![Semantic Versioning](https://img.shields.io/:standard-semver-green.svg)](http://semver.org)

##
[![Travis CI](https://img.shields.io/travis/h-s-c/libKD/develop.svg?label=TravisCI)](https://travis-ci.org/h-s-c/libKD)  
[![AppVeyor CI](https://img.shields.io/appveyor/ci/h-s-c/libKD/develop.svg?label=AppVeyorCI)](https://ci.appveyor.com/project/h-s-c/libKD)  
[![Coverity Scan](https://img.shields.io/coverity/scan/3798.svg?label=CoverityScan)](https://scan.coverity.com/projects/h-s-c-libkd)  
[![Codecov](https://img.shields.io/codecov/c/github/h-s-c/libKD/develop.svg?label=Codecov)](https://codecov.io/gh/h-s-c/libKD)  
[![Fossa License Scan](https://app.fossa.com/api/projects/git%2Bgithub.com%2Fh-s-c%2FlibKD.svg?type=shield)](https://app.fossa.com/projects/git%2Bgithub.com%2Fh-s-c%2FlibKD?ref=badge_shield)

### About
-   Cross-platform system API similar to SDL created by Khronos (OpenGL etc.)
-   [Specification](https://www.khronos.org/registry/kode/)

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