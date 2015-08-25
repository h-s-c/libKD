if(MSVC)
    set(KHR_INCLUDE_DIR ${CMAKE_BINARY_DIR}/packages/ANGLE.WindowsStore/Include)
else()
    find_path(KHR_INCLUDE_DIR NAMES KHR/khrplatform.h)
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(KHR DEFAULT_MSG KHR_INCLUDE_DIR)