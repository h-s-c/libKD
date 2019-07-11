###############################################################################
# libKD
# zlib/libpng License
###############################################################################
# Copyright (c) 2014-2019 Kevin Schmidt
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

set(CPACK_CMAKE_GENERATOR "Ninja")

set(CPACK_PACKAGE_NAME "libkd")
set(CPACK_PACKAGE_CONTACT "kevin.patrick.schmidt@googlemail.com")
set(CPACK_PACKAGE_VENDOR "Kevin Schmidt")
set(CPACK_PACKAGE_DESCRIPTION "OpenKODE Core implementation")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "OpenKODE Core implementation")

find_program(GIT_COMMAND NAMES git)
execute_process(COMMAND ${GIT_COMMAND} describe origin/master
    WORKING_DIRECTORY "$ENV{PWD}"
    OUTPUT_VARIABLE CPACK_PACKAGE_VERSION 
    OUTPUT_STRIP_TRAILING_WHITESPACE)

string(REGEX MATCHALL "-.*$|[0-9]+" CPACK_PACKAGE_VERSION ${CPACK_PACKAGE_VERSION})
list(GET CPACK_PACKAGE_VERSION 0 CPACK_PACKAGE_VERSION_MAJOR)
list(GET CPACK_PACKAGE_VERSION 1 CPACK_PACKAGE_VERSION_MINOR)
list(GET CPACK_PACKAGE_VERSION 2 CPACK_PACKAGE_VERSION_PATCH)
set(CPACK_PACKAGE_VERSION "${CPACK_PACKAGE_VERSION_MAJOR}.${CPACK_PACKAGE_VERSION_MINOR}.${CPACK_PACKAGE_VERSION_PATCH}")

set(CPACK_INSTALL_CMAKE_PROJECTS "build;KD;ALL;/")

if(UNIX)
    execute_process(COMMAND cat /etc/os-release COMMAND grep -c Ubuntu OUTPUT_VARIABLE UBUNTU)
    execute_process(COMMAND cat /etc/os-release COMMAND grep -c Debian OUTPUT_VARIABLE DEBIAN)
    execute_process(COMMAND cat /etc/os-release COMMAND grep -c Fedora OUTPUT_VARIABLE FEDORA)
    execute_process(COMMAND cat /etc/os-release COMMAND grep -c CentOS OUTPUT_VARIABLE CENTOS)
    execute_process(COMMAND cat /etc/os-release COMMAND grep -c Oracle OUTPUT_VARIABLE ORACLE)
    execute_process(COMMAND cat /etc/os-release COMMAND grep -c openSUSE OUTPUT_VARIABLE OPENSUSE)
endif()

if(UBUNTU GREATER 0 OR DEBIAN GREATER 0)
    set(CPACK_GENERATOR "DEB")
    find_program(DPKG_COMMAND dpkg)
    execute_process(COMMAND ${DPKG_COMMAND} --print-architecture
        OUTPUT_VARIABLE CPACK_DEBIAN_PACKAGE_ARCHITECTURE
        OUTPUT_STRIP_TRAILING_WHITESPACE)
    #set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)
    set(CPACK_DEBIAN_PACKAGE_DEPENDS "libegl1-mesa, libgles2-mesa, libx11, libwayland-client0, libwayland-egl1-mesa")
    set(CPACK_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME}_${CPACK_PACKAGE_VERSION}_${CPACK_DEBIAN_PACKAGE_ARCHITECTURE}")
elseif(FEDORA GREATER 0 OR CENTOS GREATER 0 OR ORACLE GREATER 0 OR OPENSUSE GREATER 0)
    set(CPACK_GENERATOR "RPM")
    exec_program(uname ARGS -m OUTPUT_VARIABLE CPACK_RPM_PACKAGE_ARCHITECTURE)
    if(FEDORA GREATER 0 OR CENTOS GREATER 0 OR ORACLE GREATER 0)
        set(CPACK_RPM_PACKAGE_REQUIRES "mesa-libEGL, mesa-libGLES, libX11, libwayland-client, mesa-libwayland-egl")
    elseif(OPENSUSE GREATER 0)
        set(CPACK_RPM_PACKAGE_REQUIRES "Mesa-libEGL1, Mesa-libGLESv2-2, libX11, libwayland-client0, libwayland-egl1")
    endif()
    set(CPACK_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME}-${CPACK_PACKAGE_VERSION}.${CPACK_RPM_PACKAGE_ARCHITECTURE}")
endif()
