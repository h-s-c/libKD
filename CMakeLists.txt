###############################################################################
# libKD
# zlib/libpng License
###############################################################################
# Copyright (c) 2014-2015 Kevin Schmidt
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

###############################################################################
# CMake toolchain file notes:
#
# Android:  https://github.com/taka-no-me/android-cmake
#			or try NVIDIA Nsight Tegra Visual Studio Edition
# Emscripten: $EMSCRIPTEN/cmake/Modules/platform/Emscripten.cmake
###############################################################################

cmake_minimum_required(VERSION 2.8.7)
if(CMAKE_MAJOR_VERSION GREATER 2)
    cmake_policy(SET CMP0037 OLD)
endif()
include(CheckIncludeFiles)
include(CheckLibraryExists)
list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_LIST_DIR})

set(DIST_DIR ${CMAKE_CURRENT_SOURCE_DIR}/distribution)
set(SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/sourcecode)
set(EXAMPLE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/examples)
set(TEST_DIR ${CMAKE_CURRENT_SOURCE_DIR}/tests)

set(KD_BUILD_EXAMPLES "Off" CACHE BOOL "Build with examples")
set(KD_BUILD_CI "Off" CACHE BOOL "Build with continous integration")
set(KD_BUILD_VFS "Off" CACHE BOOL "Build with virtual filesystem")

if(CMAKE_BUILD_TYPE MATCHES Debug)
    set(KD_BUILD_DEBUG "On")
else()
    set(KD_BUILD_DEBUG "Off")
endif()

# Nvidia Tegra Nsight doesnt set this
if(CMAKE_SYSTEM_NAME MATCHES Android)
    set(ANDROID "On")
else()
    set(ANDROID "Off")
endif()

add_custom_target(install)
add_custom_target(package)

project (KD)
    if(MSVC)
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /W3 /D_CRT_SECURE_NO_WARNINGS")
    else()
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c11 -Wall -Wextra -Wno-unused-parameter -Wno-unused-function -Wno-missing-field-initializers -Wno-format-security -Wfatal-errors -pedantic")
        if(NOT ANDROID)
            if(KD_BUILD_CI)
                set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} --coverage")
                set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} --coverage")
            endif()
        endif()
    endif()

    if(KD_BUILD_VFS)
        message(STATUS "Virtual filesystem enabled build")
        list(APPEND KD_LIBRARIES physfs)
        set(KD_VFS_SUPPORTED "On")
    endif()

    if(EMSCRIPTEN)
        message(STATUS "Emscripten build")
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-warn-absolute-paths -s ASYNCIFY=1")
    endif()

    set(KD_SOURCE ${SOURCE_DIR}/kd.c)
    list(APPEND KD_SOURCE ${KD_SOURCE})
    list(APPEND KD_INCLUDE_DIRS ${DIST_DIR})

    message(STATUS "API compatibility checks")

	set(KD_WINDOW_SUPPORTED "On")
    if(ANDROID)
        set(KD_WINDOW_ANDROID "On")
	elseif(MSVC OR MINGW)
		set(KD_WINDOW_WIN32 "On")
    else()
        check_include_files("X11/Xlib.h;X11/Xutil.h" KD_WINDOW_X11)
        if(NOT KD_WINDOW_X11)
			check_include_files("wayland-client.h;wayland-egl.h" KD_WINDOW_WAYLAND)
            if(NOT KD_WINDOW_WAYLAND)
                set(KD_WINDOW_SUPPORTED "Off")
            endif()
		else()
			list(APPEND KD_LIBRARIES X11)
        endif()
    endif()

	if(MSVC)
		check_include_files("thr/threads.h" KD_THREAD_C11)
	else()
		check_include_files("threads.h" KD_THREAD_C11)
	endif()
    if(NOT KD_THREAD_C11)
		check_include_files("pthread.h" KD_THREAD_POSIX)
		if(NOT KD_THREAD_POSIX)
			check_include_files("windows.h" KD_THREAD_WIN32)
			if(NOT KD_THREAD_WIN32)
				message(FATAL_ERROR "Platform not supported yet.")
			endif()
		endif()
    endif()

    check_include_files("stdatomic.h" KD_ATOMIC_C11)
    if(NOT KD_ATOMIC_C11)
        check_include_files("windows.h" KD_ATOMIC_WIN32)
		if(NOT KD_ATOMIC_WIN32)
			set(KD_ATOMIC_BUILTIN "On")
		endif()
    endif()

    check_include_files("dirent.h" HAVE_DIRENT)
    if(NOT HAVE_DIRENT)
		if(MSVC)
			list(APPEND KD_INCLUDE_DIRS ${SOURCE_DIR}/thirdparty/dirent)
		endif()
    endif()

    find_package(Threads)
    list(APPEND KD_LIBRARIES ${CMAKE_THREAD_LIBS_INIT})
    list(APPEND KD_LIBRARIES ${CMAKE_DL_LIBS})
    if(NOT MSVC)
        list(APPEND KD_LIBRARIES m)
    endif()

	find_package(KHR)
	list(APPEND KD_INCLUDE_DIRS ${KHR_INCLUDE_DIR})
	find_package(EGL)
	list(APPEND KD_LIBRARIES ${EGL_LIBRARY})
	list(APPEND KD_INCLUDE_DIRS ${EGL_INCLUDE_DIR})

    if(ANDROID)
        list(APPEND KD_LIBRARIES android)
        list(APPEND KD_LIBRARIES log)
    endif()

    configure_file(${DIST_DIR}/KD/kdplatform.h.in ${DIST_DIR}/KD/kdplatform.h)
    include_directories(${KD_INCLUDE_DIRS} )

	# Continous Integration
    if(KD_BUILD_CI)
        message(STATUS "Continous integration build")
        include(CTest)
        function(test_helper TEST_NAME)
            add_executable(${TEST_NAME} ${TEST_DIR}/${TEST_NAME}.c)
            target_link_libraries(${TEST_NAME} KD)
            add_test(${TEST_NAME} ${TEST_NAME})
        endfunction()
        test_helper(test_callback)
        test_helper(test_eventloop)
        test_helper(test_stacksize)
        test_helper(test_threadonce)
    endif()

	# Examples
	if(KD_BUILD_EXAMPLES)
	    function(example_helper EXAMPLE_NAME)
        add_executable(${EXAMPLE_NAME} ${EXAMPLE_DIR}/${EXAMPLE_NAME}.c)
        target_link_libraries(${EXAMPLE_NAME} KD)
        endfunction()
		find_package(GLES2)
		include_directories(${GLES2_INCLUDE_DIR})
		example_helper(example_overview)
		target_link_libraries(example_overview ${GLES2_LIBRARY})
	endif()

    if(MSVC OR MINGW)
        add_library(KD STATIC ${KD_SOURCE})
    else()
        add_library(KD SHARED ${KD_SOURCE})
    endif()
    target_link_libraries(KD ${KD_LIBRARIES})
    set_target_properties(KD PROPERTIES POSITION_INDEPENDENT_CODE TRUE)