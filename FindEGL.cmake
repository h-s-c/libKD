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

if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(ARCH_SUFFIX "-64")
else()
    set(ARCH_SUFFIX "")
endif()

find_path(EGL_INCLUDE_DIR NAMES EGL/egl.h PATHS $ENV{KHRONOS_HEADERS}
                                                ${CMAKE_SOURCE_DIR}/thirdparty/gles_angle/include
                                                ${CMAKE_SOURCE_DIR}/thirdparty/gles_amd/include
                                                ${CMAKE_SOURCE_DIR}/thirdparty/gles_mali/include)

find_library(EGL_LIBRARY NAMES egl EGL libEGL PATHS $ENV{OPENGLES_LIBDIR}
                                                    ${CMAKE_SOURCE_DIR}/thirdparty/gles_angle/lib
                                                    ${CMAKE_SOURCE_DIR}/thirdparty/gles_amd/x86${ARCH_SUFFIX}
                                                    ${CMAKE_SOURCE_DIR}/thirdparty/gles_mali
                                                    ${CMAKE_SOURCE_DIR}/thirdparty/gles_mali/lib)

if(EMSCRIPTEN)
    set(EGL_FOUND TRUE)
    SET(EGL_INCLUDE_DIR "${EMSCRIPTEN_ROOT_PATH}/system/include")
    set(EGL_LIBRARY "nul")
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(EGL DEFAULT_MSG EGL_LIBRARY)
mark_as_advanced(EGL_INCLUDE_DIR EGL_LIBRARY)