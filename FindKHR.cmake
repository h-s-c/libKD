find_path(KHR_INCLUDE_DIR NAMES KHR/khrplatform.h PATHS $ENV{KHRONOS_HEADERS}
                                                        ${CMAKE_SOURCE_DIR}/thirdparty/gles_amd/include
                                                        ${CMAKE_SOURCE_DIR}/thirdparty/gles_mali/include)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(KHR DEFAULT_MSG KHR_INCLUDE_DIR)