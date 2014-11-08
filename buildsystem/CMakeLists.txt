cmake_minimum_required(VERSION 3.0)
include(CheckIncludeFiles)
list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_LIST_DIR})

project (KD)
    set(DIST_DIR ${CMAKE_CURRENT_LIST_DIR}/../distribution)
    set(SRC_DIR ${CMAKE_CURRENT_LIST_DIR}/../sourcecode)

    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c11 -Wall -Wextra -Wno-unused-parameter -Wno-missing-field-initializers -Wfatal-errors -pedantic")

    find_package(EGL REQUIRED)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -I${EGL_INCLUDE_DIR}")
    find_package(X11 REQUIRED)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -I${X11_INCLUDE_DIR}")
    set(KD_WINDOW_SUPPORTED ${X11_FOUND})
    configure_file(${DIST_DIR}/KD/kdplatform.h.in ${DIST_DIR}/KD/kdplatform.h)

    set(KD_SOURCE  ${SRC_DIR}/kd.c)
    list(APPEND KD_SOURCE ${KD_SOURCE})
    list(APPEND KD_INCLUDE_DIRS ${DIST_DIR})

    check_include_files(threads.h HAVE_THREADS_H)
    if(NOT HAVE_THREADS_H)
        list(APPEND KD_SOURCE ${SRC_DIR}/thirdparty/threads/threads.c)
        list(APPEND KD_INCLUDE_DIRS ${SRC_DIR}/thirdparty/threads)
    endif()
    check_include_files(mqueue.h HAVE_MQUEUE_H)
    if(NOT HAVE_MQUEUE_H)
        list(APPEND KD_SOURCE ${SRC_DIR}/thirdparty/mqueue/mqeue.c)
        list(APPEND KD_INCLUDE_DIRS ${SRC_DIR}/thirdparty/mqueue)
    endif()

    include_directories(${KD_INCLUDE_DIRS} )
    add_library (KD STATIC ${KD_SOURCE})