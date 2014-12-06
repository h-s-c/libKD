/******************************************************************************
 * Copyright (c) 2014 Kevin Schmidt
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
