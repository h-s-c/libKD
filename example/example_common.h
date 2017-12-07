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

#ifndef EXAMPLE_COMMON_H
#define EXAMPLE_COMMON_H

#include <KD/kd.h>
#include <KD/kdext.h>
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>

#if defined(__MINGW32__)
#undef near
#undef far
#endif

#ifdef __cplusplus
extern "C" {
#endif

/****************************************************************************** 
 * General
 ******************************************************************************/

typedef struct Example {
    KDboolean run;
    void *userptr;
    struct
    {
        EGLDisplay display;
        EGLConfig config;
        EGLSurface surface;
        EGLContext context;
        EGLint attrib_list[8];
        EGLNativeWindowType window;
        const KDchar *extensions;
    } egl;
    struct
    {
        KDWindow *window;
    } kd;
    struct
    {
        KDboolean enable;
    } shadercache;
} Example;

Example *exampleInit(void);
void exampleRun(Example *example);
KDint exampleDestroy(Example *example);

/****************************************************************************** 
 * OpenGL ES 2.0 
 ******************************************************************************/

static GLuint exampleLoadTexture(const KDchar *filename);
static GLuint exampleCreateShader(GLenum type, const KDchar *shadersrc);
static GLuint exampleCreateProgram(const KDchar *vertexsrc, const KDchar *fragmentsrc, KDboolean link);

/****************************************************************************** 
 * 4x4 Matrix
 ******************************************************************************/

typedef KDfloat32 Matrix4x4[16];
typedef KDfloat32 Matrix3x3[9];
static void exampleMatrixCopy(Matrix4x4 dst, Matrix4x4 src);
static void exampleMatrixFrustum(Matrix4x4 m, KDfloat32 left, KDfloat32 right, KDfloat32 bottom, KDfloat32 top, KDfloat32 _near, KDfloat32 far);
static void exampleMatrixMultiply(Matrix4x4 m, Matrix4x4 n);
static void exampleMatrixIdentity(Matrix4x4 m);
static void exampleMatrixPerspective(Matrix4x4 m, KDfloat32 fovy, KDfloat32 aspect, KDfloat32 nearz, KDfloat32 farz);
static void exampleMatrixRotate(Matrix4x4 m, KDfloat32 angle, KDfloat32 x, KDfloat32 y, KDfloat32 z);
static void exampleMatrixScale(Matrix4x4 m, KDfloat32 x, KDfloat32 y, KDfloat32 z);
static void exampleMatrixTransform(Matrix4x4 m, KDfloat32 *x, KDfloat32 *y, KDfloat32 *z);
static void exampleMatrixTranslate(Matrix4x4 m, KDfloat32 x, KDfloat32 y, KDfloat32 z);

#ifdef __cplusplus
}
#endif

#ifdef EXAMPLE_COMMON_IMPLEMENTATION

/****************************************************************************** 
 * General
 ******************************************************************************/

static void KD_APIENTRY exampleCallbackKD(const KDEvent *event)
{
    switch(event->type)
    {
        case(KD_EVENT_QUIT):
        {
            Example *example = event->userptr;
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

#if defined(GL_KHR_debug)
static void GL_APIENTRY exampleCallbackGL(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar *message, const void *userParam)
{
    kdLogMessagefKHR("%s\n", message);
}
#endif

Example *exampleInit(void)
{
    Example *example = (Example *)kdMalloc(sizeof (Example));

    example->egl.display = eglGetDisplay(kdGetDisplayVEN());
    kdAssert(example->egl.display != EGL_NO_DISPLAY);

    eglInitialize(example->egl.display, 0, 0);
    eglBindAPI(EGL_OPENGL_ES_API);

    example->egl.extensions = eglQueryString(example->egl.display, EGL_EXTENSIONS); 

    EGLint egl_num_configs = 0;
    const EGLint egl_attributes[] =
    {
        EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
        EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
        EGL_RED_SIZE, 8,
        EGL_GREEN_SIZE, 8,
        EGL_BLUE_SIZE, 8,
        EGL_ALPHA_SIZE, 8,
        EGL_DEPTH_SIZE, 16,
        EGL_SAMPLE_BUFFERS, 0,
        EGL_NONE
    };

    eglChooseConfig(example->egl.display, egl_attributes, &example->egl.config, 1, &egl_num_configs);

    example->kd.window = kdCreateWindow(example->egl.display, example->egl.config, KD_NULL);
    kdRealizeWindow(example->kd.window, &example->egl.window);

    example->egl.surface = eglCreateWindowSurface(example->egl.display, example->egl.config, example->egl.window, KD_NULL);
    
    KDint offset = 0;
    const EGLint context_attributes[] =
    {
        EGL_CONTEXT_CLIENT_VERSION,
        2
    };
    kdMemcpy(example->egl.attrib_list + offset, context_attributes, sizeof(context_attributes));
    offset += (sizeof(context_attributes) /sizeof (context_attributes[0]));

#if !defined(KD_NDEBUG) && defined(EGL_KHR_create_context)
    const EGLint context_attributes_ext1[] =
    {
        EGL_CONTEXT_FLAGS_KHR,
        EGL_CONTEXT_OPENGL_DEBUG_BIT_KHR
    };
    if(kdStrstrVEN(example->egl.extensions, "EGL_KHR_create_context"))
    {
        kdMemcpy(example->egl.attrib_list+offset, context_attributes_ext1, sizeof(context_attributes_ext1));
        offset += (sizeof(context_attributes_ext1) /sizeof (context_attributes_ext1[0]));
    }
#endif

#if defined(EGL_IMG_context_priority)
    const EGLint context_attributes_ext2[] =
    {
        EGL_CONTEXT_PRIORITY_LEVEL_IMG,
        EGL_CONTEXT_PRIORITY_HIGH_IMG
    };
    if(kdStrstrVEN(example->egl.extensions, "EGL_IMG_context_priority"))
    {
        kdMemcpy(example->egl.attrib_list+offset, context_attributes_ext2, sizeof(context_attributes_ext2));
        offset += (sizeof(context_attributes_ext2) /sizeof (context_attributes_ext2[0]));
    }
#endif

    const EGLint context_attributes_end[] =
    {
        EGL_NONE
    };
    kdMemcpy(example->egl.attrib_list+offset, context_attributes_end, sizeof(context_attributes_end));

    example->egl.context = eglCreateContext(example->egl.display, example->egl.config, EGL_NO_CONTEXT, example->egl.attrib_list);
    kdAssert(example->egl.context != EGL_NO_CONTEXT);

    eglMakeCurrent(example->egl.display, example->egl.surface, example->egl.surface, example->egl.context);

    eglSwapInterval(example->egl.display, 1);

    kdAssert(eglGetError() == EGL_SUCCESS);

#if defined(GL_KHR_debug)
    if(kdStrstrVEN((const KDchar *)glGetString(GL_EXTENSIONS), "GL_KHR_debug"))
    {
        glEnable(GL_DEBUG_OUTPUT_KHR);
        glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS_KHR);
        PFNGLDEBUGMESSAGECALLBACKKHRPROC  glDebugMessageCallbackKHR = (PFNGLDEBUGMESSAGECALLBACKKHRPROC)eglGetProcAddress("glDebugMessageCallbackKHR");
        glDebugMessageCallbackKHR(&exampleCallbackGL, KD_NULL);
    }
#endif

    /* Debug message */
    kdLogMessage("-----KD-----\n");
    kdLogMessagefKHR("Vendor: %s\n", kdQueryAttribcv(KD_ATTRIB_VENDOR));
    kdLogMessagefKHR("Version: %s\n", kdQueryAttribcv(KD_ATTRIB_VERSION));
    kdLogMessagefKHR("Platform: %s\n", kdQueryAttribcv(KD_ATTRIB_PLATFORM));
    kdLogMessage("-----EGL-----\n");
    kdLogMessagefKHR("Vendor: %s\n", eglQueryString(example->egl.display, EGL_VENDOR));
    kdLogMessagefKHR("Version: %s\n", eglQueryString(example->egl.display, EGL_VERSION));
    kdLogMessagefKHR("Client APIs: %s\n", eglQueryString(example->egl.display, EGL_CLIENT_APIS));
    kdLogMessagefKHR("Extensions: %s\n", example->egl.extensions);
    kdLogMessage("-----GLES2-----\n");
    kdLogMessagefKHR("Vendor: %s\n", (const KDchar *)glGetString(GL_VENDOR));
    kdLogMessagefKHR("Version: %s\n", (const KDchar *)glGetString(GL_VERSION));
    kdLogMessagefKHR("Renderer: %s\n", (const KDchar *)glGetString(GL_RENDERER));
    kdLogMessagefKHR("Extensions: %s\n", (const KDchar *)glGetString(GL_EXTENSIONS));
#if defined(GL_OES_get_program_binary)
    if(kdStrstrVEN((const KDchar *)glGetString(GL_EXTENSIONS), "GL_OES_get_program_binary"))
    {
        example->shadercache.enable = KD_TRUE;
        kdLogMessage("GL_OES_get_program_binary formats: ");

        const KDchar *dummyvertexsrc =
            "void main()                    \n"
            "{                          \n"
            "   gl_Position = gl_Vertex;    \n"
            "}                              \n";

        const KDchar *dummyfragmentsrc =
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

        GLuint dummyprogram = exampleCreateProgram(dummyvertexsrc, dummyfragmentsrc, KD_TRUE);
        glUseProgram(dummyprogram);

        GLint formats = 0;
        glGetIntegerv(GL_NUM_PROGRAM_BINARY_FORMATS_OES, &formats);
        if(formats > 0)
        {
            GLint *binary_formats = kdMalloc(sizeof(GLint) * formats);
            glGetIntegerv(GL_PROGRAM_BINARY_FORMATS_OES, binary_formats);
            for (KDint i = 0; i < formats; ++i)
            {
#if defined(GL_MESA_program_binary_formats)
                if(binary_formats[i] == GL_PROGRAM_BINARY_FORMAT_MESA)
                {
                    kdLogMessage("Mesa ");
                }
#endif
                kdLogMessagefKHR("%d ", binary_formats[i]);
            }
            kdFree(binary_formats);
            kdLogMessage("\n");
        }
        else
        {
            kdLogMessage("None");
        }
        kdLogMessage("\n");
        glUseProgram(0);
        glDeleteProgram(dummyprogram);
    }
#endif
    kdLogMessage("---------------\n");

    kdInstallCallback(&exampleCallbackKD, KD_EVENT_QUIT, example);

    example->run = KD_TRUE;
    return example;
}

void exampleRun(Example *example)
{
    if(!example->run)
    {
        return;
    }
    if(eglSwapBuffers(example->egl.display, example->egl.surface) == EGL_FALSE)
    {
        EGLint egl_error = eglGetError();
        switch(egl_error)
        {
            case(EGL_BAD_SURFACE):
            {
                eglMakeCurrent(example->egl.display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
                eglDestroySurface(example->egl.display, example->egl.surface);
                kdRealizeWindow(example->kd.window, &example->egl.window);
                example->egl.surface = eglCreateWindowSurface(example->egl.display, example->egl.config, example->egl.window, KD_NULL);
                eglMakeCurrent(example->egl.display, example->egl.surface, example->egl.surface, example->egl.context);
                break;
            }
            case(EGL_BAD_MATCH):
            case(EGL_BAD_CONTEXT):
            case(EGL_CONTEXT_LOST):
            {
                eglMakeCurrent(example->egl.display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
                eglDestroyContext(example->egl.display, example->egl.context);
                example->egl.context = eglCreateContext(example->egl.display, example->egl.config, EGL_NO_CONTEXT, example->egl.attrib_list);
                eglMakeCurrent(example->egl.display, example->egl.surface, example->egl.surface, example->egl.context);
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
        example->run = KD_FALSE;
    }
    else
    {
        example->run = KD_TRUE;
    }
}

KDint exampleDestroy(Example *example)
{
    eglMakeCurrent(KD_NULL, KD_NULL, KD_NULL, KD_NULL);
    eglDestroyContext(example->egl.display, example->egl.context);
    eglDestroySurface(example->egl.display, example->egl.surface);
    eglTerminate(example->egl.display);
    kdDestroyWindow(example->kd.window);
    kdFree(example);
    return 0;
}

/****************************************************************************** 
 * OpenGL ES 2.0 
 ******************************************************************************/

GLuint exampleLoadTexture(const KDchar *filename)
{
    KDImageATX image = kdGetImageATX(filename, KD_IMAGE_FORMAT_RGB888_ATX, 0);
    if(!image)
    {
        kdLogMessagefKHR("Error loading (%s) image.\n", filename);
        return 0;
    }
    KDint width = kdGetImageIntATX(image, KD_IMAGE_WIDTH_ATX);
    KDint height = kdGetImageIntATX(image, KD_IMAGE_HEIGHT_ATX);
    KDchar *buffer = kdGetImagePointerATX(image, KD_IMAGE_POINTER_BUFFER_ATX);
    GLuint id;

    glGenTextures(1, &id);
    glBindTexture(GL_TEXTURE_2D, id);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, buffer);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    kdFreeImageATX(image);

    return id;
}

GLuint exampleCreateShader(GLenum type, const KDchar *shadersrc)
{
    GLuint shader = glCreateShader(type);
    if(shader == 0)
    {
        return 0;
    }

    GLint compiled = 0;
    glShaderSource(shader, 1, &shadersrc, KD_NULL);
    glCompileShader(shader);
    glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);

    if(!compiled)
    {
        GLint len = 0;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &len);

        if(len > 1)
        {
            KDchar *log = kdMalloc(sizeof(KDchar) * len);
            glGetShaderInfoLog(shader, len, KD_NULL, log);
            kdLogMessagefKHR("%s\n", log);
            kdFree(log);
        }

        glDeleteShader(shader);
        return 0;
    }
    return shader;
}

GLuint exampleCreateProgram(const KDchar *vertexsrc, const KDchar *fragmentsrc, KDboolean link)
{
    GLuint program = glCreateProgram();
    if(program == 0)
    {
        return 0;
    }

    GLuint vertexshader = exampleCreateShader(GL_VERTEX_SHADER, vertexsrc);
    if(vertexshader == 0)
    {
        glDeleteProgram(program);
        return 0;
    }
    glAttachShader(program, vertexshader);
    glDeleteShader(vertexshader);

    GLuint fragmentshader = exampleCreateShader(GL_FRAGMENT_SHADER, fragmentsrc);
    if (fragmentshader == 0)
    {
        glDeleteProgram(program);
        return 0;
    }
    glAttachShader(program, fragmentshader);
    glDeleteShader(fragmentshader);

    if(link)
    {
        GLint linked = 0;
        glLinkProgram(program);
        glGetProgramiv(program, GL_LINK_STATUS, &linked);

        if(!linked)
        {
            GLint len = 0;
            glGetProgramiv(program, GL_INFO_LOG_LENGTH, &len);

            if(len > 1)
            {
                KDchar *log = kdMalloc(sizeof(KDchar) * len);
                glGetProgramInfoLog(program, len, KD_NULL, log);
                kdLogMessagefKHR("%s\n", log);
                kdFree(log);
            }
            return 0;
        }
    }
    return program;
}

/****************************************************************************** 
 * 4x4 Matrix
 ******************************************************************************/
/******************************************************************************
 * Copyright 2010 The Chromium Authors. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 *       copyright notice, this list of conditions and the following disclaimer
 *       in the documentation and/or other materials provided with the
 *       distribution.
 *     * Neither the name of Google Inc. nor the names of its
 *       contributors may be used to endorse or promote products derived from
 *       this software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 ******************************************************************************/

void exampleMatrixCopy(Matrix4x4 dst, Matrix4x4 src)
{
    KDint i;
    for (i = 0; i < 16; ++i)
    {
        dst[i] = src[i];
    }
}

void exampleMatrixFrustum(Matrix4x4 m, KDfloat32 left, KDfloat32 right, KDfloat32 bottom, KDfloat32 top, KDfloat32 near, KDfloat32 far)
{
    KDfloat32 dx = right - left;
    KDfloat32 dy = top - bottom;
    KDfloat32 dz = far - near;
    Matrix4x4 frust;

    if (near <= 0.f || far <= 0.f || dx <= 0.f || dy <= 0.f || dz <= 0.f)
    {
        return;
    }

    frust[0*4 + 0] = 2.f * near / dx;
    frust[0*4 + 1] = 0.f;
    frust[0*4 + 2] = 0.f;
    frust[0*4 + 3] = 0.f;

    frust[1*4 + 0] = 0.f;
    frust[1*4 + 1] = 2.f * near / dy;
    frust[1*4 + 2] = 0.f;
    frust[1*4 + 3] = 0.f;

    frust[2*4 + 0] = (right + left) / dx;
    frust[2*4 + 1] = (top + bottom) / dy;
    frust[2*4 + 2] = -(near + far) / dz;
    frust[2*4 + 3] = -1.f;

    frust[3*4 + 0] = 0.f;
    frust[3*4 + 1] = 0.f;
    frust[3*4 + 2] = -2.f * near * far / dz;
    frust[3*4 + 3] = 0.f;

    exampleMatrixMultiply(m, frust);
}

void exampleMatrixMultiply(Matrix4x4 m, Matrix4x4 n)
{
    Matrix4x4 tmp;
    KDint i, j, k;
    for (i = 0; i < 4; i++)
    {
        for (j = 0; j < 4; ++j) 
        {
            tmp[i*4 + j] = 0;
            for (k = 0; k < 4; ++k)
            {
                tmp[i*4 + j] += n[i*4 + k] * m[k*4 + j];
            }
        }
    }
    exampleMatrixCopy(m, tmp);
}

void exampleMatrixIdentity(Matrix4x4 m)
{
    KDint i;
    for (i = 0; i < 16; ++i)
    {
        m[i] = 0;
    }
    for (i = 0; i < 4; ++i)
    {
        m[i*4 + i] = 1.f;
    }
}

void exampleMatrixPerspective(Matrix4x4 m, KDfloat32 fovy, KDfloat32 aspect, KDfloat32 nearz, KDfloat32 farz)
{
    KDfloat32 frustumh = kdTanf(fovy / 360.f * KD_PI_F) * nearz;
    KDfloat32 frustumw = frustumh * aspect;

    exampleMatrixFrustum(m, -frustumw, frustumw, -frustumh, frustumh, nearz, farz);
}

void exampleMatrixRotate(Matrix4x4 m, KDfloat32 angle, KDfloat32 x, KDfloat32 y, KDfloat32 z)
{
    Matrix4x4 rot;
    KDfloat32 r = angle * KD_PI_F / 180.f;
    KDfloat32 s = kdSinf(r);
    KDfloat32 c = kdCosf(r);
    KDfloat32 one_c = 1.0f - c;
    KDfloat32 xx, yy, zz, xy, yz, xz, xs, ys, zs;
    
    xx = x * x;
    yy = y * y;
    zz = z * z;
    xy = x * y;
    yz = y * z;
    xz = x * z;
    xs = x * s;
    ys = y * s;
    zs = z * s;

    rot[0*4 + 0] = xx + (1.f - xx) * c;
    rot[1*4 + 0] = xy * one_c - zs;
    rot[2*4 + 0] = xz * one_c + ys;
    rot[3*4 + 0] = 0.f;

    rot[0*4 + 1] = xy * one_c + zs;
    rot[1*4 + 1] = yy + (1.f - yy) * c;
    rot[2*4 + 1] = yz * one_c - xs;
    rot[3*4 + 1] = 0.f;

    rot[0*4 + 2] = xz * one_c - ys;
    rot[1*4 + 2] = yz * one_c + xs;
    rot[2*4 + 2] = zz + (1.f - zz) * c;
    rot[3*4 + 2] = 0.f;

    rot[0*4 + 3] = 0.f;
    rot[1*4 + 3] = 0.f;
    rot[2*4 + 3] = 0.f;
    rot[3*4 + 3] = 1.f;

    exampleMatrixMultiply(m, rot);
}

void exampleMatrixScale(Matrix4x4 m, KDfloat32 x, KDfloat32 y, KDfloat32 z)
{
    KDint i;
    for (i = 0; i < 4; ++i)
    {
        m[0*4 + i] *= x;
        m[1*4 + i] *= y;
        m[2*4 + i] *= z;
    }
}

void exampleMatrixTransform(Matrix4x4 m, KDfloat32 *x, KDfloat32 *y, KDfloat32 *z)
{
    KDfloat32 tx = m[0*4 + 0] * (*x) + m[1*4 + 0] * (*y) + m[2*4 + 0] * (*z);
    KDfloat32 ty = m[0*4 + 1] * (*x) + m[1*4 + 1] * (*y) + m[2*4 + 1] * (*z);
    KDfloat32 tz = m[0*4 + 2] * (*x) + m[1*4 + 2] * (*y) + m[2*4 + 2] * (*z);
    *x = tx;
    *y = ty;
    *z = tz;
}

void exampleMatrixTranslate(Matrix4x4 m, KDfloat32 x, KDfloat32 y, KDfloat32 z)
{
    KDint i;
    for (i = 0; i < 4; ++i)
    {
        m[3*4 + i] += m[0*4 + i] * x + m[1*4 + i] * y + m[2*4 + i] * z;
    }
}

#endif // EXAMPLE_COMMON_IMPLEMENTATION
#endif // EXAMPLE_COMMON_H