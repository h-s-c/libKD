//
// Book:      OpenGL(R) ES 2.0 Programming Guide
// Authors:   Aaftab Munshi, Dan Ginsburg, Dave Shreiner
// ISBN-10:   0321502795
// ISBN-13:   9780321502797
// Publisher: Addison-Wesley Professional
// URLs:      http://safari.informit.com/9780321563835
//            http://www.opengles-book.com
//

//    This is an example that demonstrates rendering a particle system
//    using a vertex shader and point sprites.
//
#include <KD/kd.h>
#include <KD/kdext.h>
#include <EGL/egl.h>
#include <GLES2/gl2.h>

#define NUM_PARTICLES 2000
#define PARTICLE_SIZE 7

typedef struct
{
    // Handle to a program object
    GLuint programObject;

    // Attribute locations
    GLint lifetimeLoc;
    GLint startPositionLoc;
    GLint endPositionLoc;

    // Uniform location
    GLint timeLoc;
    GLint colorLoc;
    GLint centerPositionLoc;
    GLint samplerLoc;

    // Texture handle
    GLuint textureId;

    // Particle vertex data
    GLfloat particleData[NUM_PARTICLES * PARTICLE_SIZE];

    // Current time
    KDfloat32 time;

    GLuint vertexObject;

    // EGL handles
    EGLDisplay eglDisplay;
    EGLContext eglContext;
    EGLSurface eglSurface;

    //KD handles
    KDWindow *window;
} UserData;

///
// Load texture from disk
//
GLuint LoadTexture(const KDchar *fileName)
{
    KDImageATX image = kdGetImageATX(fileName, KD_IMAGE_FORMAT_RGB888_ATX, 0);
    if(!image)
    {
        kdLogMessagefKHR("Error loading (%s) image.\n", fileName);
        return 0;
    }
    KDint width = kdGetImageIntATX(image, KD_IMAGE_WIDTH_ATX);
    KDint height = kdGetImageIntATX(image, KD_IMAGE_HEIGHT_ATX);
    KDchar *buffer = kdGetImagePointerATX(image, KD_IMAGE_POINTER_BUFFER_ATX);
    GLuint texId;

    glGenTextures(1, &texId);
    glBindTexture(GL_TEXTURE_2D, texId);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, buffer);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    kdFreeImageATX(image);

    return texId;
}

///
// Create a shader object, load the shader source, and
// compile the shader.
//
GLuint LoadShader(GLenum type, const char *shaderSrc)
{
    GLuint shader;
    GLint compiled;

    // Create the shader object
    shader = glCreateShader(type);

    if(shader == 0)
    {
        return 0;
    }

    // Load the shader source
    glShaderSource(shader, 1, &shaderSrc, KD_NULL);

    // Compile the shader
    glCompileShader(shader);

    // Check the compile status
    glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);

    if(!compiled)
    {
        GLint infoLen = 0;

        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infoLen);

        if(infoLen > 1)
        {
            char *infoLog = kdMalloc(sizeof(char) * infoLen);

            glGetShaderInfoLog(shader, infoLen, KD_NULL, infoLog);
            kdLogMessage(infoLog);

            kdFree(infoLog);
        }

        glDeleteShader(shader);
        return 0;
    }

    return shader;
}

///
// Initialize the shader and program object
//
KDboolean Init(UserData *userData)
{
    const KDchar *vShaderStr =
        "uniform float u_time;		                           \n"
        "uniform vec3 u_centerPosition;                       \n"
        "attribute float a_lifetime;                          \n"
        "attribute vec3 a_startPosition;                      \n"
        "attribute vec3 a_endPosition;                        \n"
        "varying float v_lifetime;                            \n"
        "void main()                                          \n"
        "{                                                    \n"
        "  if ( u_time <= a_lifetime )                        \n"
        "  {                                                  \n"
        "    gl_Position.xyz = a_startPosition +              \n"
        "                      (u_time * a_endPosition);      \n"
        "    gl_Position.xyz += u_centerPosition;             \n"
        "    gl_Position.w = 1.0;                             \n"
        "  }                                                  \n"
        "  else                                               \n"
        "     gl_Position = vec4( -1000, -1000, 0, 0 );       \n"
        "  v_lifetime = 1.0 - ( u_time / a_lifetime );        \n"
        "  v_lifetime = clamp ( v_lifetime, 0.0, 1.0 );       \n"
        "  gl_PointSize = ( v_lifetime * v_lifetime ) * 40.0; \n"
        "}";

    const KDchar *fShaderStr =
        "precision mediump float;                             \n"
        "uniform vec4 u_color;		                           \n"
        "varying float v_lifetime;                            \n"
        "uniform sampler2D s_texture;                         \n"
        "void main()                                          \n"
        "{                                                    \n"
        "  vec4 texColor;                                     \n"
        "  texColor = texture2D( s_texture, gl_PointCoord );  \n"
        "  gl_FragColor = vec4( u_color ) * texColor;         \n"
        "  gl_FragColor.a *= v_lifetime;                      \n"
        "}                                                    \n";

    GLuint vertexShader;
    GLuint fragmentShader;
    GLuint programObject;
    GLint linked;

    // Load the vertex/fragment shaders
    vertexShader = LoadShader(GL_VERTEX_SHADER, vShaderStr);
    fragmentShader = LoadShader(GL_FRAGMENT_SHADER, fShaderStr);

    // Create the program object
    programObject = glCreateProgram();

    if(programObject == 0)
    {
        return KD_FALSE;
    }

    glAttachShader(programObject, vertexShader);
    glAttachShader(programObject, fragmentShader);

    // Bind vPosition to attribute 0
    glBindAttribLocation(programObject, 0, "vPosition");

    // Link the program
    glLinkProgram(programObject);

    // Check the link status
    glGetProgramiv(programObject, GL_LINK_STATUS, &linked);

    if(!linked)
    {
        GLint infoLen = 0;

        glGetProgramiv(programObject, GL_INFO_LOG_LENGTH, &infoLen);

        if(infoLen > 1)
        {
            KDchar *infoLog = kdMalloc(sizeof(char) * infoLen);

            glGetProgramInfoLog(programObject, infoLen, KD_NULL, infoLog);
            kdLogMessage(infoLog);

            kdFree(infoLog);
        }

        glDeleteProgram(programObject);
        return KD_FALSE;
    }

    // Store the program object
    userData->programObject = programObject;

    // Get the attribute locations
    userData->lifetimeLoc = glGetAttribLocation(userData->programObject, "a_lifetime");
    userData->startPositionLoc = glGetAttribLocation(userData->programObject, "a_startPosition");
    userData->endPositionLoc = glGetAttribLocation(userData->programObject, "a_endPosition");

    // Get the uniform locations
    userData->timeLoc = glGetUniformLocation(userData->programObject, "u_time");
    userData->centerPositionLoc = glGetUniformLocation(userData->programObject, "u_centerPosition");
    userData->colorLoc = glGetUniformLocation(userData->programObject, "u_color");
    userData->samplerLoc = glGetUniformLocation(userData->programObject, "s_texture");

    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

    // Fill in particle data array
    for(KDint i = 0; i < NUM_PARTICLES; i++)
    {
        GLfloat *particleData = &userData->particleData[i * PARTICLE_SIZE];

        KDuint8 buffer[7] = {0};
        kdCryptoRandom(buffer, 7);

        // Lifetime of particle
        (*particleData++) = ((KDfloat32)((buffer[0] * 128) % 10000) / 10000.0f);

        // End position of particle
        (*particleData++) = ((KDfloat32)((buffer[1] * 128) % 10000) / 5000.0f) - 1.0f;
        (*particleData++) = ((KDfloat32)((buffer[2] * 128) % 10000) / 5000.0f) - 1.0f;
        (*particleData++) = ((KDfloat32)((buffer[3] * 128) % 10000) / 5000.0f) - 1.0f;

        // Start position of particle
        (*particleData++) = ((KDfloat32)((buffer[4] * 128) % 10000) / 40000.0f) - 0.125f;
        (*particleData++) = ((KDfloat32)((buffer[5] * 128) % 10000) / 40000.0f) - 0.125f;
        (*particleData++) = ((KDfloat32)((buffer[6] * 128) % 10000) / 40000.0f) - 0.125f;
    }

    glGenBuffers(1, &userData->vertexObject);
    glBindBuffer(GL_ARRAY_BUFFER, userData->vertexObject);
    glBufferData(GL_ARRAY_BUFFER, NUM_PARTICLES * PARTICLE_SIZE * 4, userData->particleData, GL_STATIC_DRAW);

    // Initialize time to cause reset on first update
    userData->time = 1.0f;

    userData->textureId = LoadTexture("data/smoke.jpg");
    if(userData->textureId <= 0)
    {
        return KD_FALSE;
    }

    return KD_TRUE;
}

///
//  Update time-based variables
//
void Update(UserData *userData, KDfloat32 deltaTime)
{
    userData->time += deltaTime;

    if(userData->time >= 1.0f)
    {
        KDfloat32 centerPos[3];
        KDfloat32 color[4];

        userData->time = 0.0f;

        KDuint8 buffer[6] = {0};
        kdCryptoRandom(buffer, 6);

        // Pick a new start location and color
        centerPos[0] = ((KDfloat32)((buffer[0] * 128) % 10000) / 10000.0f) - 0.5f;
        centerPos[1] = ((KDfloat32)((buffer[1] * 128) % 10000) / 10000.0f) - 0.5f;
        centerPos[2] = ((KDfloat32)((buffer[2] * 128) % 10000) / 10000.0f) - 0.5f;

        glUniform3fv(userData->centerPositionLoc, 1, &centerPos[0]);

        // Random color
        color[0] = ((KDfloat32)((buffer[3] * 128) % 10000) / 20000.0f) + 0.5f;
        color[1] = ((KDfloat32)((buffer[4] * 128) % 10000) / 20000.0f) + 0.5f;
        color[2] = ((KDfloat32)((buffer[5] * 128) % 10000) / 20000.0f) + 0.5f;
        color[3] = 0.5;

        glUniform4fv(userData->colorLoc, 1, &color[0]);
    }

    // Load uniform time variable
    glUniform1f(userData->timeLoc, userData->time);
}

///
// Draw a triangle using the shader pair created in Init()
//
void Draw(UserData *userData)
{
    // Set the viewport
    KDint32 windowsize[2];
    kdGetWindowPropertyiv(userData->window, KD_WINDOWPROPERTY_SIZE, windowsize);
    glViewport(0, 0, windowsize[0], windowsize[1]);

    // Clear the color buffer
    glClear(GL_COLOR_BUFFER_BIT);

    // Use the program object
    glUseProgram(userData->programObject);

    // Load the vertex attributes
    glBindBuffer(GL_ARRAY_BUFFER, userData->vertexObject);
    glVertexAttribPointer(userData->lifetimeLoc, 1, GL_FLOAT,
        GL_FALSE, PARTICLE_SIZE * sizeof(GLfloat),
        (const GLvoid *)0);

    glVertexAttribPointer(userData->endPositionLoc, 3, GL_FLOAT,
        GL_FALSE, PARTICLE_SIZE * sizeof(GLfloat),
        (const GLvoid *)4);

    glVertexAttribPointer(userData->startPositionLoc, 3, GL_FLOAT,
        GL_FALSE, PARTICLE_SIZE * sizeof(GLfloat),
        (const GLvoid *)16);


    glEnableVertexAttribArray(userData->lifetimeLoc);
    glEnableVertexAttribArray(userData->endPositionLoc);
    glEnableVertexAttribArray(userData->startPositionLoc);
    // Blend particles
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);

    // Bind the texture
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, userData->textureId);

    // Set the sampler texture unit to 0
    glUniform1i(userData->samplerLoc, 0);

    glDrawArrays(GL_POINTS, 0, NUM_PARTICLES);

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
// Cleanup
//
void ShutDown(UserData *userData)
{
    // Delete texture object
    glDeleteTextures (1, &userData->textureId);

    // Delete program object
    glDeleteProgram (userData->programObject);
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
        EGL_SURFACE_TYPE,       EGL_WINDOW_BIT,
        EGL_RENDERABLE_TYPE,    EGL_OPENGL_ES2_BIT,
        EGL_RED_SIZE,           8,
        EGL_GREEN_SIZE,         8,
        EGL_BLUE_SIZE,          8,
        EGL_ALPHA_SIZE,         EGL_DONT_CARE,
        EGL_DEPTH_SIZE,         EGL_DONT_CARE,
        EGL_STENCIL_SIZE,       EGL_DONT_CARE,
        EGL_SAMPLE_BUFFERS,     0,
        EGL_NONE
    };

    EGLint majorVersion, minorVersion;
    UserData userData;
    EGLint numConfigs;
    EGLConfig config;

    userData.eglDisplay = eglGetDisplay(EGL_DEFAULT_DISPLAY);

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

    KDust t1 = kdGetTimeUST();
    KDfloat32 deltatime;
    KDfloat32 totaltime = 0.0f;
    KDuint frames = 0;

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

        // Update
        KDust t2 = kdGetTimeUST();
        deltatime = (KDfloat32)((t2 - t1) * 1e-9);
        t1 = t2;
        Update(&userData, deltatime);

        // Draw frame
        Draw(&userData);

        // Benchmark
        totaltime += deltatime;
        frames++;
        if(totaltime > 2.0f)
        {
            kdLogMessagefKHR("%4d frames rendered in %1.4f seconds -> FPS=%3.4f\n", frames, totaltime, frames / totaltime);
            totaltime -= 2.0f;
            frames = 0;
        }
    }

    ShutDown(&userData);

    // EGL clean up
    eglMakeCurrent(0, 0, 0, 0);
    eglDestroySurface(userData.eglDisplay, userData.eglSurface);
    eglDestroyContext(userData.eglDisplay, userData.eglContext);

    // Destroy the window
    kdDestroyWindow(userData.window);

    return 0;
}
