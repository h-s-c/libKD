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

cmake_minimum_required(VERSION 3.1)
cmake_policy(SET CMP0025 NEW)

include(CheckIncludeFiles)
include(CheckLibraryExists)
list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_LIST_DIR})

set(KD_BUILD_EXAMPLES "On" CACHE BOOL "Build with examples")
set(KD_BUILD_TESTS "On" CACHE BOOL "Build with tests")
set(KD_BUILD_VFS "Off" CACHE BOOL "Build with virtual filesystem")

if(DEFINED ENV{CI})
    set(KD_BUILD_CI "On")
endif()

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(KD_BUILD_DEBUG "On")
endif()

# Nvidia Tegra Nsight doesnt set this
if(CMAKE_SYSTEM_NAME STREQUAL "Android")
    set(ANDROID "On")
endif()

project (KD C)
    set(KD_GCC_FLAGS "-Wall -Wextra -Werror -Wshadow")
    set(KD_MSVC_FLAGS "/W4 /WX /wd4100")
    set(KD_EMCC_FLAGS "-s EMTERPRETIFY_ASYNC=1 -s USE_PTHREADS=1")

    if(CMAKE_C_COMPILER_ID STREQUAL "GNU")
        if(CMAKE_C_COMPILER_VERSION VERSION_LESS 4.7)
            set(KD_GCC_FLAGS "${KD_GCC_FLAGS} -std=c1x -Wno-missing-field-initializers")
        else()
            set(KD_GCC_FLAGS "${KD_GCC_FLAGS} -std=c11")
        endif()

        if(CMAKE_C_COMPILER_VERSION VERSION_LESS 4.8)
            set(KD_GCC_FLAGS "${KD_GCC_FLAGS} -pedantic -Wno-attributes")
        else()
            set(KD_GCC_FLAGS "${KD_GCC_FLAGS} -Wpedantic")
        endif()
    elseif(CMAKE_C_COMPILER_ID STREQUAL "Clang" OR CMAKE_C_COMPILER_ID STREQUAL "AppleClang")
        set(KD_GCC_FLAGS "${KD_GCC_FLAGS} -std=c11 -Wno-missing-field-initializers")
    elseif(CMAKE_C_COMPILER_ID STREQUAL "Intel")
        set(KD_MSVC_FLAGS "${KD_MSVC_FLAGS} /Qstd=c11")
        set(KD_GCC_FLAGS "${KD_GCC_FLAGS} -std=c11")
    endif()

    if(KD_BUILD_CI)
        set(KD_GCC_FLAGS "${KD_GCC_FLAGS} ${KD_BUILD_CI_FLAGS}")
    endif()

    if(KD_BUILD_VFS)
        list(APPEND KD_LIBRARIES physfs)
        set(KD_VFS_SUPPORTED "On")
    endif()

    set(KD_SOURCE ${CMAKE_CURRENT_SOURCE_DIR}/source/kd.c)
    list(APPEND KD_SOURCE ${KD_SOURCE})
    list(APPEND KD_INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/include)

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
    if(NOT KD_ATOMIC_C11 OR CMAKE_C_COMPILER_ID STREQUAL "Intel")
		set(KD_ATOMIC_C11 "Off")
        check_include_files("windows.h" KD_ATOMIC_WIN32)
		if(NOT KD_ATOMIC_WIN32)
            if(CMAKE_C_COMPILER_ID STREQUAL "GNU" AND CMAKE_C_COMPILER_VERSION VERSION_LESS 4.7)
                set(KD_ATOMIC_LEGACY "On")
			else()
                set(KD_ATOMIC_BUILTIN "On")
            endif()
		endif()
    endif()

    if(MSVC)
        check_include_files("dirent.h" HAVE_DIRENT)
        if(NOT HAVE_DIRENT)
            list(APPEND KD_INCLUDE_DIRS ${CMAKE_SOURCE_DIR}/thirdparty/dirent/include)
        endif()

        check_include_files("inttypes.h" HAVE_INTTYPES)
        if(NOT HAVE_INTTYPES)
            list(APPEND KD_INCLUDE_DIRS ${CMAKE_SOURCE_DIR}/thirdparty/inttypes/include)
        endif()
    endif()

    find_package(Threads)
    list(APPEND KD_LIBRARIES ${CMAKE_THREAD_LIBS_INIT})
    list(APPEND KD_LIBRARIES ${CMAKE_DL_LIBS})
    if(NOT MSVC)
        list(APPEND KD_LIBRARIES m)
    endif()

    if(ANDROID)
        list(APPEND KD_LIBRARIES android)
        list(APPEND KD_LIBRARIES log)
    endif()

    list(APPEND KD_INCLUDE_DIRS ${CMAKE_BINARY_DIR}/include ${CMAKE_BINARY_DIR}/include/KD)
    configure_file(${CMAKE_SOURCE_DIR}/include/KD/kdplatform.h.in ${CMAKE_BINARY_DIR}/include/KD/kdplatform.h)
    include_directories(${KD_INCLUDE_DIRS})

	# Tests
    if(KD_BUILD_TESTS)
        include(CTest)
        function(test_helper TEST_NAME)
            add_executable(${TEST_NAME} ${CMAKE_SOURCE_DIR}/test/${TEST_NAME}.c)
            target_link_libraries(${TEST_NAME} KD)
            set_target_properties(${TEST_NAME} PROPERTIES POSITION_INDEPENDENT_CODE "True")
            if(MSVC)
                set_target_properties(${TEST_NAME} PROPERTIES COMPILE_FLAGS "${KD_MSVC_FLAGS} /wd4718")
            else()
                set_target_properties(${TEST_NAME} PROPERTIES COMPILE_FLAGS "${KD_GCC_FLAGS} -Wno-unused-parameter" LINK_FLAGS ${KD_GCC_FLAGS})
            endif()
            if(EMSCRIPTEN)
                set_target_properties(${TEST_NAME} PROPERTIES SUFFIX ".html" COMPILE_FLAGS "${KD_GCC_FLAGS} ${KD_EMCC_FLAGS}" LINK_FLAGS "${KD_EMCC_FLAGS} --emrun")
            endif()
            add_test(${TEST_NAME} ${TEST_NAME})
        endfunction()
        test_helper(test_callback)
        test_helper(test_eventloop)
        test_helper(test_threadonce)
        if(NOT KD_BUILD_CI)
            #AddressSanitizer uses too much stack memory
            test_helper(test_stacksize)
        endif()
    endif()

	# Examples
    if(NOT KD_BUILD_CI AND KD_BUILD_EXAMPLES)
        function(example_helper EXAMPLE_NAME)
            find_package(EGL)
            find_package(GLES2)
            include_directories(${EGL_INCLUDE_DIR} ${GLES2_INCLUDE_DIR})
            add_executable(${EXAMPLE_NAME} ${CMAKE_SOURCE_DIR}/example/${EXAMPLE_NAME}.c)
            target_link_libraries(${EXAMPLE_NAME} KD ${EGL_LIBRARY} ${GLES2_LIBRARY})
            set_target_properties(${EXAMPLE_NAME} PROPERTIES POSITION_INDEPENDENT_CODE "True")
            if(MSVC)
                set_target_properties(${EXAMPLE_NAME} PROPERTIES COMPILE_FLAGS ${KD_MSVC_FLAGS})
            else()
                set_target_properties(${EXAMPLE_NAME} PROPERTIES COMPILE_FLAGS "${KD_GCC_FLAGS} -Wno-unused-parameter" LINK_FLAGS ${KD_GCC_FLAGS})
            endif()
            if(EMSCRIPTEN)
                set_target_properties(${EXAMPLE_NAME} PROPERTIES SUFFIX ".html" COMPILE_FLAGS "${KD_GCC_FLAGS} ${KD_EMCC_FLAGS}" LINK_FLAGS "${KD_EMCC_FLAGS} --emrun")
            endif()
        endfunction()
        example_helper(example_overview)
        #example_helper(example_ui)
    endif()

    if(MSVC OR MINGW)
        add_definitions(-DKD_EXPORTS)
        add_library(KD STATIC ${KD_SOURCE})
    else()
        add_library(KD SHARED ${KD_SOURCE})
    endif()
    target_link_libraries(KD ${KD_LIBRARIES})
    set_target_properties(KD PROPERTIES POSITION_INDEPENDENT_CODE "True")
    if(MSVC)
        set_target_properties(KD PROPERTIES COMPILE_FLAGS ${KD_MSVC_FLAGS})
    else()
        set_target_properties(KD PROPERTIES COMPILE_FLAGS ${KD_GCC_FLAGS} LINK_FLAGS ${KD_GCC_FLAGS})
    endif()
    if(EMSCRIPTEN)
        set_target_properties(KD PROPERTIES SUFFIX ".bc" COMPILE_FLAGS "${KD_GCC_FLAGS} ${KD_EMCC_FLAGS}" LINK_FLAGS ${KD_EMCC_FLAGS})
    endif()
