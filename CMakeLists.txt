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
# Android: https://github.com/taka-no-me/android-cmake
# Emscripten: $EMSCRIPTEN/cmake/Modules/platform/Emscripten.cmake
###############################################################################

cmake_minimum_required(VERSION 2.8.8)
if(CMAKE_MAJOR_VERSION GREATER 2)
    cmake_policy(SET CMP0037 OLD)
endif()
include(CheckIncludeFiles)
include(CheckLibraryExists)

set(DIST_DIR ${CMAKE_CURRENT_SOURCE_DIR}/distribution)
set(SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/sourcecode)
set(EXAMPLE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/examples)
set(TEST_DIR ${CMAKE_CURRENT_SOURCE_DIR}/tests)

set(KD_BUILD_EXAMPLES "On" CACHE BOOL "Build with examples")
set(KD_BUILD_TESTS "Off" CACHE BOOL "Build with tests")
set(KD_BUILD_COVERAGE "Off" CACHE BOOL "Build with code coverage")
set(KD_BUILD_GC "Off" CACHE BOOL "Build with gargabe collection")
set(KD_BUILD_VFS "Off" CACHE BOOL "Build with virtual filesystem")


if(CMAKE_BUILD_TYPE MATCHES Debug)
    set(KD_BUILD_DEBUG "On")
else()
    set(KD_BUILD_DEBUG "Off")
endif()

add_custom_target(install)
add_custom_target(package)

project (KD)
    if(${CMAKE_C_COMPILER_ID} STREQUAL "MSVC")
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /W4")
    else()
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c11 -Wall -Wextra -Wno-unused-parameter -Wno-unused-function -Wno-missing-field-initializers -Wno-format-security -Wfatal-errors -pedantic")
        if(NOT ANDROID)
            set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -mcx16")
        endif()
    endif()

    if(KD_BUILD_COVERAGE)
        message(STATUS "Code coverage build")
        if ("${CMAKE_C_COMPILER_ID}" STREQUAL "GNU")
            set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fprofile-arcs -ftest-coverage")
            set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fprofile-arcs -ftest-coverage")
        endif()
    endif()
    if(KD_BUILD_GC)
        message(STATUS "Garbage collected build")
        list(APPEND KD_LIBRARIES gc)
        set(KD_GC_SUPPORTED ON)
    endif()
    if(KD_BUILD_VFS)
        message(STATUS "Virtual filesystem enabled build")
        list(APPEND KD_LIBRARIES physfs)
        set(KD_VFS_SUPPORTED ON)
    endif()

    if(EMSCRIPTEN)
        message(STATUS "Emscripten build")
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-warn-absolute-paths -s ASYNCIFY=1")
    endif()

    set(KD_SOURCE ${SOURCE_DIR}/kd.c)
    list(APPEND KD_SOURCE ${KD_SOURCE})
    list(APPEND KD_INCLUDE_DIRS ${DIST_DIR})

    message(STATUS "Windowing systems compatibility checks")
    if(ANDROID)
        set(KD_WINDOW_SUPPORTED ON)
    else()
        check_include_files("X11/Xlib.h;X11/Xutil.h" KD_WINDOW_SUPPORTED)
        if(KD_WINDOW_SUPPORTED)
            set(KD_WINDOW_SUPPORTED ON)
            list(APPEND KD_LIBRARIES X11)
            list(APPEND KD_LIBRARIES EGL)
        endif()
    endif()

    message(STATUS "C11 compatibility checks")
    check_include_files(stdatomic.h HAVE_C11_ATOMICS)
    if(NOT HAVE_C11_ATOMICS)
        list(APPEND KD_INCLUDE_DIRS ${SOURCE_DIR}/thirdparty/stdatomic)
    endif()
    check_library_exists( c thrd_create "" HAVE_C11_THREADS)
    if(NOT HAVE_C11_THREADS)
        list(APPEND KD_SOURCE ${SOURCE_DIR}/thirdparty/threads/threads.c)
        list(APPEND KD_INCLUDE_DIRS ${SOURCE_DIR}/thirdparty/threads)
    endif()
    check_library_exists( c strncat_s "" HAVE_C11_ANNEX_K)
    if(NOT HAVE_C11_ANNEX_K)
        check_library_exists( c strlcat "" HAVE_BSD_STRINGS)
        if(NOT HAVE_BSD_STRINGS)
            check_library_exists( bsd strlcat "" HAVE_LIBBSD)
            if(NOT HAVE_LIBBSD)
                list(APPEND KD_SOURCE ${SOURCE_DIR}/thirdparty/bsd/strlcat.c)
                list(APPEND KD_SOURCE ${SOURCE_DIR}/thirdparty/bsd/strlcpy.c)
            else()
                list(APPEND KD_LIBRARIES bsd)
            endif()
        endif()
    endif()

    find_package (Threads REQUIRED)
    list(APPEND KD_LIBRARIES ${CMAKE_THREAD_LIBS_INIT})
    list(APPEND KD_LIBRARIES ${CMAKE_DL_LIBS})
    list(APPEND KD_LIBRARIES m)

    if(ANDROID)
        list(APPEND KD_LIBRARIES android)
        list(APPEND KD_LIBRARIES log)
    endif()

    configure_file(${DIST_DIR}/KD/kdplatform.h.in ${DIST_DIR}/KD/kdplatform.h)
    include_directories(${KD_INCLUDE_DIRS} )
    if(ANDROID)
        add_library (KD_object OBJECT ${KD_SOURCE})
    else()
        add_library (KD SHARED ${KD_SOURCE})
        add_library (KD_static STATIC ${KD_SOURCE})
        target_link_libraries(KD ${KD_LIBRARIES})
        set_target_properties(KD PROPERTIES POSITION_INDEPENDENT_CODE TRUE)
    endif()

# Examples
if(KD_BUILD_EXAMPLES)
    function(example_helper)
        include_directories(${DIST_DIR})
        if(ANDROID)
            set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}/libs/${ANDROID_NDK_ABI_NAME})
            add_library(${PROJECT_NAME} SHARED ${EXAMPLE_DIR}/${PROJECT_NAME}.c $<TARGET_OBJECTS:KD_object>)
            target_link_libraries(${PROJECT_NAME} EGL GLESv2 android log m)
            if(KD_BUILD_DEBUG)
                set(MANIFEST_DEBUG_FLAG "true")
            else()
                set(MANIFEST_DEBUG_FLAG "false")
            endif()
            file( WRITE ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}/AndroidManifest.xml
"<?xml version=\"1.0\" encoding=\"utf-8\"?>
<!-- BEGIN_INCLUDE(manifest) -->
<manifest xmlns:android=\"http://schemas.android.com/apk/res/android\"
        package=\"kd.${PROJECT_NAME}\"
        android:versionCode=\"1\"
        android:versionName=\"1.0\">
    <uses-sdk android:minSdkVersion=\"9\" />
    <uses-feature android:glEsVersion=\"0x00020000\"></uses-feature>
    <application android:label=\"${PROJECT_NAME}\" android:hasCode=\"false\" android:debuggable=\"${MANIFEST_DEBUG_FLAG}\">
        <activity android:name=\"android.app.NativeActivity\"
                android:label=\"${PROJECT_NAME}\"
                android:configChanges=\"orientation|keyboardHidden|screenSize\"
                android:theme=\"@android:style/Theme.NoTitleBar.Fullscreen\" >
            <meta-data android:name=\"android.app.lib_name\"
                    android:value=\"${PROJECT_NAME}\" />
            <intent-filter>
                <action android:name=\"android.intent.action.MAIN\" />
                <category android:name=\"android.intent.category.LAUNCHER\" />
            </intent-filter>
        </activity>
    </application>
</manifest>
<!-- END_INCLUDE(manifest) -->")
            execute_process( COMMAND android update project --name ${PROJECT_NAME} --path . --target android-19 --subprojects WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME})
            if(KD_BUILD_DEBUG)
                set(APK_NAME ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}/bin/${PROJECT_NAME}-debug.apk)
                file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}/jni/Android.mk "APP_ABI := ${ANDROID_NDK_ABI_NAME}\n")
                file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}/jni/Application.mk "APP_ABI := ${ANDROID_NDK_ABI_NAME}\n")
                file(COPY ${ANDROID_NDK}/prebuilt/android-${ANDROID_ARCH_NAME}/gdbserver/gdbserver DESTINATION ${CMAKE_LIBRARY_OUTPUT_DIRECTORY})
                file(WRITE ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/gdb.setup "set solib-search-path ./libs/${ANDROID_NDK_ABI_NAME}\n")
                file(APPEND ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/gdb.setup "directory ${EXAMPLE_DIR} ${SOURCE_DIR}\n")
                add_custom_command(OUTPUT ${APK_NAME} COMMAND ant debug DEPENDS ${PROJECT_NAME} WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME})
            else()
                set(APK_NAME ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}/bin/${PROJECT_NAME}-release-unsigned.apk)
                add_custom_command(OUTPUT ${APK_NAME} COMMAND ant release DEPENDS ${PROJECT_NAME} WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME})
            endif()
            add_custom_target( ${PROJECT_NAME}-package DEPENDS ${APK_NAME})
            add_custom_target( ${PROJECT_NAME}-install COMMAND adb install -r ${APK_NAME} DEPENDS ${APK_NAME})
            add_dependencies(package ${PROJECT_NAME}-package)
            add_dependencies(install ${PROJECT_NAME}-install)
        else()
            add_executable(${PROJECT_NAME} ${EXAMPLE_DIR}/${PROJECT_NAME}.c)
            target_link_libraries(${PROJECT_NAME} KD EGL GLESv2)
        endif()
    endfunction()

    project (example_overview)
        example_helper()
endif()

# Tests
if(KD_BUILD_TESTS)
    enable_testing()
    function(test_helper)
        include_directories(${DIST_DIR})
        add_executable(${PROJECT_NAME} ${TEST_DIR}/${PROJECT_NAME}.c)
        target_link_libraries(${PROJECT_NAME} KD)
        add_test(${PROJECT_NAME} ${PROJECT_NAME})
        set_tests_properties(${PROJECT_NAME} PROPERTIES TIMEOUT 5)
    endfunction()


    project (test_callback)
        test_helper()

    project (test_eventloop)
        test_helper()

    project (test_stacksize)
        test_helper()

    project (test_threadonce)
        test_helper()
endif()