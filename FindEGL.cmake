find_path(EGL_INCLUDE_DIR NAMES EGL/egl.h PATHS ${CMAKE_SOURCE_DIR}/thirdparty/GLES_SDK/include)

set(EGL_NAMES ${EGL_NAMES} egl EGL)
if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(ARCH_SUFFIX "-64")
else()
    set(ARCH_SUFFIX "")
endif()
find_library(EGL_LIBRARY NAMES ${EGL_NAMES} PATHS ${CMAKE_SOURCE_DIR}/thirdparty/GLES_SDK/x86${ARCH_SUFFIX})

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(EGL DEFAULT_MSG EGL_LIBRARY EGL_INCLUDE_DIR)