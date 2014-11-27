#include <KD/kd.h>
#include <EGL/egl.h>
#include <GLES2/gl2.h>

/* Simple input example */
KDint kdMain(KDint argc, const KDchar *const *argv)
{
    kdLogMessage("Starting example 1\n");

    kdLogMessage("-----KD-----\n");
    kdLogMessage("Vendor: "); kdLogMessage(kdQueryAttribcv(KD_ATTRIB_VENDOR)); kdLogMessage("\n");
    kdLogMessage("Version: "); kdLogMessage(kdQueryAttribcv(KD_ATTRIB_VERSION)); kdLogMessage("\n");
    kdLogMessage("Platform: "); kdLogMessage(kdQueryAttribcv(KD_ATTRIB_PLATFORM)); kdLogMessage("\n");

    const EGLint egl_attributes[] =
    {
        EGL_SURFACE_TYPE,                EGL_WINDOW_BIT,
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

    const EGLint egl_attrib_list[] =
    { 
        EGL_COLORSPACE,     EGL_COLORSPACE_LINEAR,
        EGL_ALPHA_FORMAT,   EGL_ALPHA_FORMAT_NONPRE,
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

    EGLSurface egl_surface = eglCreateWindowSurface(egl_display, egl_config, egl_native_window, egl_attrib_list);  
    EGLContext egl_context = eglCreateContext(egl_display, egl_config, EGL_NO_CONTEXT, egl_context_attributes);

    eglMakeCurrent(egl_display, egl_surface, egl_surface, egl_context);

    kdLogMessage("-----GLES2-----\n");
    kdLogMessage("Vendor: "); kdLogMessage((const char*)glGetString(GL_VENDOR)); kdLogMessage("\n");
    kdLogMessage("Version: "); kdLogMessage((const char*)glGetString(GL_VERSION)); kdLogMessage("\n");
    kdLogMessage("Renderer: "); kdLogMessage((const char*)glGetString(GL_RENDERER)); kdLogMessage("\n");
    kdLogMessage("Extensions: "); kdLogMessage((const char*)glGetString(GL_EXTENSIONS)); kdLogMessage("\n");

    for(;;)
    {
        const KDEvent *event = kdWaitEvent(0);
        if(event)
        {
            if(event->type == KD_EVENT_WINDOW_CLOSE)
            {
                break;
            }
            kdDefaultEvent(event);
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
