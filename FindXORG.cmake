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
pkg_check_modules(PC_XORG QUIET xcb xcb-ewmh xcb-icccm xkbcommon)

find_path(XCB_INCLUDE_DIR NAMES xcb/xcb.h PATHS ${PC_XORG_INCLUDE_DIRS})
find_path(XCB_EWHM_INCLUDE_DIR NAMES xcb/xcb_ewmh.h PATHS ${PC_XORG_INCLUDE_DIRS})
find_path(XCB_ICCCM_INCLUDE_DIR NAMES xcb/xcb_icccm.h PATHS ${PC_XORG_INCLUDE_DIRS})
find_path(XKBCOMMON_INCLUDE_DIR NAMES xkbcommon/xkbcommon.h PATHS ${PC_XORG_INCLUDE_DIRS})
find_path(XKBCOMMON_X11_INCLUDE_DIR NAMES xkbcommon/xkbcommon-x11.h PATHS ${PC_XORG_INCLUDE_DIRS})
set(XORG_INCLUDE_DIR ${XCB_INCLUDE_DIR} ${XCB_EWHM_INCLUDE_DIR} ${XCB_ICCCM_INCLUDE_DIR} ${XKBCOMMON_INCLUDE_DIR} ${XKBCOMMON_X11_INCLUDE_DIR})
list(REMOVE_DUPLICATES XORG_INCLUDE_DIR)

find_library(XCB_LIBRARY NAMES xcb PATHS ${PC_XORG_LIBRARY_DIRS})
find_library(XCB_EWHM_LIBRARY NAMES xcb-ewmh PATHS ${PC_XORG_LIBRARY_DIRS})
find_library(XCB_ICCCM_LIBRARY NAMES xcb-icccm PATHS ${PC_XORG_LIBRARY_DIRS})
find_library(XKBCOMMON_LIBRARY NAMES xkbcommon PATHS ${PC_XORG_LIBRARY_DIRS})
find_library(XKBCOMMON_X11_LIBRARY NAMES xkbcommon-x11 PATHS ${PC_XORG_LIBRARY_DIRS})
set(XORG_LIBRARIES ${XCB_LIBRARY} ${XCB_EWHM_LIBRARY} ${XCB_ICCCM_LIBRARY} ${XKBCOMMON_LIBRARY} ${XKBCOMMON_X11_LIBRARY})

find_package_handle_standard_args(XORG DEFAULT_MSG XORG_LIBRARIES)
mark_as_advanced(XCB_INCLUDE_DIR XCB_EWHM_INCLUDE_DIR XCB_ICCCM_INCLUDE_DIR XKBCOMMON_INCLUDE_DIR XKBCOMMON_X11_INCLUDE_DIR XCB_LIBRARY XCB_EWHM_LIBRARY XCB_ICCCM_LIBRARY XKBCOMMON_LIBRARY XKBCOMMON_X11_LIBRARY)