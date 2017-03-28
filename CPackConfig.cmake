###############################################################################
# libKD
# zlib/libpng License
###############################################################################
# Copyright (c) 2014-2017 Kevin Schmidt
#
# This software is provided 'as-is', without any express or implied
# warranty. In no event will the authors be held liable for any damages
# arising from the use of this software.
#
# Permission is granted to anyone to use this software for any purpose,
# including commercial applications, and to alter it and redistribute it
# freely, subject to the following restrictions:
#
# 1. The origin of this software must not be misrepresented; you must not
#    claim that you wrote the original software. If you use this software
#    in a product, an acknowledgment in the product documentation would be
#    appreciated but is not required.
# 2. Altered source versions must be plainly marked as such, and must not be
#    misrepresented as being the original software.
# 3. This notice may not be removed or altered from any source distribution.
###############################################################################

set(CPACK_PACKAGE_NAME "kd")
set(CPACK_PACKAGE_CONTACT "h-s-c@users.noreply.github.com")
set(CPACK_PACKAGE_VENDOR "Kevin Schmidt")
set(CPACK_PACKAGE_DESCRIPTION "OpenKODE Core implementation")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "OpenKODE Core implementation")
set(CPACK_PACKAGE_VERSION_MAJOR "0")
set(CPACK_PACKAGE_VERSION_MINOR "2")
set(CPACK_PACKAGE_VERSION_PATCH "0")

if(DEFINED ENV{TRAVIS_TAG})
    string(SUBSTRING $ENV{TRAVIS_TAG} 1 -1 TAG)
    set(CPACK_PACKAGE_VERSION ${TAG})
else()
    set(CPACK_PACKAGE_VERSION "${CPACK_PACKAGE_VERSION_MAJOR}.${CPACK_PACKAGE_VERSION_MINOR}.${CPACK_PACKAGE_VERSION_PATCH}")
endif()

set(CPACK_CMAKE_GENERATOR "Unix Makefiles")
set(CPACK_INSTALL_CMAKE_PROJECTS "build;KD;ALL;/")

set(ARCH "${CMAKE_SYSTEM_PROCESSOR}")
if(UNIX)
    execute_process(COMMAND cat /etc/os-release COMMAND grep -c Ubuntu OUTPUT_VARIABLE UBUNTU)
    execute_process(COMMAND cat /etc/os-release COMMAND grep -c Debian OUTPUT_VARIABLE DEBIAN)
    execute_process(COMMAND cat /etc/os-release COMMAND grep -c Fedora OUTPUT_VARIABLE FEDORA)
    execute_process(COMMAND cat /etc/os-release COMMAND grep -c openSUSE OUTPUT_VARIABLE OPENSUSE)
endif()

if(UBUNTU EQUAL 1 OR DEBIAN EQUAL 1)
    set(CPACK_GENERATOR "DEB")
    set(CPACK_DEBIAN_PACKAGE_DEPENDS "libegl1-mesa, libgles2-mesa, libx11, wayland")
    if(ARCH STREQUAL "x86_64")
        set(ARCH "amd64")
    endif ()
    set(CPACK_DEBIAN_PACKAGE_ARCHITECTURE ${ARCH})
    set(CPACK_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME}_${CPACK_PACKAGE_VERSION}_${CPACK_DEBIAN_PACKAGE_ARCHITECTURE}")
elseif(FEDORA EQUAL 1 OR OPENSUSE EQUAL 1)
    set(CPACK_GENERATOR "RPM")
    set(CPACK_RPM_PACKAGE_REQUIRES "Mesa-libEGL, Mesa-libGLESv2, libX11, wayland")
    set(CPACK_RPM_PACKAGE_ARCHITECTURE ${ARCH})
    set(CPACK_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME}-${CPACK_PACKAGE_VERSION}.${CPACK_RPM_PACKAGE_ARCHITECTURE}")
endif()
