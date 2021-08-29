/******************************************************************************
 * San Angeles Observation OpenGL ES version example
 * Copyright (c) 2004-2005, Jetro Lauha
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in
 *       the documentation and/or other materials provided with the
 *       distribution.
 *     * Neither the name of the software product's copyright owner nor
 *       the names of its contributors may be used to endorse or promote
 *       products derived from this software without specific prior written
 *       permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
 * TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 ******************************************************************************/

#define EXAMPLE_COMMON_IMPLEMENTATION
#include "example_common.h"

#include "data/cams.h"
#include "data/shaders.h"
#if !defined(__EMSCRIPTEN__)
#define SUPERSHAPE_HIGH_RES
#endif
#include "data/shapes.h"


// Total run length is 20 * camera track base unit length (see cams.h).
#define RUN_LENGTH (20 * CAMTRACK_LEN)

static KDuint64 sRandomSeed = 0;

static void seedRandom(KDuint64 seed)
{
    sRandomSeed = seed;
}

static KDuint64 randomUInt()
{
    sRandomSeed = sRandomSeed * 0x343fd + 0x269ec3;
    return sRandomSeed >> 16;
}

// Definition of one GL object in this demo.
typedef struct {
    /* Vertex array and color array are enabled for all objects, so their
     * pointers must always be valid and non-NULL. Normal array is not
     * used by the ground plane, so when its pointer is NULL then normal
     * array usage is disabled.
     *
     * Vertex array is supposed to use GL_FIXED datatype and stride 0
     * (i.e. tightly packed array). Color array is supposed to have 4
     * components per color with GL_UNSIGNED_BYTE datatype and stride 0.
     * Normal array is supposed to use GL_FIXED datatype and stride 0.
     */
    GLfloat *vertexArray;
    GLint vertexArraySize;
    GLintptr vertexArrayOffset;
    GLubyte *colorArray;
    GLint colorArraySize;
    GLintptr colorArrayOffset;
    GLfloat *normalArray;
    GLint normalArraySize;
    GLintptr normalArrayOffset;
    GLint vertexComponents;
    GLsizei count;
    GLuint shaderProgram;
} GLOBJECT;

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

static SHADERLIT sShaderLit;
static SHADERFLAT sShaderFlat;
static SHADERFADE sShaderFade;

static Matrix4x4 sModelView;
static Matrix4x4 sProjection;

static KDust sStartTick = 0;
static KDust sTick = 0;

static KDint sCurrentCamTrack = 0;
static KDust sCurrentCamTrackStartTick = 0;
static KDust sNextCamTrackStartTick = 0x7fffffff;

static GLOBJECT *sSuperShapeObjects[SUPERSHAPE_COUNT] = {KD_NULL};
static GLOBJECT *sGroundPlane = KD_NULL;
static GLOBJECT *sFadeQuad = KD_NULL;

static GLuint sVBO = 0;

typedef struct {
    KDfloat32 x, y, z;
} VECTOR3;


static void freeGLObject(GLOBJECT *object)
{
    if(object == KD_NULL)
        return;

    kdFree(object->normalArray);
    kdFree(object->colorArray);
    kdFree(object->vertexArray);

    kdFree(object);
}


static GLOBJECT *newGLObject(KDint64 vertices, KDint vertexComponents,
    KDint useColorArray, KDint useNormalArray)
{
    GLOBJECT *result;
    result = (GLOBJECT *)kdMalloc(sizeof(GLOBJECT));
    if(result == KD_NULL)
        return KD_NULL;
    result->count = (GLsizei)vertices;
    result->vertexComponents = vertexComponents;
    result->vertexArraySize = (GLint)(vertices * vertexComponents * sizeof(GLfloat));
    result->vertexArray = kdMalloc(result->vertexArraySize);
    result->vertexArrayOffset = 0;
    if(useColorArray)
    {
        result->colorArraySize = (GLint)(vertices * 4 * sizeof(GLubyte));
        result->colorArray = kdMalloc(result->colorArraySize);
    }
    else
    {
        result->colorArraySize = 0;
        result->colorArray = KD_NULL;
    }
    result->colorArrayOffset = result->vertexArrayOffset +
        result->vertexArraySize;
    if(useNormalArray)
    {
        result->normalArraySize = (GLint)(vertices * 3 * sizeof(GLfloat));
        result->normalArray = kdMalloc(result->normalArraySize);
    }
    else
    {
        result->normalArraySize = 0;
        result->normalArray = KD_NULL;
    }
    result->normalArrayOffset = result->colorArrayOffset +
        result->colorArraySize;
    if(result->vertexArray == KD_NULL ||
        (useColorArray && result->colorArray == KD_NULL) ||
        (useNormalArray && result->normalArray == KD_NULL))
    {
        freeGLObject(result);
        return KD_NULL;
    }
    result->shaderProgram = 0;
    return result;
}


static void appendObjectVBO(GLOBJECT *object, GLint *offset)
{
    kdAssert(object != KD_NULL);

    object->vertexArrayOffset += *offset;
    object->colorArrayOffset += *offset;
    object->normalArrayOffset += *offset;
    *offset += object->vertexArraySize + object->colorArraySize +
        object->normalArraySize;

    glBufferSubData(GL_ARRAY_BUFFER, object->vertexArrayOffset,
        object->vertexArraySize, object->vertexArray);
    if(object->colorArray)
        glBufferSubData(GL_ARRAY_BUFFER, object->colorArrayOffset,
            object->colorArraySize, object->colorArray);
    if(object->normalArray)
        glBufferSubData(GL_ARRAY_BUFFER, object->normalArrayOffset,
            object->normalArraySize, object->normalArray);

    kdFree(object->normalArray);
    object->normalArray = KD_NULL;
    kdFree(object->colorArray);
    object->colorArray = KD_NULL;
    kdFree(object->vertexArray);
    object->vertexArray = KD_NULL;
}


static GLuint createVBO(GLOBJECT **superShapes, KDint superShapeCount,
    GLOBJECT *groundPlane, GLOBJECT *fadeQuad)
{
    GLuint vbo;
    GLint totalSize = 0;
    KDint a;
    for(a = 0; a < superShapeCount; ++a)
    {
        kdAssert(superShapes[a] != KD_NULL);
        totalSize += superShapes[a]->vertexArraySize +
            superShapes[a]->colorArraySize +
            superShapes[a]->normalArraySize;
    }
    totalSize += groundPlane->vertexArraySize +
        groundPlane->colorArraySize +
        groundPlane->normalArraySize;
    totalSize += fadeQuad->vertexArraySize +
        fadeQuad->colorArraySize +
        fadeQuad->normalArraySize;
    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, totalSize, 0, GL_DYNAMIC_DRAW);
    GLint offset = 0;
    for(a = 0; a < superShapeCount; ++a)
        appendObjectVBO(superShapes[a], &offset);
    appendObjectVBO(groundPlane, &offset);
    appendObjectVBO(fadeQuad, &offset);
    kdAssert(offset == totalSize);
    return vbo;
}


static void computeNormalMatrix(Matrix4x4 m, Matrix3x3 normal)
{
    KDfloat32 det = m[0 * 4 + 0] * (m[1 * 4 + 1] * m[2 * 4 + 2] - m[2 * 4 + 1] * m[1 * 4 + 2]) -
        m[0 * 4 + 1] * (m[1 * 4 + 0] * m[2 * 4 + 2] - m[1 * 4 + 2] * m[2 * 4 + 0]) +
        m[0 * 4 + 2] * (m[1 * 4 + 0] * m[2 * 4 + 1] - m[1 * 4 + 1] * m[2 * 4 + 0]);
    KDfloat32 invDet = 1.f / det;
    normal[0 * 3 + 0] = invDet * (m[1 * 4 + 1] * m[2 * 4 + 2] - m[2 * 4 + 1] * m[1 * 4 + 2]);
    normal[1 * 3 + 0] = invDet * -(m[0 * 4 + 1] * m[2 * 4 + 2] - m[0 * 4 + 2] * m[2 * 4 + 1]);
    normal[2 * 3 + 0] = invDet * (m[0 * 4 + 1] * m[1 * 4 + 2] - m[0 * 4 + 2] * m[1 * 4 + 1]);
    normal[0 * 3 + 1] = invDet * -(m[1 * 4 + 0] * m[2 * 4 + 2] - m[1 * 4 + 2] * m[2 * 4 + 0]);
    normal[1 * 3 + 1] = invDet * (m[0 * 4 + 0] * m[2 * 4 + 2] - m[0 * 4 + 2] * m[2 * 4 + 0]);
    normal[2 * 3 + 1] = invDet * -(m[0 * 4 + 0] * m[1 * 4 + 2] - m[1 * 4 + 0] * m[0 * 4 + 2]);
    normal[0 * 3 + 2] = invDet * (m[1 * 4 + 0] * m[2 * 4 + 1] - m[2 * 4 + 0] * m[1 * 4 + 1]);
    normal[1 * 3 + 2] = invDet * -(m[0 * 4 + 0] * m[2 * 4 + 1] - m[2 * 4 + 0] * m[0 * 4 + 1]);
    normal[2 * 3 + 2] = invDet * (m[0 * 4 + 0] * m[1 * 4 + 1] - m[1 * 4 + 0] * m[0 * 4 + 1]);
}


static KDint getLocations()
{
    KDint rt = 1;
#define GET_ATTRIBUTE_LOC(programName, varName)                      \
    sShader##programName.varName =                                   \
        glGetAttribLocation(sShader##programName.program, #varName); \
    if(sShader##programName.varName == -1)                           \
    rt = 0
#define GET_UNIFORM_LOC(programName, varName)                         \
    sShader##programName.varName =                                    \
        glGetUniformLocation(sShader##programName.program, #varName); \
    if(sShader##programName.varName == -1)                            \
    rt = 0
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
    exampleMatrixIdentity(sModelView);
    exampleMatrixIdentity(sProjection);

    sShaderFlat.program = exampleCreateProgram(sFlatVertexSource, sFlatFragmentSource, KD_TRUE);
    sShaderLit.program = exampleCreateProgram(sLitVertexSource, sFlatFragmentSource, KD_TRUE);
    sShaderFade.program = exampleCreateProgram(sFadeVertexSource, sFlatFragmentSource, KD_TRUE);
    if(sShaderFlat.program == 0 || sShaderLit.program == 0 || sShaderFade.program == 0)
    {
        return 0;
    }

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

    if(program == sShaderLit.program)
    {
        loc_mvp = sShaderLit.mvp;
        loc_normalMatrix = sShaderLit.normalMatrix;
    }
    else if(program == sShaderFlat.program)
    {
        loc_mvp = sShaderFlat.mvp;
    }

    if(loc_mvp != -1)
    {
        Matrix4x4 mvp;
        kdMemcpy(mvp, sProjection, sizeof(sProjection));
        exampleMatrixMultiply(mvp, sModelView);
        glUniformMatrix4fv(loc_mvp, 1, GL_FALSE, (GLfloat *)mvp);
    }
    if(loc_normalMatrix != -1)
    {
        Matrix3x3 normalMatrix;
        computeNormalMatrix(sModelView, normalMatrix);
        glUniformMatrix3fv(loc_normalMatrix, 1, GL_FALSE,
            (GLfloat *)normalMatrix);
    }
}

static void drawGLObject(GLOBJECT *object)
{
    int loc_pos = -1;
    int loc_colorIn = -1;
    int loc_normal = -1;

    bindShaderProgram(object->shaderProgram);
    if(object->shaderProgram == sShaderLit.program)
    {
        loc_pos = sShaderLit.pos;
        loc_colorIn = sShaderLit.colorIn;
        loc_normal = sShaderLit.normal;
    }
    else if(object->shaderProgram == sShaderFlat.program)
    {
        loc_pos = sShaderFlat.pos;
        loc_colorIn = sShaderFlat.colorIn;
    }
    else
    {
        kdAssert(0);
    }
    glVertexAttribPointer(loc_pos, object->vertexComponents, GL_FLOAT,
        GL_FALSE, 0, (GLvoid *)object->vertexArrayOffset);
    glEnableVertexAttribArray(loc_pos);
    glVertexAttribPointer(loc_colorIn, 4, GL_UNSIGNED_BYTE, GL_TRUE, 0,
        (GLvoid *)object->colorArrayOffset);
    glEnableVertexAttribArray(loc_colorIn);
    if(object->normalArraySize > 0)
    {
        glVertexAttribPointer(loc_normal, 3, GL_FLOAT, GL_FALSE, 0,
            (GLvoid *)object->normalArrayOffset);
        glEnableVertexAttribArray(loc_normal);
    }
    glDrawArrays(GL_TRIANGLES, 0, object->count);

    if(object->normalArraySize > 0)
        glDisableVertexAttribArray(loc_normal);
    glDisableVertexAttribArray(loc_colorIn);
    glDisableVertexAttribArray(loc_pos);
}


static void vector3Sub(VECTOR3 *dest, VECTOR3 *v1, VECTOR3 *v2)
{
    dest->x = v1->x - v2->x;
    dest->y = v1->y - v2->y;
    dest->z = v1->z - v2->z;
}


static void superShapeMap(VECTOR3 *point, KDfloat32 r1, KDfloat32 r2, KDfloat32 t, KDfloat32 p)
{
    // sphere-mapping of supershape parameters
    point->x = (kdCosf(t) * kdCosf(p) / r1 / r2);
    point->y = (kdSinf(t) * kdCosf(p) / r1 / r2);
    point->z = (kdSinf(p) / r2);
}


static KDfloat32 ssFunc(const KDfloat32 t, const KDfloat32 *p)
{
    return (kdPowf(kdPowf(kdFabsf(kdCosf(p[0] * t / 4)) / p[1], p[4]) +
            kdPowf(kdFabsf(kdSinf(p[0] * t / 4)) / p[2], p[5]),
        1 / p[3]));
}


// Creates and returns a supershape object.
// Based on Paul Bourke's POV-Ray implementation.
// http://astronomy.swin.edu.au/~pbourke/povray/supershape/
static GLOBJECT *createSuperShape(const KDfloat32 *params)
{
    const KDint resol1 = (KDint)params[SUPERSHAPE_PARAMS - 3];
    const KDint resol2 = (KDint)params[SUPERSHAPE_PARAMS - 2];
    // latitude 0 to pi/2 for no mirrored bottom
    // (latitudeBegin==0 for -pi/2 to pi/2 originally)
    const KDint latitudeBegin = resol2 / 4;
    const KDint latitudeEnd = resol2 / 2;  // non-inclusive
    const KDint longitudeCount = resol1;
    const KDint latitudeCount = latitudeEnd - latitudeBegin;
    const KDint64 triangleCount = longitudeCount * latitudeCount * 2;
    const KDint64 vertices = triangleCount * 3;
    GLOBJECT *result;
    KDfloat32 baseColor[3];
    KDint a, longitude, latitude;
    KDint64 currentVertex, currentQuad;

    result = newGLObject(vertices, 3, 1, 1);
    if(result == KD_NULL)
        return KD_NULL;

    for(a = 0; a < 3; ++a)
        baseColor[a] = ((randomUInt() % 155) + 100) / 255.f;

    currentQuad = 0;
    currentVertex = 0;

    // longitude -pi to pi
    for(longitude = 0; longitude < longitudeCount; ++longitude)
    {

        // latitude 0 to pi/2
        for(latitude = latitudeBegin; latitude < latitudeEnd; ++latitude)
        {
            KDfloat32 t1 = -KD_PI_F + longitude * 2 * KD_PI_F / resol1;
            KDfloat32 t2 = -KD_PI_F + (longitude + 1) * 2 * KD_PI_F / resol1;
            KDfloat32 p1 = -KD_PI_F / 2 + latitude * 2 * KD_PI_F / resol2;
            KDfloat32 p2 = -KD_PI_F / 2 + (latitude + 1) * 2 * KD_PI_F / resol2;
            KDfloat32 r0, r1, r2, r3;

            r0 = ssFunc(t1, params);
            r1 = ssFunc(p1, &params[6]);
            r2 = ssFunc(t2, params);
            r3 = ssFunc(p2, &params[6]);

            if(r0 != 0 && r1 != 0 && r2 != 0 && r3 != 0)
            {
                VECTOR3 pa, pb, pc, pd;
                VECTOR3 v1, v2, n;
                KDfloat32 ca;
                KDint64 i;
                //float lenSq, invLenSq;

                superShapeMap(&pa, r0, r1, t1, p1);
                superShapeMap(&pb, r2, r1, t2, p1);
                superShapeMap(&pc, r2, r3, t2, p2);
                superShapeMap(&pd, r0, r3, t1, p2);

                // kludge to set lower edge of the object to fixed level
                if(latitude == latitudeBegin + 1)
                    pa.z = pb.z = 0;

                vector3Sub(&v1, &pb, &pa);
                vector3Sub(&v2, &pd, &pa);

                // Calculate normal with cross product.
                /*   i    j    k      i    j
                 * v1.x v1.y v1.z | v1.x v1.y
                 * v2.x v2.y v2.z | v2.x v2.y
                 */

                n.x = v1.y * v2.z - v1.z * v2.y;
                n.y = v1.z * v2.x - v1.x * v2.z;
                n.z = v1.x * v2.y - v1.y * v2.x;

                /* Pre-normalization of the normals is disabled here because
                 * they will be normalized anyway later due to automatic
                 * normalization (GL_NORMALIZE). It is enabled because the
                 * objects are scaled with glScale.
                 */
                /*
                lenSq = n.x * n.x + n.y * n.y + n.z * n.z;
                invLenSq = (float)(1 / sqrt(lenSq));
                n.x *= invLenSq;
                n.y *= invLenSq;
                n.z *= invLenSq;
                */

                ca = pa.z + 0.5f;

                for(i = currentVertex * 3;
                    i < (currentVertex + 6) * 3;
                    i += 3)
                {
                    result->normalArray[i] = n.x;
                    result->normalArray[i + 1] = n.y;
                    result->normalArray[i + 2] = n.z;
                }
                for(i = currentVertex * 4;
                    i < (currentVertex + 6) * 4;
                    i += 4)
                {
                    KDint color[3];
                    for(KDint a = 0; a < 3; ++a)
                    {
                        color[a] = (KDint)(ca * baseColor[a] * 255);
                        if(color[a] > 255)
                            color[a] = 255;
                    }
                    result->colorArray[i] = (GLubyte)color[0];
                    result->colorArray[i + 1] = (GLubyte)color[1];
                    result->colorArray[i + 2] = (GLubyte)color[2];
                    result->colorArray[i + 3] = 0;
                }
                result->vertexArray[currentVertex * 3] = pa.x;
                result->vertexArray[currentVertex * 3 + 1] = pa.y;
                result->vertexArray[currentVertex * 3 + 2] = pa.z;
                ++currentVertex;
                result->vertexArray[currentVertex * 3] = pb.x;
                result->vertexArray[currentVertex * 3 + 1] = pb.y;
                result->vertexArray[currentVertex * 3 + 2] = pb.z;
                ++currentVertex;
                result->vertexArray[currentVertex * 3] = pd.x;
                result->vertexArray[currentVertex * 3 + 1] = pd.y;
                result->vertexArray[currentVertex * 3 + 2] = pd.z;
                ++currentVertex;
                result->vertexArray[currentVertex * 3] = pb.x;
                result->vertexArray[currentVertex * 3 + 1] = pb.y;
                result->vertexArray[currentVertex * 3 + 2] = pb.z;
                ++currentVertex;
                result->vertexArray[currentVertex * 3] = pc.x;
                result->vertexArray[currentVertex * 3 + 1] = pc.y;
                result->vertexArray[currentVertex * 3 + 2] = pc.z;
                ++currentVertex;
                result->vertexArray[currentVertex * 3] = pd.x;
                result->vertexArray[currentVertex * 3 + 1] = pd.y;
                result->vertexArray[currentVertex * 3 + 2] = pd.z;
                ++currentVertex;
            }  // r0 && r1 && r2 && r3
            ++currentQuad;
        }  // latitude
    }      // longitude

    // Set number of vertices in object to the actual amount created.
    result->count = (GLsizei)currentVertex;
    result->shaderProgram = sShaderLit.program;
    return result;
}


static GLOBJECT *createGroundPlane()
{
    const KDint scale = 4;
    const KDint yBegin = -15, yEnd = 15;  // ends are non-inclusive
    const KDint xBegin = -15, xEnd = 15;
    const KDint64 triangleCount = (yEnd - yBegin) * (xEnd - xBegin) * 2;
    const KDint64 vertices = triangleCount * 3;
    GLOBJECT *result;
    KDint x, y;
    KDint64 currentVertex, currentQuad;

    result = newGLObject(vertices, 2, 1, 0);
    if(result == KD_NULL)
        return KD_NULL;

    currentQuad = 0;
    currentVertex = 0;

    for(y = yBegin; y < yEnd; ++y)
    {
        for(x = xBegin; x < xEnd; ++x)
        {
            GLubyte color;
            color = (GLubyte)((randomUInt() & 0x5f) + 81);  // 101 1111
            for(KDint64 i = currentVertex * 4; i < (currentVertex + 6) * 4; i += 4)
            {
                result->colorArray[i] = color;
                result->colorArray[i + 1] = color;
                result->colorArray[i + 2] = color;
                result->colorArray[i + 3] = 0;
            }

            // Axis bits for quad triangles:
            // x: 011100 (0x1c), y: 110001 (0x31)  (clockwise)
            // x: 001110 (0x0e), y: 100011 (0x23)  (counter-clockwise)
            for(KDint a = 0; a < 6; ++a)
            {
                const KDint xm = x + ((0x1c >> a) & 1);
                const KDint ym = y + ((0x31 >> a) & 1);
                const KDfloat32 m =(kdCosf((KDfloat32)(xm * 2)) * kdSinf((KDfloat32)(ym * 4)) * 0.75f);
                result->vertexArray[currentVertex * 2] = xm * scale + m;
                result->vertexArray[currentVertex * 2 + 1] = ym * scale + m;
                ++currentVertex;
            }
            ++currentQuad;
        }
    }
    result->shaderProgram = sShaderFlat.program;
    return result;
}


static void drawGroundPlane()
{
    glDisable(GL_CULL_FACE);
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_ZERO, GL_SRC_COLOR);

    drawGLObject(sGroundPlane);

    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
}


static GLOBJECT *createFadeQuad()
{
    static const GLfloat quadVertices[] = {
        -1, -1,
        1, -1,
        -1, 1,
        1, -1,
        1, 1,
        -1, 1};

    GLOBJECT *result;
    KDint i;

    result = newGLObject(6, 2, 0, 0);
    if(result == KD_NULL)
        return KD_NULL;

    for(i = 0; i < 12; ++i)
        result->vertexArray[i] = quadVertices[i];

    result->shaderProgram = sShaderFade.program;
    return result;
}


static void drawFadeQuad()
{
    const KDust beginFade = sTick - sCurrentCamTrackStartTick;
    const KDust endFade = sNextCamTrackStartTick - sTick;
    const KDust minFade = beginFade < endFade ? beginFade : endFade;

    if(minFade < 1024)
    {
        const GLfloat fadeColor = minFade / 1024.f;
        glDisable(GL_DEPTH_TEST);
        glEnable(GL_BLEND);
        glBlendFunc(GL_ZERO, GL_SRC_COLOR);
        bindShaderProgram(sShaderFade.program);
        glUniform1f(sShaderFade.minFade, fadeColor);
        glVertexAttribPointer(sShaderFade.pos, 2, GL_FLOAT, GL_FALSE, 0,
            (GLvoid *)sFadeQuad->vertexArrayOffset);
        glEnableVertexAttribArray(sShaderFade.pos);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        glDisableVertexAttribArray(sShaderFade.pos);

        glDisable(GL_BLEND);
        glEnable(GL_DEPTH_TEST);
    }
}


// Called from the app framework.
int appInit()
{
    KDint a;
    static GLfloat light0Diffuse[] = {1.f, 0.4f, 0, 1.f};
    static GLfloat light1Diffuse[] = {0.07f, 0.14f, 0.35f, 1.f};
    static GLfloat light2Diffuse[] = {0.07f, 0.17f, 0.14f, 1.f};
    static GLfloat materialSpecular[] = {1.f, 1.f, 1.f, 1.f};
    static GLfloat lightAmbient[] = {0.2f, 0.2f, 0.2f, 1.f};

    glDisable(GL_CULL_FACE);
    glEnable(GL_DEPTH_TEST);
    if(initShaderPrograms() == 0)
    {
        kdLogMessage("Error: initShaderPrograms failed\n");
        return 0;
    }
    seedRandom(15);

    for(a = 0; a < SUPERSHAPE_COUNT; ++a)
    {
        sSuperShapeObjects[a] = createSuperShape(sSuperShapeParams[a]);
        kdAssert(sSuperShapeObjects[a] != KD_NULL);
    }
    sGroundPlane = createGroundPlane();
    kdAssert(sGroundPlane != KD_NULL);
    sFadeQuad = createFadeQuad();
    kdAssert(sFadeQuad != KD_NULL);
    sVBO = createVBO(sSuperShapeObjects, SUPERSHAPE_COUNT,
        sGroundPlane, sFadeQuad);

    // setup non-changing lighting parameters
    bindShaderProgram(sShaderLit.program);
    glUniform4fv(sShaderLit.ambient, 1, lightAmbient);
    glUniform4fv(sShaderLit.light_0_diffuse, 1, light0Diffuse);
    glUniform4fv(sShaderLit.light_1_diffuse, 1, light1Diffuse);
    glUniform4fv(sShaderLit.light_2_diffuse, 1, light2Diffuse);
    glUniform4fv(sShaderLit.light_0_specular, 1, materialSpecular);
    glUniform1f(sShaderLit.shininess, 60.f);
    return 1;
}


// Called from the app framework.
void appDeinit()
{
    KDint a;
    for(a = 0; a < SUPERSHAPE_COUNT; ++a)
        freeGLObject(sSuperShapeObjects[a]);
    freeGLObject(sGroundPlane);
    freeGLObject(sFadeQuad);
    glDeleteBuffers(1, &sVBO);
    deInitShaderPrograms();
}


static void prepareFrame(KDint width, KDint height)
{
    glViewport(0, 0, width, height);

    glClearColor(0.1f, 0.2f, 0.3f, 1.0f);
    glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);

    exampleMatrixIdentity(sProjection);
    exampleMatrixPerspective(sProjection, 45.0f, (KDfloat32)width / height, 0.5f, 150.0f);

    exampleMatrixIdentity(sModelView);
}


static void configureLightAndMaterial()
{
    GLfloat light0Position[] = {-4.f, 1.f, 1.f, 0};
    GLfloat light1Position[] = {1.f, -2.f, -1.f, 0};
    GLfloat light2Position[] = {-1.f, 0, -4.f, 0};

    exampleMatrixTransform(sModelView,
        light0Position, light0Position + 1, light0Position + 2);
    exampleMatrixTransform(sModelView,
        light1Position, light1Position + 1, light1Position + 2);
    exampleMatrixTransform(sModelView,
        light2Position, light2Position + 1, light2Position + 2);

    bindShaderProgram(sShaderLit.program);
    glUniform3fv(sShaderLit.light_0_direction, 1, light0Position);
    glUniform3fv(sShaderLit.light_1_direction, 1, light1Position);
    glUniform3fv(sShaderLit.light_2_direction, 1, light2Position);
}


static void drawModels(KDfloat32 zScale)
{
    const KDint translationScale = 9;
    KDint x, y;

    seedRandom(9);

    exampleMatrixScale(sModelView, 1.f, 1.f, zScale);

    for(y = -5; y <= 5; ++y)
    {
        for(x = -5; x <= 5; ++x)
        {
            KDfloat32 buildingScale;
            Matrix4x4 tmp;

            KDint curShape = randomUInt() % SUPERSHAPE_COUNT;
            buildingScale = sSuperShapeParams[curShape][SUPERSHAPE_PARAMS - 1];
            kdMemcpy(tmp, sModelView, sizeof(sModelView));
            exampleMatrixTranslate(sModelView, (KDfloat32)(x * translationScale), (KDfloat32)(y * translationScale), 0);
            exampleMatrixRotate(sModelView, (KDfloat32)(randomUInt() % 360), 0, 0, 1.f);
            exampleMatrixScale(sModelView, buildingScale, buildingScale, buildingScale);

            drawGLObject(sSuperShapeObjects[curShape]);
            kdMemcpy(sModelView, tmp, sizeof(tmp));
        }
    }

    for(x = -2; x <= 2; ++x)
    {
        const KDint shipScale100 = translationScale * 500;
        const KDint offs100 = x * shipScale100 + (sTick % shipScale100);
        KDfloat32 offs = offs100 * 0.01f;
        Matrix4x4 tmp;
        kdMemcpy(tmp, sModelView, sizeof(sModelView));
        exampleMatrixTranslate(sModelView, offs, -4.f, 2.f);
        drawGLObject(sSuperShapeObjects[SUPERSHAPE_COUNT - 1]);
        kdMemcpy(sModelView, tmp, sizeof(tmp));
        exampleMatrixTranslate(sModelView, -4.f, offs, 4.f);
        exampleMatrixRotate(sModelView, 90.f, 0, 0, 1.f);
        drawGLObject(sSuperShapeObjects[SUPERSHAPE_COUNT - 1]);
        kdMemcpy(sModelView, tmp, sizeof(tmp));
    }
}

/* Following gluLookAt implementation is adapted from the
 * Mesa 3D Graphics library. http://www.mesa3d.org
 */
static void gluLookAt(GLfloat eyex, GLfloat eyey, GLfloat eyez,
    GLfloat centerx, GLfloat centery, GLfloat centerz,
    GLfloat upx, GLfloat upy, GLfloat upz)
{
    Matrix4x4 m;
    GLfloat x[3], y[3], z[3];
    GLfloat mag;

    /* Make rotation matrix */

    /* Z vector */
    z[0] = eyex - centerx;
    z[1] = eyey - centery;
    z[2] = eyez - centerz;
    mag = kdSqrtf(z[0] * z[0] + z[1] * z[1] + z[2] * z[2]);
    if(mag)
    { /* mpichler, 19950515 */
        z[0] /= mag;
        z[1] /= mag;
        z[2] /= mag;
    }

    /* Y vector */
    y[0] = upx;
    y[1] = upy;
    y[2] = upz;

    /* X vector = Y cross Z */
    x[0] = y[1] * z[2] - y[2] * z[1];
    x[1] = -y[0] * z[2] + y[2] * z[0];
    x[2] = y[0] * z[1] - y[1] * z[0];

    /* Recompute Y = Z cross X */
    y[0] = z[1] * x[2] - z[2] * x[1];
    y[1] = -z[0] * x[2] + z[2] * x[0];
    y[2] = z[0] * x[1] - z[1] * x[0];

    /* mpichler, 19950515 */
    /* cross product gives area of parallelogram, which is < 1.0 for
     * non-perpendicular unit-length vectors; so normalize x, y here
     */

    mag = kdSqrtf(x[0] * x[0] + x[1] * x[1] + x[2] * x[2]);
    if(mag)
    {
        x[0] /= mag;
        x[1] /= mag;
        x[2] /= mag;
    }

    mag = kdSqrtf(y[0] * y[0] + y[1] * y[1] + y[2] * y[2]);
    if(mag)
    {
        y[0] /= mag;
        y[1] /= mag;
        y[2] /= mag;
    }

#define M(row, col) m[col * 4 + row]
    M(0, 0) = x[0];
    M(0, 1) = x[1];
    M(0, 2) = x[2];
    M(0, 3) = 0.0;
    M(1, 0) = y[0];
    M(1, 1) = y[1];
    M(1, 2) = y[2];
    M(1, 3) = 0.0;
    M(2, 0) = z[0];
    M(2, 1) = z[1];
    M(2, 2) = z[2];
    M(2, 3) = 0.0;
    M(3, 0) = 0.0;
    M(3, 1) = 0.0;
    M(3, 2) = 0.0;
    M(3, 3) = 1.0;
#undef M

    exampleMatrixMultiply(sModelView, m);

    exampleMatrixTranslate(sModelView, -eyex, -eyey, -eyez);
}

static void camTrack()
{
    KDfloat32 lerp[5];
    KDfloat32 eX, eY, eZ, cX, cY, cZ;
    KDfloat32 trackPos;
    CAMTRACK *cam;
    KDust currentCamTick;
    KDint a;

    if(sNextCamTrackStartTick <= sTick)
    {
        ++sCurrentCamTrack;
        sCurrentCamTrackStartTick = sNextCamTrackStartTick;
    }
    sNextCamTrackStartTick = sCurrentCamTrackStartTick +
        sCamTracks[sCurrentCamTrack].len * CAMTRACK_LEN;

    cam = &sCamTracks[sCurrentCamTrack];
    currentCamTick = sTick - sCurrentCamTrackStartTick;
    trackPos = (KDfloat32)currentCamTick / (CAMTRACK_LEN * cam->len);

    for(a = 0; a < 5; ++a)
        lerp[a] = (cam->src[a] + cam->dest[a] * trackPos) * 0.01f;

    if(cam->dist)
    {
        KDfloat32 dist = cam->dist * 0.1f;
        cX = lerp[0];
        cY = lerp[1];
        cZ = lerp[2];
        eX = cX - kdCosf(lerp[3]) * dist;
        eY = cY - kdSinf(lerp[3]) * dist;
        eZ = cZ - lerp[4];
    }
    else
    {
        eX = lerp[0];
        eY = lerp[1];
        eZ = lerp[2];
        cX = eX + kdCosf(lerp[3]);
        cY = eY + kdSinf(lerp[3]);
        cZ = eZ + lerp[4];
    }
    gluLookAt(eX, eY, eZ, cX, cY, cZ, 0, 0, 1);
}

// Called from the app framework.
/* The tick is current time in milliseconds, width and height
 * are the image dimensions to be rendered.
 */
void appRender(Example *example, KDust tick, KDint width, KDint height)
{
    Matrix4x4 tmp;

    if(sStartTick == 0)
        sStartTick = tick;
    if(!example->run)
        return;

    // Actual tick value is "blurred" a little bit.
    sTick = (sTick + tick - sStartTick) >> 1;

    // Terminate application after running through the demonstration once.
    if(sTick >= RUN_LENGTH)
    {
        example->run = KD_FALSE;
        return;
    }

    // Prepare OpenGL ES for rendering of the frame.
    prepareFrame(width, height);

    // Update the camera position and set the lookat.
    camTrack();

    // Configure environment.
    configureLightAndMaterial();

    // Draw the reflection by drawing models with negated Z-axis.

    kdMemcpy(tmp, sModelView, sizeof(sModelView));
    drawModels(-1);
    kdMemcpy(sModelView, tmp, sizeof(tmp));

    // Blend the ground plane to the window.
    drawGroundPlane();

    // Draw all the models normally.
    drawModels(1);

    // Draw fade quad over whole window (when changing cameras).
    drawFadeQuad();
}

KDint KD_APIENTRY kdMain(KDint argc, const KDchar *const *argv)
{
    Example *example = exampleInit();
    exampleLoadAudio(example, "data/!Cube - San Angeles Observation.ogg");

    appInit();

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

        EGLint width = 0; 
        EGLint height = 0;
        eglQuerySurface(example->egl.display, example->egl.surface, EGL_WIDTH, &width);
        eglQuerySurface(example->egl.display, example->egl.surface, EGL_HEIGHT, &height);
        appRender(example, kdGetTimeUST() / 1000000, width, height);
        exampleRun(example);
    }

    appDeinit();

    return exampleDestroy(example);
}
