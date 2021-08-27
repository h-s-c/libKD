###############################################################################
# libKD
# zlib/libpng License
###############################################################################
# Copyright (c) 2014-2021 Kevin Schmidt
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

find_package(PkgConfig)
pkg_check_modules(PC_WAYLAND QUIET wayland-client wayland-egl xkbcommon)

find_path(WAYLAND_CLIENT_INCLUDE_DIR NAMES wayland-client.h PATHS ${PC_WAYLAND_INCLUDE_DIRS})
find_path(WAYLAND_EGL_INCLUDE_DIR NAMES wayland-egl.h PATHS ${PC_WAYLAND_INCLUDE_DIRS})
find_path(XKBCOMMON_INCLUDE_DIR NAMES xkbcommon/xkbcommon.h PATHS ${PC_WAYLAND_INCLUDE_DIRS})
set(WAYLAND_INCLUDE_DIR ${WAYLAND_CLIENT_INCLUDE_DIR} ${WAYLAND_EGL_INCLUDE_DIR} ${XKBCOMMON_INCLUDE_DIR})
list(REMOVE_DUPLICATES WAYLAND_INCLUDE_DIR)

find_library(WAYLAND_CLIENT_LIBRARY NAMES wayland-client PATHS ${PC_WAYLAND_LIBRARY_DIRS})
find_library(WAYLAND_EGL_LIBRARY NAMES wayland-egl PATHS ${PC_WAYLAND_LIBRARY_DIRS})
find_library(XKBCOMMON_LIBRARY NAMES xkbcommon PATHS ${PC_WAYLAND_LIBRARY_DIRS})
set(WAYLAND_LIBRARIES ${WAYLAND_CLIENT_LIBRARY} ${WAYLAND_EGL_LIBRARY} ${XKBCOMMON_LIBRARY})

find_package_handle_standard_args(WAYLAND DEFAULT_MSG WAYLAND_LIBRARIES)
mark_as_advanced(WAYLAND_CLIENT_INCLUDE_DIR WAYLAND_EGL_INCLUDE_DIR XKBCOMMON_INCLUDE_DIR WAYLAND_CLIENT_LIBRARY WAYLAND_EGL_LIBRARY XKBCOMMON_LIBRARY)