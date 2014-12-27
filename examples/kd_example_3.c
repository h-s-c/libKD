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

/* Simple event with callback example (KD_EVENT_WINDOW_CLOSE is triggered by pressing Escape)*/
static KDboolean quit = 0;
void callback(const KDEvent *event)
{
    if(event->type == KD_EVENT_WINDOW_CLOSE)
    {
        quit = 1;
    }
    kdDefaultEvent(event);
}

KDint kdMain(KDint argc, const KDchar *const *argv)
{
    kdLogMessage("Starting example 3\n");

    kdLogMessage("-----KD-----\n");
    kdLogMessage("Vendor: "); kdLogMessage(kdQueryAttribcv(KD_ATTRIB_VENDOR)); kdLogMessage("\n");
    kdLogMessage("Version: "); kdLogMessage(kdQueryAttribcv(KD_ATTRIB_VERSION)); kdLogMessage("\n");
    kdLogMessage("Platform: "); kdLogMessage(kdQueryAttribcv(KD_ATTRIB_PLATFORM)); kdLogMessage("\n");

    const EGLint egl_attributes[] =
    {
        EGL_SURFACE_TYPE,                EGL_SWAP_BEHAVIOR_PRESERVED_BIT,
        EGL_RENDERABLE_TYPE,             EGL_OPENGL_ES2_BIT,
        EGL_RED_SIZE,                    8,
        EGL_GREEN_SIZE,                  8,
        EGL_BLUE_SIZE,                   8,
        EGL_ALPHA_SIZE,                  8,
        EGL_DEPTH_SIZE,                  24,
        EGL_STENCIL_SIZE,                8,
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

    eglSurfaceAttrib(egl_display, egl_surface, EGL_SWAP_BEHAVIOR, EGL_BUFFER_PRESERVED);
    eglSwapInterval(egl_display, 1);

    kdLogMessage("-----GLES2-----\n");
    kdLogMessage("Vendor: "); kdLogMessage((const char*)glGetString(GL_VENDOR)); kdLogMessage("\n");
    kdLogMessage("Version: "); kdLogMessage((const char*)glGetString(GL_VERSION)); kdLogMessage("\n");
    kdLogMessage("Renderer: "); kdLogMessage((const char*)glGetString(GL_RENDERER)); kdLogMessage("\n");
    kdLogMessage("Extensions: "); kdLogMessage((const char*)glGetString(GL_EXTENSIONS)); kdLogMessage("\n");

    kdInstallCallback(callback, KD_EVENT_WINDOW_CLOSE, KD_NULL);

    for(;;)
    {
        kdPumpEvents();
        if(quit)
        {
            break;
        }

        glClearColor(0, 1, 0, 0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
        eglSwapBuffers(egl_display, egl_surface);
    }
    eglDestroyContext(egl_display, egl_context);
    eglDestroySurface(egl_display, egl_surface);
    eglTerminate(egl_display);
    kdDestroyWindow(kd_window);

    return 0;
}
