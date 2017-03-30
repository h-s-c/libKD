# OpenKODE Core implementation
[![Zlib License](https://img.shields.io/:license-zlib-blue.svg)](https://opensource.org/licenses/Zlib)  

### Status
[![Coverity Scan](https://img.shields.io/coverity/scan/3798.svg?label=CoverityScan)](https://scan.coverity.com/projects/h-s-c-libkd)  
  
[![Travis CI](https://img.shields.io/travis/h-s-c/libKD/master.svg?label=TravisCI)](https://travis-ci.org/h-s-c/libKD)  
[![AppVeyor CI](https://img.shields.io/appveyor/ci/h-s-c/libKD/master.svg?label=AppVeyorCI)](https://ci.appveyor.com/project/h-s-c/libKD)  

### About
-   Cross-platform system API similar to POSIX or SDL
-   [Specification](https://www.khronos.org/registry/kode/)

### Platforms
-   Windows, Android, Linux, Web (ASM.js) support
-   Experimental OSX/iOS support (needs an [EGL implementation](https://github.com/davidandreoletti/libegl/))

### Ubuntu/Debian Install
```bash
curl -s https://packagecloud.io/install/repositories/h-s-c/libKD/script.deb.sh | sudo bash
apt-get install libkd
```

### Fedora Install
```bash
curl -s https://packagecloud.io/install/repositories/h-s-c/libKD/script.rpm.sh | sudo bash
dnf install libkd
```

### Opensuse Install
```bash
curl -s https://packagecloud.io/install/repositories/h-s-c/libKD/script.rpm.sh | sudo bash
zypper install libkd
```

### RHEL/CentOS/Oracle/Amazon Install
```bash
curl -s https://packagecloud.io/install/repositories/h-s-c/libKD/script.rpm.sh | sudo bash
yum install libkd
```

### Compilers
-   Visual C++ (2013 and up)
-   GCC (4.7 and up)
-   Clang/Xcode, Intel C++, Mingw-w64, Tiny C, Emscripten

### Dependencies
-   EGL, OpenGL ES (samples)