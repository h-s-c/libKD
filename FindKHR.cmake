find_path(KHR_INCLUDE_DIR NAMES KHR/khrplatform.h PATHS ${CMAKE_SOURCE_DIR}/thirdparty/GLES_SDK/include)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(KHR DEFAULT_MSG KHR_INCLUDE_DIR)