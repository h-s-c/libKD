###############################################################################
 # Copyright (c) 2014 Kevin Schmidt
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
 ##############################################################################

cmake_minimum_required(VERSION 2.8)
include(CheckIncludeFiles)
include(CheckLibraryExists)

set(DIST_DIR distribution)
set(SOURCE_DIR sourcecode)
set(EXAMPLE_DIR examples)

project (KD)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c11 -Wall -Wextra -Wno-unused-parameter -Wno-unused-function -Wno-missing-field-initializers -Wno-format-security -Wfatal-errors -pedantic")

    if(COVERAGE)
        message(STATUS "Code coverage build.")
        if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
            set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fprofile-arcs -ftest-coverage")
            set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fprofile-arcs -ftest-coverage")
        endif()
    endif()

    set(KD_SOURCE ${SOURCE_DIR}/kd.c)
    list(APPEND KD_SOURCE ${KD_SOURCE})
    list(APPEND KD_INCLUDE_DIRS ${DIST_DIR})

    message(STATUS "Windowing systems compatibility checks")
    check_include_files("X11/Xlib.h;X11/Xutil.h" HAVE_X11)
    if(HAVE_X11)
        set(KD_WINDOW_SUPPORTED ON)
        set(KD_WINDOW_X11 ON)
        list(APPEND KD_LIBRARIES X11)
    endif()
    check_include_files("wayland-client.h;wayland-egl.h" HAVE_WAYLAND)
    if(HAVE_WAYLAND)
        set(KD_WINDOW_SUPPORTED ON)
        set(KD_WINDOW_WAYLAND ON)
        list(APPEND KD_LIBRARIES wayland-egl)
    endif()
    check_include_files("/opt/vc/include/bcm_host.h" HAVE_DISPMANX)
    if(HAVE_DISPMANX)
        set(KD_WINDOW_SUPPORTED ON)
        set(KD_WINDOW_DISPMANX ON)
        list(APPEND KD_LIBRARIES bcm_host)
        list(APPEND KD_INCLUDE_DIRS /opt/vc/include)
    endif()
    configure_file(${DIST_DIR}/KD/kdplatform.h.in ${CMAKE_SOURCE_DIR}/${DIST_DIR}/KD/kdplatform.h)

    message(STATUS "C11 compatibility checks")
    check_include_files(threads.h HAVE_THREADS_H)
    if(NOT HAVE_THREADS_H)
        list(APPEND KD_SOURCE ${SOURCE_DIR}/thirdparty/threads/threads.c)
        list(APPEND KD_INCLUDE_DIRS ${SOURCE_DIR}/thirdparty/threads)
    endif()

    message(STATUS "POSIX compatibility checks")
    check_include_files(mqueue.h HAVE_MQUEUE_H)
    if(NOT HAVE_MQUEUE_H)
        list(APPEND KD_SOURCE ${SOURCE_DIR}/thirdparty/mqueue/mqueue.c)
        list(APPEND KD_INCLUDE_DIRS ${SOURCE_DIR}/thirdparty/mqueue)
    endif()
    find_package (Threads)
    list(APPEND KD_LIBRARIES ${CMAKE_THREAD_LIBS_INIT})

    message(STATUS "Other compatibility checks")
    check_library_exists( c strlcat "" HAVE_BSD_LIBC)
    if(NOT HAVE_BSD_LIBC)
        check_include_files(bsd/string.h HAVE_BSD_STRINGS_H)
        if(NOT HAVE_BSD_STRINGS_H)
            list(APPEND KD_SOURCE ${SOURCE_DIR}/thirdparty/bsd/strlcat.c)
            list(APPEND KD_SOURCE ${SOURCE_DIR}/thirdparty/bsd/strlcpy.c)
            list(APPEND KD_INCLUDE_DIRS ${SOURCE_DIR}/thirdparty)
        else()
            list(APPEND KD_LIBRARIES bsd)
        endif()
    endif()

    list(APPEND KD_LIBRARIES ${CMAKE_DL_LIBS})
    list(APPEND KD_LIBRARIES rt)
    list(APPEND KD_LIBRARIES m)
    list(APPEND KD_LIBRARIES EGL)

    include_directories(${KD_INCLUDE_DIRS} )
    add_library (KD SHARED ${KD_SOURCE})
    target_link_libraries(KD ${KD_LIBRARIES})
    set_target_properties(KD PROPERTIES POSITION_INDEPENDENT_CODE TRUE)

project (KD_example_1)
    include_directories(${DIST_DIR})
    add_executable(KD_example_1 ${EXAMPLE_DIR}/kd_example_1.c)
    target_link_libraries(KD_example_1 KD EGL GLESv2)

project (KD_example_2)
    include_directories(${DIST_DIR})
    add_executable(KD_example_2 ${EXAMPLE_DIR}/kd_example_2.c)
    target_link_libraries(KD_example_2 KD EGL GLESv2)

project (KD_example_3)
    include_directories(${DIST_DIR})
    add_executable(KD_example_3 ${EXAMPLE_DIR}/kd_example_3.c)
    target_link_libraries(KD_example_3 KD EGL GLESv2)

project (KD_example_4)
    include_directories(${DIST_DIR})
    add_executable(KD_example_4 ${EXAMPLE_DIR}/kd_example_4.c)
    target_link_libraries(KD_example_4 KD EGL GLESv2)