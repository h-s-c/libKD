/******************************************************************************
 * This is free and unencumbered software released into the public domain.
 * 
 * Anyone is free to copy, modify, publish, use, compile, sell, or
 * distribute this software, either in source code form or as a compiled
 * binary, for any purpose, commercial or non-commercial, and by any
 * means.

 * In jurisdictions that recognize copyright laws, the author or authors
 * of this software dedicate any and all copyright interest in the
 * software to the public domain. We make this dedication for the benefit
 * of the public at large and to the detriment of our heirs and
 * successors. We intend this dedication to be an overt act of
 * relinquishment in perpetuity of all present and future rights to this
 * software under copyright law.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 * 
 * For more information, please refer to <http://unlicense.org/>
 ******************************************************************************/

#include <KD/kd.h>
#include <EGL/egl.h>
#include <GLES2/gl2.h>

/* Example with eventloop, timer and callback*/
static KDboolean quit = 0;
void KD_APIENTRY callback(const KDEvent *event)
{
    switch(event->type)
    {
        case(KD_EVENT_QUIT):
        {
            quit = 1;
            break;
        }
        default:
        {
            kdDefaultEvent(event);
            break;
        }
    }
}
KDint KD_APIENTRY kdMain(KDint argc, const KDchar *const *argv)
{
    kdLogMessage("Starting example\n");

    kdLogMessage("-----KD-----\n");
    kdLogMessage("Vendor: "); kdLogMessage(kdQueryAttribcv(KD_ATTRIB_VENDOR)); kdLogMessage("\n");
    kdLogMessage("Version: "); kdLogMessage(kdQueryAttribcv(KD_ATTRIB_VERSION)); kdLogMessage("\n");
    kdLogMessage("Platform: "); kdLogMessage(kdQueryAttribcv(KD_ATTRIB_PLATFORM)); kdLogMessage("\n");

    const EGLint egl_attributes[] =
    {
        EGL_SURFACE_TYPE,       EGL_WINDOW_BIT,
        EGL_RENDERABLE_TYPE,    EGL_OPENGL_ES2_BIT,
        EGL_RED_SIZE,           8,
        EGL_GREEN_SIZE,         8,
        EGL_BLUE_SIZE,          8,
        EGL_ALPHA_SIZE,         8,
        EGL_DEPTH_SIZE,         24,
        EGL_STENCIL_SIZE,       8,
        EGL_NONE
    };

    const EGLint egl_context_attributes[] =
    {
        EGL_CONTEXT_CLIENT_VERSION, 2,
        EGL_NONE,
    };

    EGLDisplay egl_display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    eglInitialize(egl_display, 0, 0);
    eglBindAPI(EGL_OPENGL_ES_API);

    kdLogMessage("-----EGL-----\n");
    kdLogMessage("Vendor: "); kdLogMessage(eglQueryString(egl_display, EGL_VENDOR)); kdLogMessage("\n");
    kdLogMessage("Version: "); kdLogMessage(eglQueryString(egl_display, EGL_VERSION)); kdLogMessage("\n");
    kdLogMessage("Client APIs: "); kdLogMessage(eglQueryString(egl_display, EGL_CLIENT_APIS)); kdLogMessage("\n");
    kdLogMessage("Extensions: "); kdLogMessage(eglQueryString(egl_display, EGL_EXTENSIONS)); kdLogMessage("\n");

    EGLint egl_num_configs = 0;
    EGLConfig egl_config;
    eglChooseConfig(egl_display, egl_attributes, &egl_config, 1, &egl_num_configs);

    KDWindow *kd_window = kdCreateWindow(egl_display, egl_config, KD_NULL);
    EGLNativeWindowType egl_native_window;
    kdRealizeWindow(kd_window, &egl_native_window);
    EGLSurface egl_surface = eglCreateWindowSurface(egl_display, egl_config, egl_native_window, KD_NULL);
    EGLContext egl_context = eglCreateContext(egl_display, egl_config, EGL_NO_CONTEXT, egl_context_attributes);
    eglMakeCurrent(egl_display, egl_surface, egl_surface, egl_context);

    kdLogMessage("-----GLES2-----\n");
    kdLogMessage("Vendor: "); kdLogMessage((const char*)glGetString(GL_VENDOR)); kdLogMessage("\n");
    kdLogMessage("Version: "); kdLogMessage((const char*)glGetString(GL_VERSION)); kdLogMessage("\n");
    kdLogMessage("Renderer: "); kdLogMessage((const char*)glGetString(GL_RENDERER)); kdLogMessage("\n");
    kdLogMessage("Extensions: "); kdLogMessage((const char*)glGetString(GL_EXTENSIONS)); kdLogMessage("\n");

    kdInstallCallback(&callback, KD_EVENT_QUIT, KD_NULL);
    KDTimer* kd_timer = kdSetTimer(1000000000, KD_TIMER_PERIODIC_AVERAGE, KD_NULL);

    float r = 0.0f;
    float g = 1.0f;
    float b = 0.0f;

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
                    r = buffer[0]%256 / 255.0f;
                    g = buffer[1]%256 / 255.0f;
                    b = buffer[2]%256 / 255.0f;
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
                    kdRealizeWindow(kd_window, &egl_native_window);
                    egl_surface = eglCreateWindowSurface(egl_display, egl_config, egl_native_window, KD_NULL);
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
