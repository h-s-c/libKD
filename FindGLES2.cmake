find_path(GLES2_INCLUDE_DIR NAMES GLES2/gl2.h PATHS ${CMAKE_SOURCE_DIR}/thirdparty/GLES_SDK/include)

if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(ARCH_SUFFIX "-64")
else()
    set(ARCH_SUFFIX "")
endif()
find_library(GLES2_LIBRARY NAMES GLESv2 PATHS ${CMAKE_SOURCE_DIR}/thirdparty/GLES_SDK/x86${ARCH_SUFFIX})

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(GLES2 DEFAULT_MSG GLES2_LIBRARY GLES2_INCLUDE_DIR)