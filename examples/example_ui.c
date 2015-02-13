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
#include <EGL/eglext.h>
#include <GL/gl.h>

#include "thirdparty/glimgui.h"
#include "../distribution/KD/kd.h"

/* Example with ui*/
static KDboolean quit = 0;
int gui_layout = 0;
void draw_ui () {
    glimgui_prepare ();

    if (gui_layout == 0)
    {
        glimgui_label (99, "Main Menu", 20, 20, 1, 1);

        if (glimgui_button (1, "Some Button", 220, 100, 180, 50))
        {
            kdLogMessage("Some Button was pressed!\n");
        }

        if (glimgui_button (2, "Options", 220, 180, 180, 50))
        {
            glimgui_clear();
            gui_layout = 1;
            kdLogMessage("Switching to Options!\n");
        }

        if (glimgui_button (3, "Quit", 220, 380, 180, 50))
        {
            kdLogMessage("Quit!\n");
            quit = 1;
        }
    }
    else
    {
        glimgui_label (99, "Options", 20, 20, 1, 1);

        if (glimgui_button (1, "Some Other Button", 220, 100, 180, 50))
        {
            kdLogMessage("Some Other Button was pressed!\n");
        }

        glimgui_label (98, "Enter your name:", 150, 180, 1, 50);

        static char name[32] = { "IMGUI fan\0" };

        if (glimgui_lineedit (2, name, 16, 290, 195, -1, 20))
        {
            kdLogMessage("Changed name value: "); kdLogMessage(name); kdLogMessage("\n");
        }

        glimgui_label (97, "Enter your age:", 150, 210, 1, 50);

        static char age[32] = { "99\0" };

        if (glimgui_lineedit (3, age, 3, 290, 225, -1, 20))
        {
            kdLogMessage("Changed age value: "); kdLogMessage(age); kdLogMessage("\n");
        }

        if (glimgui_button (4, "Back", 220, 380, 180, 50))
        {
            kdLogMessage("Switching back!\n");
            glimgui_clear();
            gui_layout = 0;
        }
    }

    glimgui_finish ();
}

void callback_pointer(const KDEvent *event)
{
    switch(event->type)
    {
        case(KD_EVENT_INPUT_POINTER):
        {
            switch(event->data.inputpointer.index)
            {
                case(KD_INPUT_POINTER_SELECT):
                {
                    uistate.left_mousebutton_state = event->data.inputpointer.select;
                    uistate.mousepos_x = event->data.inputpointer.x;
                    uistate.mousepos_y = event->data.inputpointer.y;
                    break;
                }
                case(KD_INPUT_POINTER_X):
                {
                    uistate.mousepos_x = event->data.inputpointer.x;
                    break;
                }
                case(KD_INPUT_POINTER_Y):
                {
                    uistate.mousepos_y = event->data.inputpointer.y;
                    break;
                }
                default:
                {
                    break;
                }
            }
            break;
        }
        default:
        {
            kdDefaultEvent(event);
            break;
        }
    }
    return;
}

KDint kdMain(KDint argc, const KDchar *const *argv)
{
    kdLogMessage("Starting example\n");

    kdLogMessage("-----KD-----\n");
    kdLogMessage("Vendor: "); kdLogMessage(kdQueryAttribcv(KD_ATTRIB_VENDOR)); kdLogMessage("\n");
    kdLogMessage("Version: "); kdLogMessage(kdQueryAttribcv(KD_ATTRIB_VERSION)); kdLogMessage("\n");
    kdLogMessage("Platform: "); kdLogMessage(kdQueryAttribcv(KD_ATTRIB_PLATFORM)); kdLogMessage("\n");

    const EGLint egl_attributes[] =
            {
                    EGL_SURFACE_TYPE,                EGL_WINDOW_BIT,
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
                    EGL_CONTEXT_MAJOR_VERSION_KHR, 2,
                    EGL_CONTEXT_MINOR_VERSION_KHR, 1,
                    EGL_NONE,
            };

    EGLDisplay egl_display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    eglInitialize(egl_display, 0, 0);
    eglBindAPI(EGL_OPENGL_API);

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

    kdLogMessage("-----GL-----\n");
    kdLogMessage("Vendor: "); kdLogMessage((const char*)glGetString(GL_VENDOR)); kdLogMessage("\n");
    kdLogMessage("Version: "); kdLogMessage((const char*)glGetString(GL_VERSION)); kdLogMessage("\n");
    kdLogMessage("Renderer: "); kdLogMessage((const char*)glGetString(GL_RENDERER)); kdLogMessage("\n");
    kdLogMessage("Extensions: "); kdLogMessage((const char*)glGetString(GL_EXTENSIONS)); kdLogMessage("\n");

    kdInstallCallback(callback_pointer, KD_EVENT_INPUT_POINTER, KD_NULL);

    glClearColor (0.3, 0.3, 0.3, 1.);

    while(!quit)
    {
        const KDEvent *event = kdWaitEvent(-1);
        if(event)
        {
            switch(event->type)
            {
                case(KD_EVENT_WINDOW_REDRAW):
                {
                    KDint32 window_size[2];
                    kdGetWindowPropertyiv(kd_window, KD_WINDOWPROPERTY_SIZE, window_size);
                    glViewport (0, 0, window_size[0],window_size[1]);

                    glMatrixMode (GL_PROJECTION);
                    glLoadIdentity ();

                    glOrtho(0, window_size[0],window_size[1], 0, -1.0f, 1.0f);
                    glMatrixMode (GL_MODELVIEW);
                    glLoadIdentity();
                    break;
                }
                case(KD_EVENT_QUIT):
                {
                    quit = 1;
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
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
            draw_ui();
        }
    }
    eglDestroyContext(egl_display, egl_context);
    eglDestroySurface(egl_display, egl_surface);
    eglTerminate(egl_display);

    kdDestroyWindow(kd_window);

    return 0;
}
