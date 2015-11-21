if(MSVC OR MINGW)
    set(GLES_SDK_INCLUDE_PATH ${CMAKE_SOURCE_DIR}/thirdparty/gles_amd/include)
endif()

find_path(KHR_INCLUDE_DIR NAMES KHR/khrplatform.h PATHS ${GLES_SDK_INCLUDE_PATH})

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(KHR DEFAULT_MSG KHR_INCLUDE_DIR)