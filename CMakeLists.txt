###############################################################################
# libKD
# zlib/libpng License
###############################################################################
# Copyright (c) 2014-2021 Kevin Schmidt
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
# Emscripten: $EMSCRIPTEN/cmake/Modules/Platform/Emscripten.cmake
###############################################################################

cmake_minimum_required(VERSION 3.13)
cmake_policy(SET CMP0025 NEW)
cmake_policy(SET CMP0054 NEW)
cmake_policy(SET CMP0069 NEW)
cmake_policy(SET CMP0072 NEW)

include(CheckIncludeFiles)
include(CheckLibraryExists)
include(CheckIPOSupported)
list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_LIST_DIR})

option(KD_BUILD_EXAMPLES "Build with examples" Off)
option(KD_BUILD_TESTS "Build with tests" On)
option(KD_BUILD_OPTIMIZATONS "Build with optimizations (Haswell or later required)" Off)
option(KD_BUILD_VERBOSE "Build with further checks" Off)
option(KD_BUILD_LIBDECOR "Build with wayland client-side decorations" Off)

project (KD C)
    include(CPack)
    include(CTest)

    file(GLOB KD_SOURCES "${CMAKE_SOURCE_DIR}/source/*.c")
    list(REMOVE_ITEM KD_SOURCES "${CMAKE_SOURCE_DIR}/source/xdg-shell.c")
    list(REMOVE_ITEM KD_SOURCES "${CMAKE_SOURCE_DIR}/source/miniz.c")
    if(ANDROID OR EMSCRIPTEN)
        list(REMOVE_ITEM KD_SOURCES "${CMAKE_SOURCE_DIR}/source/rpmalloc.c")
    endif()
    if(EMSCRIPTEN OR ANDROID OR MINGW)
        add_library(KD STATIC ${KD_SOURCES})
    else()
        add_library(KD SHARED ${KD_SOURCES})
    endif()
    install(TARGETS KD DESTINATION lib)

    if(DEFINED ENV{CI})
        if(DEFINED CI_FLAGS)
            set_target_properties(KD PROPERTIES COMPILE_FLAGS "${CI_FLAGS}")
            set_target_properties(KD PROPERTIES LINK_FLAGS "${CI_FLAGS}")
        endif()
    endif()

    set_target_properties(KD PROPERTIES C_STANDARD 11 C_EXTENSIONS "OFF")
    if(NOT EMSCRIPTEN)
        set_target_properties(KD PROPERTIES POSITION_INDEPENDENT_CODE "True")
    endif()
    if(NOT ANDROID AND NOT EMSCRIPTEN AND NOT APPLE AND NOT CMAKE_C_COMPILER_ID STREQUAL "Intel")
        set_target_properties(KD PROPERTIES LINK_WHAT_YOU_USE "True")
    endif()

    if(KD_BUILD_OPTIMIZATONS AND NOT MSVC)
        check_ipo_supported(RESULT IPO)
        if(IPO)
            set_target_properties(KD PROPERTIES INTERPROCEDURAL_OPTIMIZATION "ON")
        endif()
    endif()

    target_include_directories(KD PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
    target_include_directories(KD PUBLIC ${CMAKE_BINARY_DIR}/include ${CMAKE_BINARY_DIR}/include/KD)

    if(MSVC)
        target_compile_options(KD PRIVATE /W3 /WX /wd4100 /wd4127 /wd4204 /wd5105)
        target_compile_options(KD PRIVATE /Od /GS- /Zl /Gs9999999)
        set_target_properties(KD PROPERTIES LINK_FLAGS "/NODEFAULTLIB /SUBSYSTEM:CONSOLE /STACK:0x100000,0x100000")
        string(REGEX REPLACE "/RTC(su|[1su])" "" CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG}")
        set(KD_FREESTANDING "On")
        if(KD_BUILD_OPTIMIZATONS)
            target_compile_definitions(KD PRIVATE __SSE__=1 __SSE2__=1 __SSE4_1__=1 __SSE4_2__=1)
        endif()
    else()
        target_compile_options(KD PRIVATE -Wall -Wextra -Werror -Wshadow -pedantic)
        target_compile_options(KD PRIVATE -Wno-unused-function)
        target_compile_options(KD PRIVATE $<$<CONFIG:Debug>:-fno-omit-frame-pointer>)
        if(CMAKE_C_COMPILER_ID STREQUAL "Clang")
            target_compile_options(KD PRIVATE -Weverything)
            target_compile_options(KD PRIVATE -Wno-used-but-marked-unused -Wno-declaration-after-statement -Wno-unknown-warning-option -Wno-unsafe-buffer-usage -Wno-cast-qual)
        endif()
        if(CMAKE_C_COMPILER_ID STREQUAL "Intel")
            target_compile_options(KD PRIVATE -fp-model precise -D_Float128=__float128 )
        endif()
        if(MINGW)
            target_compile_options(KD PRIVATE -nostdlib -ffreestanding)
            target_compile_options(KD PRIVATE -mno-stack-arg-probe)
            set(KD_FREESTANDING "On")
        endif()
        if(EMSCRIPTEN)
            target_compile_options(KD PRIVATE -pthread -Wno-unreachable-code -Wno-reserved-identifier -Wno-strict-prototypes -Wno-unsafe-buffer-usage -Wno-switch-default -Wno-cast-qual)
            target_link_options(KD PRIVATE "SHELL:-s WASM=1" "SHELL:-s ASYNCIFY=1" "SHELL:-s ALLOW_MEMORY_GROWTH=1" "SHELL:-s USE_PTHREADS=1" "SHELL:-s PTHREAD_POOL_SIZE=20")
        endif()
        if(KD_BUILD_OPTIMIZATONS)
            if(CMAKE_SYSTEM_NAME STREQUAL "Android")
                target_compile_options(KD PRIVATE -mfpu=neon)
            else()
                target_compile_options(KD PRIVATE -msse -msse2 -msse4.1 -msse4.2 -mbmi -mtune=haswell)
            endif()
        endif()
    endif()

    set(KD_WINDOW_SUPPORTED "On")
    if(ANDROID)
        set(KD_WINDOW_ANDROID "On")
    elseif(EMSCRIPTEN)
        set(KD_WINDOW_EMSCRIPTEN "On")
    elseif(WIN32)
        set(KD_WINDOW_WIN32 "On")
    elseif(APPLE)
        set(KD_WINDOW_COCOA "On")
    else()
        set(KD_WINDOW_SUPPORTED "Off")
        find_package(XORG)
        if(XORG_FOUND)
            set(KD_WINDOW_X11 "On")
            target_link_libraries(KD PRIVATE ${XORG_LIBRARIES})
            target_include_directories(KD PRIVATE ${XORG_INCLUDE_DIR})
            set(KD_WINDOW_SUPPORTED "On")
        endif()
        find_package(WAYLAND)
        if(WAYLAND_FOUND)
            set(KD_WINDOW_WAYLAND "On")
            target_link_libraries(KD PRIVATE ${WAYLAND_LIBRARIES})
            target_include_directories(KD PRIVATE ${WAYLAND_INCLUDE_DIR})
            target_sources(KD PRIVATE "${CMAKE_SOURCE_DIR}/source/xdg-shell.c")
            if(CMAKE_C_COMPILER_ID STREQUAL "Clang")
                target_compile_options(KD PRIVATE -Wno-documentation-unknown-command -Wno-cast-qual -Wno-missing-variable-declarations)
            endif()
            set(KD_WINDOW_SUPPORTED "On")
            if(KD_BUILD_LIBDECOR)
                find_path(LIBDECOR_INCLUDE_DIR NAMES libdecor.h PATHS /usr/include/libdecor-0)
                find_library(LIBDECOR_LIBRARY NAMES decor-0 PATHS /usr/lib/)
                mark_as_advanced(LIBDECOR_INCLUDE_DIR LIBDECOR_LIBRARY)
                if(LIBDECOR_INCLUDE_DIR AND LIBDECOR_LIBRARY)
                    set(KD_WINDOW_WAYLAND_DECOR "On")
                    target_link_libraries(KD PRIVATE ${LIBDECOR_LIBRARY})
                    target_include_directories(KD PRIVATE ${LIBDECOR_INCLUDE_DIR})
                endif()
            endif()
        endif()
    endif()
    if(KD_WINDOW_SUPPORTED)
        find_package(EGL)
        if(EGL_FOUND)
            target_include_directories(KD PUBLIC ${EGL_INCLUDE_DIR})
        else()
            set(KD_WINDOW_SUPPORTED "Off")
        endif()
    endif()

    configure_file(${CMAKE_SOURCE_DIR}/include/KD/kdplatform.h.in ${CMAKE_BINARY_DIR}/include/KD/kdplatform.h)

    if(NOT EMSCRIPTEN AND NOT KD_FREESTANDING)
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
        target_link_libraries(KD PRIVATE EGL)

        add_library(miniz STATIC ${CMAKE_SOURCE_DIR}/source/miniz.c)
        target_include_directories(miniz PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
        target_include_directories(miniz PUBLIC ${CMAKE_BINARY_DIR}/include ${CMAKE_BINARY_DIR}/include/KD)

        target_link_libraries(KD PRIVATE miniz)
    endif()

    if(WIN32)
        target_link_libraries(KD PRIVATE ws2_32)
    endif()

    if(EMSCRIPTEN)
        set_target_properties(KD PROPERTIES SUFFIX ".a")
    endif()


    if(KD_BUILD_VERBOSE AND NOT MINGW AND NOT EMSCRIPTEN AND NOT ANDROID AND NOT CMAKE_C_COMPILER_ID STREQUAL "Intel")
        find_program(CLANG_TIDY NAMES clang-tidy)
        if(CLANG_TIDY AND NOT DEFINED ENV{CI} AND NOT CMAKE_CROSSCOMPILING AND NOT KD_BUILD_OPTIMIZATONS)
            message(STATUS "clang-tidy found: ${CLANG_TIDY}")
            set_target_properties(KD PROPERTIES C_CLANG_TIDY ${CLANG_TIDY})
        endif()
        # pip install --user cpplint
        find_program(CPPLINT NAMES cpplint PATHS ~/.local/bin)
        if(CPPLINT)
            message(STATUS "cpplint found: ${CPPLINT}")
            set_target_properties(KD PROPERTIES C_CPPLINT ${CPPLINT})
        endif()
        find_program(CPPCHECK NAMES cppcheck)
        if(CPPCHECK)
            message(STATUS "cppcheck found: ${CPPCHECK}")
            set(CPPCHECK "${CPPCHECK};--enable=warning,performance,portability;--inline-suppr")
            if(UNIX OR MINGW)
                set(CPPCHECK "${CPPCHECK};--template=[{file}:{line}}: ({severity}/{id}) {message}")
            endif()
            set_target_properties(KD PROPERTIES C_CPPCHECK "${CPPCHECK}")
        endif()
        find_program(INCLUDE_WHAT_YOU_USE NAMES include-what-you-use iwyu PATHS ~/.local/bin)
        if(INCLUDE_WHAT_YOU_USE)
            message(STATUS "include-what-you-use found: ${INCLUDE_WHAT_YOU_USE}")
            set_target_properties(KD PROPERTIES C_INCLUDE_WHAT_YOU_USE ${INCLUDE_WHAT_YOU_USE})
        endif()
        find_program(PVS_STUDIO NAMES pvs-studio-analyzer)
        if(PVS_STUDIO)
            message(STATUS "pvs-studio-analyzer found: ${PVS_STUDIO}")
            include(PVS-Studio.cmake)
            pvs_studio_add_target(TARGET ALL OUTPUT FORMAT errorfile
                LOG "${CMAKE_BINARY_DIR}/pvs-report.log" ANALYZE KD )
        endif()
        find_program(FLAWFINDER NAMES flawfinder)
        if(FLAWFINDER)
            message(STATUS "flawfinder found: ${FLAWFINDER}")
            include(Flawfinder.cmake)
            flawfinder_add_target(KD "${CMAKE_SOURCE_DIR}/source")
        endif()
        mark_as_advanced(CLANG_TIDY CPPLINT CPPCHECK INCLUDE_WHAT_YOU_USE PVS_STUDIO FLAWFINDER)
    endif()

    file(GLOB KD_HEADERS "${CMAKE_SOURCE_DIR}/include/KD/*.h")
    list(APPEND KD_HEADERS ${CMAKE_BINARY_DIR}/include/KD/kdplatform.h)
    install(FILES ${KD_HEADERS} DESTINATION include/KD)

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
            target_link_libraries(${TEST_NAME} PRIVATE KD)
            set_target_properties(${TEST_NAME} PROPERTIES C_STANDARD 11 C_EXTENSIONS "OFF")
            set_target_properties(${TEST_NAME} PROPERTIES POSITION_INDEPENDENT_CODE "True")
            set_target_properties(${TEST_NAME} PROPERTIES ENABLE_EXPORTS "ON")
            if(MSVC)
                set_property(TARGET ${TEST_NAME} APPEND PROPERTY WINDOWS_EXPORT_ALL_SYMBOLS "ON")
            elseif(MINGW)
                set_target_properties(${TEST_NAME} PROPERTIES LINK_FLAGS "${CI_FLAGS} -Wl,--export-all-symbols -static")
            elseif(EMSCRIPTEN)
                target_compile_options(${TEST_NAME} PRIVATE -pthread)
                set_target_properties(${TEST_NAME} PROPERTIES LINK_FLAGS "${CI_FLAGS} -s WASM=1 -s ASYNCIFY=1 -s ALLOW_MEMORY_GROWTH=1 -s USE_PTHREADS=1 -s PTHREAD_POOL_SIZE=20 --emrun --embed-file ${CMAKE_BINARY_DIR}/data@data")
            endif()
            if(ANDROID)
                add_test(NAME ${TEST_NAME} COMMAND sh -c "adb push ${TEST_NAME} /data/local/tmp && adb shell /data/local/tmp/${TEST_NAME}")
            else()
                add_test(NAME ${TEST_NAME} COMMAND ${TEST_NAME})
            endif()
        endfunction()
        file(GLOB DATA ${CMAKE_SOURCE_DIR}/test/data)
        if(MSVC)
            file(COPY ${DATA} DESTINATION ${CMAKE_BINARY_DIR}/Debug)
            file(COPY ${DATA} DESTINATION ${CMAKE_BINARY_DIR}/Release)
            file(COPY ${DATA} DESTINATION ${CMAKE_BINARY_DIR}/MinSizeRel)
            file(COPY ${DATA} DESTINATION ${CMAKE_BINARY_DIR}/RelWithDebInfo)
        elseif(ANDROID)
            execute_process(COMMAND sh -c "adb push ${CMAKE_SOURCE_DIR}/test/data /data/local/tmp")
        else()
            file(COPY ${DATA} DESTINATION ${CMAKE_BINARY_DIR})
        endif()
        file(GLOB TESTS test/*.c)
        foreach(TEST ${TESTS})
            get_filename_component(TEST ${TEST} NAME)
            string(REGEX REPLACE "\\.[^.]*$" "" TEST ${TEST})
            test_helper(${TEST})
        endforeach()
    endif()

    # Examples
    if(KD_BUILD_EXAMPLES)
        add_library(stb_vorbis STATIC ${CMAKE_SOURCE_DIR}/example/stb_vorbis.c)
        if(NOT WIN32)
            target_link_libraries(stb_vorbis PRIVATE m) 
        endif()
        target_compile_definitions(stb_vorbis PRIVATE "_CRT_SECURE_NO_WARNINGS") 
        if(EMSCRIPTEN)
            set_target_properties(stb_vorbis PROPERTIES SUFFIX ".a")
        endif()     
        find_package(EGL REQUIRED)
        find_package(GLES2 REQUIRED)
        if(NOT ANDROID)
            find_package(OpenAL REQUIRED)
        endif()
        function(example_helper EXAMPLE_NAME)
            if(ANDROID)
                add_library(${EXAMPLE_NAME} SHARED ${CMAKE_SOURCE_DIR}/example/${EXAMPLE_NAME}.c)
            else()
                add_executable(${EXAMPLE_NAME} ${CMAKE_SOURCE_DIR}/example/${EXAMPLE_NAME}.c)
            endif()
            target_include_directories(${EXAMPLE_NAME} PRIVATE ${EGL_INCLUDE_DIR} ${GLES2_INCLUDE_DIR})
            if(NOT ANDROID)
                target_include_directories(${EXAMPLE_NAME} PRIVATE ${OPENAL_INCLUDE_DIR})
            endif()
            target_link_libraries(${EXAMPLE_NAME} PRIVATE KD stb_vorbis ${EGL_LIBRARY} ${GLES2_LIBRARY})
            if(NOT ANDROID AND NOT EMSCRIPTEN)
                target_link_libraries(${EXAMPLE_NAME} PRIVATE ${OPENAL_LIBRARY})
            endif()
            set_target_properties(${EXAMPLE_NAME} PROPERTIES C_STANDARD 11 C_EXTENSIONS "OFF")
            set_target_properties(${EXAMPLE_NAME} PROPERTIES POSITION_INDEPENDENT_CODE "True")
            set_target_properties(${EXAMPLE_NAME} PROPERTIES ENABLE_EXPORTS "ON")
            if(MSVC)
                set_property(TARGET ${EXAMPLE_NAME} APPEND PROPERTY WINDOWS_EXPORT_ALL_SYMBOLS "ON")
            elseif(MINGW)
                target_compile_options(${EXAMPLE_NAME} PRIVATE -Wno-pointer-to-int-cast)
                target_link_options(${EXAMPLE_NAME} PRIVATE -Wl,--export-all-symbols -static)
            elseif(EMSCRIPTEN)
                target_compile_options(${EXAMPLE_NAME} PRIVATE -pthread)
                target_link_options(${EXAMPLE_NAME} PRIVATE "SHELL:-s WASM=1" "SHELL:-s ASYNCIFY=1" "SHELL:-s ALLOW_MEMORY_GROWTH=1" "SHELL:-s USE_PTHREADS=1" "SHELL:-s PTHREAD_POOL_SIZE=20")
                target_link_options(${EXAMPLE_NAME} PRIVATE --emrun --shell-file ${CMAKE_SOURCE_DIR}/example/shell.html --preload-file ${CMAKE_BINARY_DIR}/data@data)
                set_target_properties(${EXAMPLE_NAME} PROPERTIES SUFFIX ".html")
            endif()
        endfunction()
        file(GLOB DATA example/data)
        if(MSVC)
            file(COPY ${DATA} DESTINATION ${CMAKE_BINARY_DIR}/Debug)
            file(COPY ${DATA} DESTINATION ${CMAKE_BINARY_DIR}/Release)
            file(COPY ${DATA} DESTINATION ${CMAKE_BINARY_DIR}/MinSizeRel)
            file(COPY ${DATA} DESTINATION ${CMAKE_BINARY_DIR}/RelWithDebInfo)
        endif()
        file(COPY ${DATA} DESTINATION ${CMAKE_BINARY_DIR})
        file(GLOB EXAMPLES example/example_*.c)
        foreach(EXAMPLE ${EXAMPLES})
            get_filename_component(EXAMPLE ${EXAMPLE} NAME)
            string(REGEX REPLACE "\\.[^.]*$" "" EXAMPLE ${EXAMPLE})
            example_helper(${EXAMPLE})
        endforeach()
    endif()
