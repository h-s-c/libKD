//
// Book:      OpenGL(R) ES 2.0 Programming Guide
// Authors:   Aaftab Munshi, Dan Ginsburg, Dave Shreiner
// ISBN-10:   0321502795
// ISBN-13:   9780321502797
// Publisher: Addison-Wesley Professional
// URLs:      http://safari.informit.com/9780321563835
//            http://www.opengles-book.com
//

//    This is a simple example that draws a single triangle with
//    a minimal vertex/fragment shader using OpenKODE.
//
#include <KD/kd.h>
#include <EGL/egl.h>
#include <GLES2/gl2.h>

#define EXAMPLE_COMMON_IMPLEMENTATION
#include "example_common.h"

typedef struct
{
    // Handle to a program object
    GLuint programObject;

    // EGL handles
    EGLDisplay eglDisplay;
    EGLContext eglContext;
    EGLSurface eglSurface;

    //KD handles
    KDWindow *window;
} UserData;


///
// Initialize the shader and program object
//
KDboolean Init(UserData *userData)
{
    const KDchar *vShaderStr =
        "attribute vec4 vPosition;    \n"
        "void main()                  \n"
        "{                            \n"
        "   gl_Position = vPosition;  \n"
        "}                            \n";

    const KDchar *fShaderStr =
        "precision mediump float;\n"
        "void main()                                  \n"
        "{                                            \n"
        "  gl_FragColor = vec4 ( 1.0, 0.0, 0.0, 1.0 );\n"
        "}                                            \n";

    // Store the program object
    userData->programObject = exampleCreateProgram(vShaderStr, fShaderStr);

    // Bind vPosition to attribute 0
    glBindAttribLocation(userData->programObject, 0, "vPosition");

    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    return KD_TRUE;
}

///
// Draw a triangle using the shader pair created in Init()
//
void Draw(UserData *userData)
{
    GLfloat vVertices[] = {
        0.0f, 0.5f, 0.0f,
        -0.5f, -0.5f, 0.0f,
        0.5f, -0.5f, 0.0f};

    // No clientside arrays, so do this in a webgl-friendly manner
    GLuint vertexPosObject;
    glGenBuffers(1, &vertexPosObject);
    glBindBuffer(GL_ARRAY_BUFFER, vertexPosObject);
    glBufferData(GL_ARRAY_BUFFER, 9 * 4, vVertices, GL_STATIC_DRAW);

    // Set the viewport
    EGLint width = 0; 
    EGLint height = 0;
    eglQuerySurface(userData->eglDisplay, userData->eglSurface, EGL_WIDTH, &width);
    eglQuerySurface(userData->eglDisplay, userData->eglSurface, EGL_HEIGHT, &height);
    glViewport(0, 0, width, height);

    // Clear the color buffer
    glClear(GL_COLOR_BUFFER_BIT);

    // Use the program object
    glUseProgram(userData->programObject);

    // Load the vertex data
    glBindBuffer(GL_ARRAY_BUFFER, vertexPosObject);
    glVertexAttribPointer(0, 3, GL_FLOAT, 0, 0, 0);
    glEnableVertexAttribArray(0);

    glDrawArrays(GL_TRIANGLES, 0, 3);

    eglSwapBuffers(userData->eglDisplay, userData->eglSurface);
}

///
// InitEGLContext()
//
//    Initialize an EGL rendering context and all associated elements
//
EGLBoolean InitEGLContext(UserData *userData,
    EGLConfig config)
{
    EGLContext context;
    EGLSurface surface;
    EGLint contextAttribs[] = {EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE, EGL_NONE};

    // Get native window handle
    EGLNativeWindowType hWnd;
    if(kdRealizeWindow(userData->window, &hWnd) != 0)
    {
        return EGL_FALSE;
    }
    surface = eglCreateWindowSurface(userData->eglDisplay, config, hWnd, KD_NULL);
    if(surface == EGL_NO_SURFACE)
    {
        return EGL_FALSE;
    }

    // Create a GL context
    context = eglCreateContext(userData->eglDisplay, config, EGL_NO_CONTEXT, contextAttribs);
    if(context == EGL_NO_CONTEXT)
    {
        return EGL_FALSE;
    }

    // Make the context current
    if(!eglMakeCurrent(userData->eglDisplay, surface, surface, context))
    {
        return EGL_FALSE;
    }

    userData->eglContext = context;
    userData->eglSurface = surface;

    return EGL_TRUE;
}

///
// kdMain()
//
//    Main function for OpenKODE application
//
KDint KD_APIENTRY kdMain(KDint argc, const KDchar *const *argv)
{
    EGLint attribList[] =
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

    EGLint majorVersion, minorVersion;
    UserData userData;
    EGLint numConfigs;
    EGLConfig config;

    userData.eglDisplay = eglGetDisplay(kdGetPlatformDisplayVEN());

    // Initialize EGL
    if(!eglInitialize(userData.eglDisplay, &majorVersion, &minorVersion))
    {
        return EGL_FALSE;
    }

    // Get configs
    if(!eglGetConfigs(userData.eglDisplay, KD_NULL, 0, &numConfigs))
    {
        return EGL_FALSE;
    }

    // Choose config
    if(!eglChooseConfig(userData.eglDisplay, attribList, &config, 1, &numConfigs))
    {
        return EGL_FALSE;
    }

    // Use OpenKODE to create a Window
    userData.window = kdCreateWindow(userData.eglDisplay, config, KD_NULL);
    if(!userData.window)
    {
        kdExit(0);
    }

    if(!InitEGLContext(&userData, config))
    {
        kdExit(0);
    }

    if(!Init(&userData))
    {
        kdExit(0);
    }

    // Main Loop
    while(1)
    {
        // Wait for an event
        const KDEvent *evt = kdWaitEvent(0);
        if(evt)
        {
            // Exit app
            if(evt->type == KD_EVENT_WINDOW_CLOSE)
            {
                break;
            }
        }

        // Draw frame
        Draw(&userData);
    }

    // EGL clean up
    eglMakeCurrent(0, 0, 0, 0);
    eglDestroySurface(userData.eglDisplay, userData.eglSurface);
    eglDestroyContext(userData.eglDisplay, userData.eglContext);

    // Destroy the window
    kdDestroyWindow(userData.window);

    return 0;
}
