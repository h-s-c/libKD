if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(ARCH_SUFFIX "-64")
else()
    set(ARCH_SUFFIX "")
endif()

find_path(GLES2_INCLUDE_DIR NAMES GLES2/gl2.h PATHS $ENV{KHRONOS_HEADERS}
                                                    ${CMAKE_SOURCE_DIR}/thirdparty/gles_amd/include
                                                    ${CMAKE_SOURCE_DIR}/thirdparty/gles_mali/include)
find_library(GLES2_LIBRARY NAMES GLESv2 libGLESv2 PATHS $ENV{OPENGLES_LIBDIR}
                                                        ${CMAKE_SOURCE_DIR}/thirdparty/gles_amd/x86${ARCH_SUFFIX}
                                                        ${CMAKE_SOURCE_DIR}/thirdparty/gles_mali
                                                        ${CMAKE_SOURCE_DIR}/thirdparty/gles_mali/lib)

if(EMSCRIPTEN)
    set(GLES2_LIBRARY GLESv2)
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(GLES2 DEFAULT_MSG GLES2_LIBRARY GLES2_INCLUDE_DIR)