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
set(TEST_DIR tests)

project (KD)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c11 -Wall -Wextra -Wno-unused-parameter -Wno-unused-function -Wno-missing-field-initializers -Wno-format-security -Wfatal-errors -pedantic")

    if(WANT_COVERAGE)
        message(STATUS "Code coverage build")
        if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
            set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fprofile-arcs -ftest-coverage")
            set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fprofile-arcs -ftest-coverage")
        endif()
    endif()

    if(WANT_GC)
        message(STATUS "Garbage collected build")
        list(APPEND KD_LIBRARIES gc)
        set(KD_GC ON)
    endif()

    if(EMSCRIPTEN)
        message(STATUS "Emscripten build")
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-warn-absolute-paths -s ASYNCIFY=1")
        set(KD_NO_STATIC_DATA ON)
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

    message(STATUS "C11 compatibility checks")
    check_include_files(threads.h HAVE_THREADS_H)
    if(NOT HAVE_THREADS_H)
        if(NOT EMSCRIPTEN)
            list(APPEND KD_SOURCE ${SOURCE_DIR}/thirdparty/threads/threads.c)
            list(APPEND KD_INCLUDE_DIRS ${SOURCE_DIR}/thirdparty/threads)
        endif()
    endif()

    message(STATUS "POSIX compatibility checks")
    check_include_files(mqueue.h HAVE_MQUEUE_H)
    if(NOT HAVE_MQUEUE_H)
        list(APPEND KD_SOURCE ${SOURCE_DIR}/thirdparty/mqueue/mqueue.c)
        list(APPEND KD_INCLUDE_DIRS ${SOURCE_DIR}/thirdparty/mqueue)
    endif()

    if(NOT EMSCRIPTEN)
        find_package (Threads)
        list(APPEND KD_LIBRARIES ${CMAKE_THREAD_LIBS_INIT})
    endif()

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

    configure_file(${DIST_DIR}/KD/kdplatform.h.in ${CMAKE_SOURCE_DIR}/${DIST_DIR}/KD/kdplatform.h)
    include_directories(${KD_INCLUDE_DIRS} )
    add_library (KD SHARED ${KD_SOURCE})
    target_link_libraries(KD ${KD_LIBRARIES})
    set_target_properties(KD PROPERTIES POSITION_INDEPENDENT_CODE TRUE)

# Tests
project (test_stacksize)
    include_directories(${DIST_DIR})
    add_executable(test_stacksize ${TEST_DIR}/test_stacksize.c)
    target_link_libraries(test_stacksize KD)
    enable_testing()
    add_test(test_stacksize test_stacksize)

# Examples
project (example_event)
    include_directories(${DIST_DIR})
    add_executable(example_event ${EXAMPLE_DIR}/example_event.c)
    target_link_libraries(example_event KD EGL GLESv2)

project (example_event_callback)
    include_directories(${DIST_DIR})
    add_executable(example_event_callback ${EXAMPLE_DIR}/example_event_callback.c)
    target_link_libraries(example_event_callback KD EGL GLESv2)

project (example_event_timer)
    include_directories(${DIST_DIR})
    add_executable(example_event_timer ${EXAMPLE_DIR}/example_event_timer.c)
    target_link_libraries(example_event_timer KD EGL GLESv2)