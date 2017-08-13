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

// Adapted from the javascript implementation upon WebGL by kwaters@.

#include "shadersrc.h"


typedef struct {
    // program
    GLuint program;
    // attribute
    GLint pos;
    GLint normal;
    GLint colorIn;
    // uniform
    GLint mvp;
    GLint normalMatrix;
    GLint ambient;
    GLint shininess;
    GLint light_0_direction;
    GLint light_0_diffuse;
    GLint light_0_specular;
    GLint light_1_direction;
    GLint light_1_diffuse;
    GLint light_2_direction;
    GLint light_2_diffuse;
} SHADERLIT;

typedef struct {
    // program
    GLuint program;
    // attribute
    GLint pos;
    GLint colorIn;
    // uniform
    GLint mvp;
} SHADERFLAT;

typedef struct {
    // program
    GLuint program;
    // attribute
    GLint pos;
    // uniform
    GLint minFade;
} SHADERFADE;

SHADERLIT sShaderLit;
SHADERFLAT sShaderFlat;
SHADERFADE sShaderFade;

Matrix4x4 sModelView;
Matrix4x4 sProjection;


static void printShaderLog(GLuint shader)
{
    KDint infoLogSize, infoWritten;
    KDchar *infoLog;
    glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infoLogSize);
    infoLog = kdMalloc(infoLogSize);
    glGetShaderInfoLog(shader, infoLogSize, &infoWritten, infoLog);
    kdLogMessagefKHR("Error: glCompileShader failed: %s\n", infoLog);
    kdFree(infoLog);
}


static GLuint createShader(const KDchar *src, GLenum shaderType)
{
    GLint bShaderCompiled;
    GLuint shader = glCreateShader(shaderType);
    if (shader == 0)
        return 0;
    glShaderSource(shader, 1, &src, KD_NULL);
    glCompileShader(shader);
    glGetShaderiv(shader, GL_COMPILE_STATUS, &bShaderCompiled);
    if (!bShaderCompiled)
    {
        printShaderLog(shader);
        glDeleteShader(shader);
        return 0;
    }
    return shader;
}


static GLuint createProgram(const char *srcVertex, const KDchar * srcFragment)
{
    GLuint program = glCreateProgram();
    if (program == 0)
        return 0;

    GLuint shaderVertex = createShader(srcVertex, GL_VERTEX_SHADER);
    if (shaderVertex == 0)
    {
        glDeleteProgram(program);
        return 0;
    }
    glAttachShader(program, shaderVertex);
    glDeleteShader(shaderVertex);

    GLuint shaderFragment = createShader(srcFragment, GL_FRAGMENT_SHADER);
    if (shaderFragment == 0)
    {
        glDeleteProgram(program);
        return 0;
    }
    glAttachShader(program, shaderFragment);
    glDeleteShader(shaderFragment);

    glLinkProgram(program);
    return program;
}


static void computeNormalMatrix(Matrix4x4 m, Matrix3x3 normal)
{
    KDfloat32 det = m[0*4+0] * (m[1*4+1] * m[2*4+2] - m[2*4+1] * m[1*4+2]) -
                m[0*4+1] * (m[1*4+0] * m[2*4+2] - m[1*4+2] * m[2*4+0]) +
                m[0*4+2] * (m[1*4+0] * m[2*4+1] - m[1*4+1] * m[2*4+0]);
    KDfloat32 invDet = 1.f / det;
    normal[0*3+0] = invDet * (m[1*4+1] * m[2*4+2] - m[2*4+1] * m[1*4+2]);
    normal[1*3+0] = invDet * -(m[0*4+1] * m[2*4+2] - m[0*4+2] * m[2*4+1]);
    normal[2*3+0] = invDet * (m[0*4+1] * m[1*4+2] - m[0*4+2] * m[1*4+1]);
    normal[0*3+1] = invDet * -(m[1*4+0] * m[2*4+2] - m[1*4+2] * m[2*4+0]);
    normal[1*3+1] = invDet * (m[0*4+0] * m[2*4+2] - m[0*4+2] * m[2*4+0]);
    normal[2*3+1] = invDet * -(m[0*4+0] * m[1*4+2] - m[1*4+0] * m[0*4+2]);
    normal[0*3+2] = invDet * (m[1*4+0] * m[2*4+1] - m[2*4+0] * m[1*4+1]);
    normal[1*3+2] = invDet * -(m[0*4+0] * m[2*4+1] - m[2*4+0] * m[0*4+1]);
    normal[2*3+2] = invDet * (m[0*4+0] * m[1*4+1] - m[1*4+0] * m[0*4+1]);
}


static KDint getLocations()
{
    KDint rt = 1;
#define GET_ATTRIBUTE_LOC(programName, varName) \
        sShader##programName.varName = \
        glGetAttribLocation(sShader##programName.program, #varName); \
        if (sShader##programName.varName == -1) rt = 0
#define GET_UNIFORM_LOC(programName, varName) \
        sShader##programName.varName = \
        glGetUniformLocation(sShader##programName.program, #varName); \
        if (sShader##programName.varName == -1) rt = 0
    GET_ATTRIBUTE_LOC(Lit, pos);
    GET_ATTRIBUTE_LOC(Lit, normal);
    GET_ATTRIBUTE_LOC(Lit, colorIn);
    GET_UNIFORM_LOC(Lit, mvp);
    GET_UNIFORM_LOC(Lit, normalMatrix);
    GET_UNIFORM_LOC(Lit, ambient);
    GET_UNIFORM_LOC(Lit, shininess);
    GET_UNIFORM_LOC(Lit, light_0_direction);
    GET_UNIFORM_LOC(Lit, light_0_diffuse);
    GET_UNIFORM_LOC(Lit, light_0_specular);
    GET_UNIFORM_LOC(Lit, light_1_direction);
    GET_UNIFORM_LOC(Lit, light_1_diffuse);
    GET_UNIFORM_LOC(Lit, light_2_direction);
    GET_UNIFORM_LOC(Lit, light_2_diffuse);

    GET_ATTRIBUTE_LOC(Flat, pos);
    GET_ATTRIBUTE_LOC(Flat, colorIn);
    GET_UNIFORM_LOC(Flat, mvp);

    GET_ATTRIBUTE_LOC(Fade, pos);
    GET_UNIFORM_LOC(Fade, minFade);
#undef GET_ATTRIBUTE_LOC
#undef GET_UNIFORM_LOC
    return rt;
}


KDint initShaderPrograms()
{
    Matrix4x4_LoadIdentity(sModelView);
    Matrix4x4_LoadIdentity(sProjection);

    sShaderFlat.program = createProgram(sFlatVertexSource,
                                        sFlatFragmentSource);
    sShaderLit.program = createProgram(sLitVertexSource,
                                       sFlatFragmentSource);
    sShaderFade.program = createProgram(sFadeVertexSource,
                                        sFlatFragmentSource);
    if (sShaderFlat.program == 0 || sShaderLit.program == 0 ||
        sShaderFade.program == 0)
        return 0;

    return getLocations();
}


void deInitShaderPrograms()
{
    glDeleteProgram(sShaderFlat.program);
    glDeleteProgram(sShaderLit.program);
    glDeleteProgram(sShaderFade.program);
}


void bindShaderProgram(GLuint program)
{
    KDint loc_mvp = -1;
    KDint loc_normalMatrix = -1;

    glUseProgram(program);

    if (program == sShaderLit.program)
    {
        loc_mvp = sShaderLit.mvp;
        loc_normalMatrix = sShaderLit.normalMatrix;
    }
    else if (program == sShaderFlat.program)
    {
        loc_mvp = sShaderFlat.mvp;
    }

    if (loc_mvp != -1)
    {
        Matrix4x4 mvp;
        Matrix4x4_Multiply(mvp, sModelView, sProjection);
        glUniformMatrix4fv(loc_mvp, 1, GL_FALSE, (GLfloat *)mvp);
    }
    if (loc_normalMatrix != -1)
    {
        Matrix3x3 normalMatrix;
        computeNormalMatrix(sModelView, normalMatrix);
        glUniformMatrix3fv(loc_normalMatrix, 1, GL_FALSE,
                           (GLfloat *)normalMatrix);
    }
}

