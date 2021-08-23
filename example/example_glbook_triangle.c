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

#define EXAMPLE_COMMON_IMPLEMENTATION
#include "example_common.h"

typedef struct
{
    // Handle to a program object
    GLuint programObject;
} UserData;

///
// Initialize the shader and program object
//
KDboolean Init(Example *example)
{
    UserData *userData = (UserData *)example->userptr;

    const KDchar *vShaderStr =
        "attribute vec4 vPosition;    \n"
        "void main()                  \n"
        "{                            \n"
        "   gl_Position = vPosition;  \n"
        "}                            \n";

    const KDchar *fShaderStr =
        "#ifdef GL_FRAGMENT_PRECISION_HIGH              \n"
        "   precision highp float;                      \n"
        "#else                                          \n"
        "   precision mediump float;                    \n"
        "#endif                                         \n"
        "                                               \n"
        "void main()                                    \n"
        "{                                              \n"
        "  gl_FragColor = vec4 ( 1.0, 0.0, 0.0, 1.0 );  \n"
        "}                                              \n";

    // Store the program object
    userData->programObject = exampleCreateProgram(vShaderStr, fShaderStr, KD_FALSE);
    
    // Bind vPosition to attribute 0
    glBindAttribLocation(userData->programObject, 0, "vPosition");
    glLinkProgram(userData->programObject);

    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    return KD_TRUE;
}

///
// Draw a triangle using the shader pair created in Init()
//
void Draw(Example *example)
{
    UserData *userData = (UserData *)example->userptr;

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
    eglQuerySurface(example->egl.display, example->egl.surface, EGL_WIDTH, &width);
    eglQuerySurface(example->egl.display, example->egl.surface, EGL_HEIGHT, &height);
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
}


///
// kdMain()
//
//    Main function for OpenKODE application
//
KDint KD_APIENTRY kdMain(KDint argc, const KDchar *const *argv)
{
    Example *example = exampleInit();

    UserData userData;
    example->userptr = &userData;
    Init(example);

    // Main Loop
    while(example->run)
    {
        const KDEvent *event = kdWaitEvent(-1);
        if(event)
        {
            switch(event->type)
            {
                case(KD_EVENT_QUIT):
                case(KD_EVENT_WINDOW_CLOSE):
                {
                    example->run = KD_FALSE;
                    break;
                }
                default:
                {
                    kdDefaultEvent(event);
                    break;
                }
            }
        }

        // Draw frame
        Draw(example);
        exampleRun(example);
    }

    return exampleDestroy(example);
}
