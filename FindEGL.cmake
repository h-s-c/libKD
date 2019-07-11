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


find_path(EGL_INCLUDE_DIR NAMES EGL/egl.h PATHS $ENV{KHRONOS_HEADERS})
find_library(EGL_LIBRARY NAMES egl EGL libEGL PATHS $ENV{OPENGLES_LIBDIR})

include(FindPackageHandleStandardArgs)

if(EMSCRIPTEN)
    set(EGL_INCLUDE_DIR "${EMSCRIPTEN_ROOT_PATH}/system/include")
    find_package_handle_standard_args(EGL DEFAULT_MSG EGL_INCLUDE_DIR)
    set(EGL_LIBRARY "")
else()
    find_package_handle_standard_args(EGL DEFAULT_MSG EGL_INCLUDE_DIR EGL_LIBRARY)
endif()

mark_as_advanced(EGL_INCLUDE_DIR EGL_LIBRARY)