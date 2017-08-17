/******************************************************************************
 * libKD
 * zlib/libpng License
 ******************************************************************************
 * Copyright (c) 2014-2017 Kevin Schmidt
 *
 * This software is provided 'as-is', without any express or implied
 * warranty. In no event will the authors be held liable for any damages
 * arising from the use of this software.
 *
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:
 *
 * 1. The origin of this software must not be misrepresented; you must not
 *    claim that you wrote the original software. If you use this software
 *    in a product, an acknowledgment in the product documentation would be
 *    appreciated but is not required.
 * 2. Altered source versions must be plainly marked as such, and must not be
 *    misrepresented as being the original software.
 * 3. This notice may not be removed or altered from any source distribution.
 ******************************************************************************/

#include <KD/kd.h>
#include <KD/kdext.h>
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>

/* Example with eventloop, timer and callback*/
static KDboolean quit = KD_FALSE;
static void KD_APIENTRY kd_callback(const KDEvent *event)
{
    switch(event->type)
    {
        case(KD_EVENT_WINDOW_CLOSE):
        {
            quit = KD_TRUE;
            break;
        }
        default:
        {
            kdDefaultEvent(event);
            break;
        }
    }
}

#if defined(GL_KHR_debug)
static void GL_APIENTRY gl_callback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar *message, const void *userParam)
{
    kdLogMessage(message);
}
#endif

KDint KD_APIENTRY kdMain(KDint argc, const KDchar *const *argv)
{
    const EGLint egl_attributes[] =
        {
            EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
            EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
            EGL_RED_SIZE, 8,
            EGL_GREEN_SIZE, 8,
            EGL_BLUE_SIZE, 8,
            EGL_ALPHA_SIZE, EGL_DONT_CARE,
            EGL_DEPTH_SIZE, EGL_DONT_CARE,
            EGL_STENCIL_SIZE, EGL_DONT_CARE,
            EGL_SAMPLE_BUFFERS, 0,
            EGL_NONE};

    const EGLint egl_context_attributes[] =
    {
        EGL_CONTEXT_CLIENT_VERSION,
        2,
#if defined(EGL_KHR_create_context)
        EGL_CONTEXT_FLAGS_KHR,
        EGL_CONTEXT_OPENGL_DEBUG_BIT_KHR,
#endif
        EGL_NONE,
    };

    EGLDisplay egl_display = eglGetDisplay(EGL_DEFAULT_DISPLAY);

    eglInitialize(egl_display, 0, 0);
    eglBindAPI(EGL_OPENGL_ES_API);

    EGLint egl_num_configs = 0;
    EGLConfig egl_config;
    eglChooseConfig(egl_display, egl_attributes, &egl_config, 1, &egl_num_configs);
    EGLContext egl_context = eglCreateContext(egl_display, egl_config, EGL_NO_CONTEXT, egl_context_attributes);

    KDWindow *kd_window = kdCreateWindow(egl_display, egl_config, KD_NULL);
    EGLNativeWindowType native_window;
    kdRealizeWindow(kd_window, &native_window);

    EGLSurface egl_surface = eglCreateWindowSurface(egl_display, egl_config, native_window, KD_NULL);

    eglMakeCurrent(egl_display, egl_surface, egl_surface, egl_context);

    if(eglGetError() != EGL_SUCCESS)
    {
        kdAssert(0);
    }

#if defined(GL_KHR_debug)
    if(kdStrstrVEN((const KDchar *)glGetString(GL_EXTENSIONS), "GL_KHR_debug"))
    {
        glEnable(GL_DEBUG_OUTPUT_KHR);
        glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS_KHR);
        PFNGLDEBUGMESSAGECALLBACKKHRPROC glDebugMessageCallback = (PFNGLDEBUGMESSAGECALLBACKKHRPROC)eglGetProcAddress("glDebugMessageCallbackKHR");
        glDebugMessageCallback(&gl_callback, KD_NULL);
    }
#endif

    /* Debug message */
    kdLogMessage("-----KD-----\n");
    kdLogMessagefKHR("Vendor: %s\n", kdQueryAttribcv(KD_ATTRIB_VENDOR));
    kdLogMessagefKHR("Version: %s\n", kdQueryAttribcv(KD_ATTRIB_VERSION));
    kdLogMessagefKHR("Platform: %s\n", kdQueryAttribcv(KD_ATTRIB_PLATFORM));
    kdLogMessage("-----EGL-----\n");
    kdLogMessagefKHR("Vendor: %s\n", eglQueryString(egl_display, EGL_VENDOR));
    kdLogMessagefKHR("Version: %s\n", eglQueryString(egl_display, EGL_VERSION));
    kdLogMessagefKHR("Client APIs: %s\n", eglQueryString(egl_display, EGL_CLIENT_APIS));
    kdLogMessagefKHR("Extensions: %s\n", eglQueryString(egl_display, EGL_EXTENSIONS));
    kdLogMessage("-----GLES2-----\n");
    kdLogMessagefKHR("Vendor: %s\n", (const KDchar *)glGetString(GL_VENDOR));
    kdLogMessagefKHR("Version: %s\n", (const KDchar *)glGetString(GL_VERSION));
    kdLogMessagefKHR("Renderer: %s\n", (const KDchar *)glGetString(GL_RENDERER));
    kdLogMessagefKHR("Extensions: %s\n", (const KDchar *)glGetString(GL_EXTENSIONS));

    kdInstallCallback(&kd_callback, KD_EVENT_WINDOW_CLOSE, KD_NULL);
    KDTimer *kd_timer = kdSetTimer(1000000000, KD_TIMER_PERIODIC_AVERAGE, KD_NULL);

    KDfloat32 r = 0.0f;
    KDfloat32 g = 1.0f;
    KDfloat32 b = 0.0f;

    while(!quit)
    {
        const KDEvent *event = kdWaitEvent(-1);
        if(event)
        {
            switch(event->type)
            {
                case(KD_EVENT_TIMER):
                {
                    KDuint8 buffer[3] = {0};
                    kdCryptoRandom(buffer, 3);
                    r = buffer[0] % 256 / 255.0f;
                    g = buffer[1] % 256 / 255.0f;
                    b = buffer[2] % 256 / 255.0f;
                    break;
                }
                default:
                {
                    kdDefaultEvent(event);
                }
            }
        }

        if(eglSwapBuffers(egl_display, egl_surface) == EGL_FALSE)
        {
            EGLint egl_error = eglGetError();
            switch(egl_error)
            {
                case(EGL_BAD_SURFACE):
                {
                    eglMakeCurrent(egl_display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
                    eglDestroySurface(egl_display, egl_surface);
                    kdRealizeWindow(kd_window, &native_window);
                    egl_surface = eglCreateWindowSurface(egl_display, egl_config, native_window, KD_NULL);
                    eglMakeCurrent(egl_display, egl_surface, egl_surface, egl_context);
                    break;
                }
                case(EGL_BAD_MATCH):
                case(EGL_BAD_CONTEXT):
                case(EGL_CONTEXT_LOST):
                {
                    eglMakeCurrent(egl_display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
                    eglDestroyContext(egl_display, egl_context);
                    egl_context = eglCreateContext(egl_display, egl_config, EGL_NO_CONTEXT, egl_context_attributes);
                    eglMakeCurrent(egl_display, egl_surface, egl_surface, egl_context);
                    break;
                }
                case(EGL_BAD_DISPLAY):
                case(EGL_NOT_INITIALIZED):
                case(EGL_BAD_ALLOC):
                {
                    kdAssert(0);
                    break;
                }
                default:
                {
                    kdAssert(0);
                    break;
                }
            }
        }
        else
        {
            glClearColor(r, g, b, 1.0);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
        }
    }

    eglDestroyContext(egl_display, egl_context);
    eglDestroySurface(egl_display, egl_surface);
    eglTerminate(egl_display);

    kdCancelTimer(kd_timer);
    kdDestroyWindow(kd_window);

    return 0;
}
