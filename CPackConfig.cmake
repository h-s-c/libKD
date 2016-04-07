###############################################################################
# libKD
# zlib/libpng License
###############################################################################
# Copyright (c) 2014-2016 Kevin Schmidt
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
set(CPACK_PACKAGE_DESCRIPTION "OpenKODE Core implementation")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "OpenKODE Core implementation")
set(CPACK_PACKAGE_VERSION_MAJOR "0")
set(CPACK_PACKAGE_VERSION_MINOR "1")
set(CPACK_PACKAGE_VERSION_PATCH "0")

if(DEFINED ENV{TRAVIS_TAG})
    set(CPACK_PACKAGE_VERSION $ENV{TRAVIS_TAG})
else()
    set(CPACK_PACKAGE_VERSION "${CPACK_PACKAGE_VERSION_MAJOR}.${CPACK_PACKAGE_VERSION_MINOR}.${CPACK_PACKAGE_VERSION_PATCH}-test")
endif()

set(CPACK_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME}_${CPACK_PACKAGE_VERSION}_amd64")
set(CPACK_GENERATOR "DEB")
set(CPACK_DEBIAN_PACKAGE_MAINTAINER "Kevin Schmidt")
set(CPACK_DEBIAN_PACKAGE_ARCHITECTURE "amd64")
set(CPACK_DEBIAN_PACKAGE_HOMEPAGE "https://github.com/h-s-c/libKD")
set(CPACK_DEBIAN_PACKAGE_DEPENDS "")

set(CPACK_CMAKE_GENERATOR "Unix Makefiles")
set(CPACK_INSTALL_CMAKE_PROJECTS "build;KD;ALL;/")