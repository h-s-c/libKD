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
#include <GLES2/gl2.h>

#ifdef __cplusplus
extern "C" {
#endif

/****************************************************************************** 
 * OpenGL ES 2.0 
 ******************************************************************************/

static GLuint exampleLoadTexture(const KDchar *filename);
static GLuint exampleCreateShader(GLenum type, const KDchar *shadersrc);
static GLuint exampleCreateProgram(const KDchar *vertexsrc, const KDchar *fragmentsrc);

/****************************************************************************** 
 * 4x4 Matrix
 ******************************************************************************/

typedef KDfloat32 Matrix4x4[16];
typedef KDfloat32 Matrix3x3[9];
static void exampleMatrixCopy(Matrix4x4 dst, Matrix4x4 src);
static void exampleMatrixFrustum(Matrix4x4 m, KDfloat32 left, KDfloat32 right, KDfloat32 bottom, KDfloat32 top, KDfloat32 near, KDfloat32 far);
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

GLuint exampleCreateProgram(const KDchar *vertexsrc, const KDchar *fragmentsrc)
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

    glLinkProgram(program);
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

static KDfloat32 normalize(KDfloat32 *ax, KDfloat32 *ay, KDfloat32 *az)
{
    KDfloat32 norm = kdSqrtf((*ax) * (*ax) + (*ay) * (*ay) + (*az) * (*az));
    if (norm > 0)
    {
        *ax /= norm;
        *ay /= norm;
        *az /= norm;
    }
    return norm;
}

void exampleMatrixRotate(Matrix4x4 m, KDfloat32 angle, KDfloat32 x, KDfloat32 y, KDfloat32 z)
{
    Matrix4x4 rot;
    KDfloat32 r = angle * KD_PI_F / 180.f;
    KDfloat32 s = kdSinf(r);
    KDfloat32 c = kdCosf(r);
    KDfloat32 one_c = 1.f - c;
    KDfloat32 xx, yy, zz, xy, yz, xz, xs, ys, zs;
    KDfloat32 norm = normalize(&x, &y, &z);

    if (norm == 0 || angle == 0)
    {
        return;
    }

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