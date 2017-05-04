###############################################################################
# libKD
# zlib/libpng License
###############################################################################
# Copyright (c) 2014-2017 Kevin Schmidt
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
#           or try NVIDIA Nsight Tegra Visual Studio Edition
# Emscripten: $EMSCRIPTEN/cmake/Modules/Platform/Emscripten.cmake
###############################################################################

cmake_minimum_required(VERSION 3.1)
cmake_policy(SET CMP0025 NEW)
cmake_policy(SET CMP0054 NEW)

include(CheckIncludeFiles)
include(CheckLibraryExists)
list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_LIST_DIR})

option(KD_BUILD_EXAMPLES "Build with examples" Off)
option(KD_BUILD_TESTS "Build with tests" On)
set(KD_BUILD_STDLIB "default" CACHE STRING "Build with libc (default/musl/none)")
set_property(CACHE KD_BUILD_STDLIB PROPERTY STRINGS "default" "musl" "none")
mark_as_advanced(KD_BUILD_STDLIB)

project (KD C)
    include(CPack)
    include(CTest)

    set(KD_SOURCE "${CMAKE_CURRENT_SOURCE_DIR}/source/kd.c")
    if(EMSCRIPTEN OR ANDROID OR KD_FREESTANDING)
        add_library(KD STATIC ${KD_SOURCE})
    else()
        add_library(KD SHARED ${KD_SOURCE})
    endif()
    install(TARGETS KD DESTINATION lib)

    if(DEFINED ENV{CI})
        if(DEFINED CI_FLAGS)
            set_target_properties(KD PROPERTIES COMPILE_FLAGS "${CI_FLAGS}")
            set_target_properties(KD PROPERTIES LINK_FLAGS "${CI_FLAGS}")
        endif()
    endif()

    set_target_properties(KD PROPERTIES C_STANDARD 11 C_EXTENSIONS "OFF")
    set_target_properties(KD PROPERTIES POSITION_INDEPENDENT_CODE "True")    

    target_include_directories(KD PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
    target_include_directories(KD PUBLIC ${CMAKE_BINARY_DIR}/include ${CMAKE_BINARY_DIR}/include/KD)

    if(MSVC)
        target_compile_options(KD PRIVATE /W4 /WX /wd4100 /wd4127)
    else()
        target_compile_options(KD PRIVATE -Wall -Wextra -Werror -Wshadow -pedantic)
        target_compile_options(KD PRIVATE -Wno-attributes -Wno-missing-field-initializers -Wno-unused-parameter -Wno-unused-function)
        target_compile_options(KD PRIVATE $<$<CONFIG:Debug>:-fno-omit-frame-pointer>)
        if(KD_BUILD_STDLIB MATCHES "musl")
            target_compile_options(KD PRIVATE -mmusl)
        elseif(KD_BUILD_STDLIB MATCHES "none")
            target_compile_options(KD PRIVATE -nostdlib -ffreestanding)
            # disable __chkstk_ms
            target_compile_options(KD PRIVATE -mno-stack-arg-probe)
            set(KD_FREESTANDING "On")
        endif()
        if(EMSCRIPTEN)
            set(EMCC_FLAGS "-s EMTERPRETIFY=1 -s EMTERPRETIFY_ASYNC=1")
            set_target_properties(KD PROPERTIES LINK_FLAGS ${EMCC_FLAGS})
        endif()
    endif()
    set(KD_WINDOW_SUPPORTED "On")
    if(ANDROID OR EMSCRIPTEN)
        #No window
    elseif(WIN32)
        set(KD_WINDOW_WIN32 "On")
    elseif(APPLE)
        set(KD_WINDOW_COCOA "On")
    else()
        set(KD_WINDOW_SUPPORTED "Off")
        find_package(X11)
        if(X11_FOUND)
            set(KD_WINDOW_X11 "On")
            target_link_libraries(KD PRIVATE ${X11_LIBRARIES})
            target_include_directories(KD PRIVATE ${X11_INCLUDE_DIR})
            set(KD_WINDOW_SUPPORTED "On")
        endif()
        find_package(Wayland)
        if(WAYLAND_FOUND)
            set(KD_WINDOW_WAYLAND "On")
            target_link_libraries(KD PRIVATE ${WAYLAND_LIBRARIES})
            target_include_directories(KD PRIVATE ${WAYLAND_INCLUDE_DIR})
            set(KD_WINDOW_SUPPORTED "On")
        endif()
        if(X11_FOUND OR WAYLAND_FOUND)
            find_package(EGL REQUIRED)
            target_link_libraries(KD PUBLIC ${EGL_LIBRARY})
            target_include_directories(KD PUBLIC ${EGL_INCLUDE_DIR})
        endif()
    endif()
    configure_file(${CMAKE_SOURCE_DIR}/include/KD/kdplatform.h.in ${CMAKE_BINARY_DIR}/include/KD/kdplatform.h)

    if(NOT EMSCRIPTEN AND NOT KD_BUILD_STDLIB MATCHES "none")
        find_package(Threads)
        target_link_libraries(KD PRIVATE ${CMAKE_THREAD_LIBS_INIT})
        target_link_libraries(KD PRIVATE ${CMAKE_DL_LIBS})
        if(CMAKE_SYSTEM_NAME STREQUAL "FreeBSD")
            target_link_libraries(KD PRIVATE stdthreads)
        endif()
    endif()

    if(ANDROID)
        target_link_libraries(KD PRIVATE android)
        target_link_libraries(KD PRIVATE log)
    endif()
   
    if(WIN32)
         target_compile_definitions(KD PRIVATE "_WIN32_WINNT=0x0600")
    endif()

    if(EMSCRIPTEN)
        set_target_properties(KD PROPERTIES SUFFIX ".bc")
    endif()

    find_program(CLANG_TIDY NAMES clang-tidy)
    if(CLANG_TIDY AND CMAKE_MAJOR_VERSION GREATER 2 AND CMAKE_MINOR_VERSION GREATER 5 AND NOT CMAKE_CROSSCOMPILING)
        set_target_properties(KD PROPERTIES C_CLANG_TIDY ${CLANG_TIDY})
    endif()
    find_program(CPPLINT NAMES cpplint)
    if(CPPLINT AND CMAKE_MAJOR_VERSION GREATER 2 AND CMAKE_MINOR_VERSION GREATER 7 AND NOT CMAKE_CROSSCOMPILING)
        set_target_properties(KD PROPERTIES C_CPPLINT ${CPPLINT})
    endif()
    mark_as_advanced(CLANG_TIDY CPPLINT)

    file(GLOB KD_HEADERS "${CMAKE_SOURCE_DIR}/include/KD/*.h")
    list(APPEND KD_HEADERS ${CMAKE_BINARY_DIR}/include/KD/kdplatform.h)
    install(FILES ${KD_HEADERS} DESTINATION include/KD)
    source_group("KD" FILES ${KD_HEADERS})

    # Tests
    if(KD_BUILD_TESTS)
        function(test_helper TEST_NAME)
            add_executable(${TEST_NAME} ${CMAKE_SOURCE_DIR}/test/${TEST_NAME}.c)
            if(DEFINED ENV{CI})
                if(DEFINED CI_FLAGS)
                    set_target_properties(${TEST_NAME} PROPERTIES COMPILE_FLAGS "${CI_FLAGS}")
                    set_target_properties(${TEST_NAME} PROPERTIES LINK_FLAGS "${CI_FLAGS}")
                endif()
            endif()
            if(ANDROID)
                target_link_libraries(${TEST_NAME} PRIVATE EGL)
            endif()
            target_link_libraries(${TEST_NAME} PRIVATE KD)
            set_target_properties(${TEST_NAME} PROPERTIES C_STANDARD 11 C_EXTENSIONS "OFF")
            set_target_properties(${TEST_NAME} PROPERTIES POSITION_INDEPENDENT_CODE "True")
            set_target_properties(${TEST_NAME} PROPERTIES ENABLE_EXPORTS "ON")
            if(MSVC)
                set_property(TARGET ${TEST_NAME} APPEND PROPERTY WINDOWS_EXPORT_ALL_SYMBOLS "ON")
            elseif(MINGW)
                set_target_properties(${TEST_NAME} PROPERTIES LINK_FLAGS "${CI_FLAGS} -Wl,--export-all-symbols")  
            elseif(EMSCRIPTEN)
                set_target_properties(${TEST_NAME} PROPERTIES LINK_FLAGS "${CI_FLAGS} ${EMCC_FLAGS}")
            elseif(CMAKE_C_COMPILER_ID STREQUAL "TinyCC")
                #TCC does not support -Wl,--export-dynamic
                set_target_properties(${TEST_NAME} PROPERTIES ENABLE_EXPORTS "OFF")    
                set_property(TARGET ${TEST_NAME} APPEND PROPERTY LINK_FLAGS "${CI_FLAGS} -rdynamic")
            endif()
            add_test(NAME ${TEST_NAME} COMMAND ${TEST_NAME})
            if(CMAKE_C_COMPILER_ID STREQUAL "TinyCC")
                set_tests_properties(${TEST_NAME} PROPERTIES ENVIRONMENT "LD_LIBRARY_PATH=.")
            endif()
        endfunction()
        file(GLOB TESTS test/*.c)
        foreach(TEST ${TESTS})
            get_filename_component(TEST ${TEST} NAME)
            string(REGEX REPLACE "\\.[^.]*$" "" TEST ${TEST})
            test_helper(${TEST})
        endforeach()
    endif()

    # Examples
    if(NOT DEFINED ENV{CI} AND KD_BUILD_EXAMPLES)
        find_package(EGL REQUIRED)
        find_package(GLES2 REQUIRED)
        function(example_helper EXAMPLE_NAME)
            add_executable(${EXAMPLE_NAME} ${CMAKE_SOURCE_DIR}/example/${EXAMPLE_NAME}.c)
            target_include_directories(${EXAMPLE_NAME} PRIVATE ${EGL_INCLUDE_DIR} ${GLES2_INCLUDE_DIR})
            target_link_libraries(${EXAMPLE_NAME} PRIVATE KD ${EGL_LIBRARY} ${GLES2_LIBRARY})
            set_target_properties(${EXAMPLE_NAME} PROPERTIES C_STANDARD 11 C_EXTENSIONS "OFF")
            set_target_properties(${EXAMPLE_NAME} PROPERTIES POSITION_INDEPENDENT_CODE "True")
            set_target_properties(${EXAMPLE_NAME} PROPERTIES ENABLE_EXPORTS "ON")
            if(MSVC)
                set_property(TARGET ${TEST_NAME} APPEND PROPERTY WINDOWS_EXPORT_ALL_SYMBOLS "ON")
            elseif(MINGW)
                set_target_properties(${EXAMPLE_NAME} PROPERTIES LINK_FLAGS "-Wl,--export-all-symbols")    
            elseif(EMSCRIPTEN)
                set_target_properties(${EXAMPLE_NAME} PROPERTIES LINK_FLAGS "${EMCC_FLAGS} --emrun --preload-file ${CMAKE_BINARY_DIR}/data@data")
                set_target_properties(${EXAMPLE_NAME} PROPERTIES SUFFIX ".html")
            elseif(CMAKE_C_COMPILER_ID STREQUAL "TinyCC")
                #TCC does not support -Wl,--export-dynamic
                set_target_properties(${EXAMPLE_NAME} PROPERTIES ENABLE_EXPORTS "OFF")    
                set_property(TARGET ${EXAMPLE_NAME} APPEND PROPERTY LINK_FLAGS "-rdynamic")
            endif()
        endfunction()
        file(GLOB DATA example/data/*.*)
        file(COPY ${DATA} DESTINATION ${CMAKE_BINARY_DIR}/data)
        file(GLOB EXAMPLES example/*.c)
        foreach(EXAMPLE ${EXAMPLES})
            get_filename_component(EXAMPLE ${EXAMPLE} NAME)
            string(REGEX REPLACE "\\.[^.]*$" "" EXAMPLE ${EXAMPLE})
            example_helper(${EXAMPLE})
        endforeach()
    endif()
