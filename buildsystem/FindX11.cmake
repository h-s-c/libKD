##  Public Domain
##
##  X11_FOUND
##  X11_INCLUDE_DIR
##  X11_LIBRARY
##

find_path(X11_INCLUDE_DIR NAMES X11/Xlib.h PATH_SUFFIXES X11)
find_library(X11_LIBRARY NAMES X11)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(X11 DEFAULT_MSG X11_LIBRARY X11_INCLUDE_DIR)
mark_as_advanced(X11_LIBRARY X11_INCLUDE_DIR)

