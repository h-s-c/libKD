if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(ARCH_SUFFIX "-64")
else()
    set(ARCH_SUFFIX "")
endif()

find_path(EGL_INCLUDE_DIR NAMES EGL/egl.h PATHS $ENV{KHRONOS_HEADERS}
                                                ${CMAKE_SOURCE_DIR}/thirdparty/gles_amd/include
                                                ${CMAKE_SOURCE_DIR}/thirdparty/gles_mali/include)
find_library(EGL_LIBRARY NAMES egl EGL libEGL PATHS $ENV{OPENGLES_LIBDIR}
                                                    ${CMAKE_SOURCE_DIR}/thirdparty/gles_amd/x86${ARCH_SUFFIX}
                                                    ${CMAKE_SOURCE_DIR}/thirdparty/gles_mali
                                                    ${CMAKE_SOURCE_DIR}/thirdparty/gles_mali/lib)


if(EMSCRIPTEN)
    set(EGL_LIBRARY EGL)
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(EGL DEFAULT_MSG EGL_LIBRARY EGL_INCLUDE_DIR)