/******************************************************************************
 * San Angeles Observation
 * Copyright 2004-2005 Jetro Lauha
 * All rights reserved.
 * Web: http://iki.fi/jetro/
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 ******************************************************************************/

#include <KD/kd.h>
#include <EGL/egl.h>
#include <GLES/gl.h>

#include "angeles/shapes.h"
#include "angeles/cams.h"


// Total run length is 20 * camera track base unit length (see cams.h).
#define RUN_LENGTH  (20 * CAMTRACK_LEN)
#define RANDOM_UINT_MAX 65535


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


// Capped conversion from float to fixed.
static KDint64 floatToFixed(KDfloat32 value)
{
    if (value < -32768) value = -32768;
    if (value > 32767) value = 32767;
    return (KDint64)(value * 65536);
}

#define FIXED(value) floatToFixed(value)


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
    GLfixed *vertexArray;
    GLubyte *colorArray;
    GLfixed *normalArray;
    GLint vertexComponents;
    GLsizei count;
} GLOBJECT;


static KDust sStartTick = 0;
static KDust sTick = 0;

static KDint sCurrentCamTrack = 0;
static KDust sCurrentCamTrackStartTick = 0;
static KDust sNextCamTrackStartTick = 0x7fffffff;

static GLOBJECT *sSuperShapeObjects[SUPERSHAPE_COUNT] = { KD_NULL };
static GLOBJECT *sGroundPlane = KD_NULL;


typedef struct {
    KDfloat32 x, y, z;
} VECTOR3;


static void freeGLObject(GLOBJECT *object)
{
    if (object == KD_NULL)
        return;
    kdFree(object->normalArray);
    kdFree(object->colorArray);
    kdFree(object->vertexArray);
    kdFree(object);
}


static GLOBJECT * newGLObject(KDint64 vertices, KDint vertexComponents,
                              KDint useNormalArray)
{
    GLOBJECT *result;
    result = (GLOBJECT *)kdMalloc(sizeof(GLOBJECT));
    if (result == KD_NULL)
        return KD_NULL;
    result->count = vertices;
    result->vertexComponents = vertexComponents;
    result->vertexArray = (GLfixed *)kdMalloc(vertices * vertexComponents *
                                            sizeof(GLfixed));
    result->colorArray = (GLubyte *)kdMalloc(vertices * 4 * sizeof(GLubyte));
    if (useNormalArray)
    {
        result->normalArray = (GLfixed *)kdMalloc(vertices * 3 *
                                                sizeof(GLfixed));
    }
    else
        result->normalArray = KD_NULL;
    if (result->vertexArray == KD_NULL ||
        result->colorArray == KD_NULL ||
        (useNormalArray && result->normalArray == KD_NULL))
    {
        freeGLObject(result);
        return KD_NULL;
    }
    return result;
}


static void drawGLObject(GLOBJECT *object)
{
    kdAssert(object != KD_NULL);

    glVertexPointer(object->vertexComponents, GL_FIXED,
                    0, object->vertexArray);
    glColorPointer(4, GL_UNSIGNED_BYTE, 0, object->colorArray);

    // Already done in initialization:
    //glEnableClientState(GL_VERTEX_ARRAY);
    //glEnableClientState(GL_COLOR_ARRAY);

    if (object->normalArray)
    {
        glNormalPointer(GL_FIXED, 0, object->normalArray);
        glEnableClientState(GL_NORMAL_ARRAY);
    }
    else
        glDisableClientState(GL_NORMAL_ARRAY);
    glDrawArrays(GL_TRIANGLES, 0, object->count);
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
                       kdPowf(kdFabsf(kdSinf(p[0] * t / 4)) / p[2], p[5]), 1 / p[3]));
}


// Creates and returns a supershape object.
// Based on Paul Bourke's POV-Ray implementation.
// http://astronomy.swin.edu.au/~pbourke/povray/supershape/
static GLOBJECT * createSuperShape(const KDfloat32 *params)
{
    const KDint resol1 = (KDint)params[SUPERSHAPE_PARAMS - 3];
    const KDint resol2 = (KDint)params[SUPERSHAPE_PARAMS - 2];
    // latitude 0 to pi/2 for no mirrored bottom
    // (latitudeBegin==0 for -pi/2 to pi/2 originally)
    const KDint latitudeBegin = resol2 / 4;
    const KDint latitudeEnd = resol2 / 2;    // non-inclusive
    const KDint longitudeCount = resol1;
    const KDint latitudeCount = latitudeEnd - latitudeBegin;
    const KDint64 triangleCount = longitudeCount * latitudeCount * 2;
    const KDint64 vertices = triangleCount * 3;
    GLOBJECT *result;
    KDfloat32 baseColor[3];
    KDint a, longitude, latitude;
    KDint64 currentVertex, currentQuad;

    result = newGLObject(vertices, 3, 1);
    if (result == KD_NULL)
        return KD_NULL;

    for (a = 0; a < 3; ++a)
        baseColor[a] = ((randomUInt() % 155) + 100) / 255.f;

    currentQuad = 0;
    currentVertex = 0;

    // longitude -pi to pi
    for (longitude = 0; longitude < longitudeCount; ++longitude)
    {

        // latitude 0 to pi/2
        for (latitude = latitudeBegin; latitude < latitudeEnd; ++latitude)
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

            if (r0 != 0 && r1 != 0 && r2 != 0 && r3 != 0)
            {
                VECTOR3 pa, pb, pc, pd;
                VECTOR3 v1, v2, n;
                KDfloat32 ca;
                KDint i;
                //float lenSq, invLenSq;

                superShapeMap(&pa, r0, r1, t1, p1);
                superShapeMap(&pb, r2, r1, t2, p1);
                superShapeMap(&pc, r2, r3, t2, p2);
                superShapeMap(&pd, r0, r3, t1, p2);

                // kludge to set lower edge of the object to fixed level
                if (latitude == latitudeBegin + 1)
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

                for (i = currentVertex * 3;
                     i < (currentVertex + 6) * 3;
                     i += 3)
                {
                    result->normalArray[i] = FIXED(n.x);
                    result->normalArray[i + 1] = FIXED(n.y);
                    result->normalArray[i + 2] = FIXED(n.z);
                }
                for (i = currentVertex * 4;
                     i < (currentVertex + 6) * 4;
                     i += 4)
                {
                    KDint a, color[3];
                    for (a = 0; a < 3; ++a)
                    {
                        color[a] = (KDint)(ca * baseColor[a] * 255);
                        if (color[a] > 255) color[a] = 255;
                    }
                    result->colorArray[i] = (GLubyte)color[0];
                    result->colorArray[i + 1] = (GLubyte)color[1];
                    result->colorArray[i + 2] = (GLubyte)color[2];
                    result->colorArray[i + 3] = 0;
                }
                result->vertexArray[currentVertex * 3] = FIXED(pa.x);
                result->vertexArray[currentVertex * 3 + 1] = FIXED(pa.y);
                result->vertexArray[currentVertex * 3 + 2] = FIXED(pa.z);
                ++currentVertex;
                result->vertexArray[currentVertex * 3] = FIXED(pb.x);
                result->vertexArray[currentVertex * 3 + 1] = FIXED(pb.y);
                result->vertexArray[currentVertex * 3 + 2] = FIXED(pb.z);
                ++currentVertex;
                result->vertexArray[currentVertex * 3] = FIXED(pd.x);
                result->vertexArray[currentVertex * 3 + 1] = FIXED(pd.y);
                result->vertexArray[currentVertex * 3 + 2] = FIXED(pd.z);
                ++currentVertex;
                result->vertexArray[currentVertex * 3] = FIXED(pb.x);
                result->vertexArray[currentVertex * 3 + 1] = FIXED(pb.y);
                result->vertexArray[currentVertex * 3 + 2] = FIXED(pb.z);
                ++currentVertex;
                result->vertexArray[currentVertex * 3] = FIXED(pc.x);
                result->vertexArray[currentVertex * 3 + 1] = FIXED(pc.y);
                result->vertexArray[currentVertex * 3 + 2] = FIXED(pc.z);
                ++currentVertex;
                result->vertexArray[currentVertex * 3] = FIXED(pd.x);
                result->vertexArray[currentVertex * 3 + 1] = FIXED(pd.y);
                result->vertexArray[currentVertex * 3 + 2] = FIXED(pd.z);
                ++currentVertex;
            } // r0 && r1 && r2 && r3
            ++currentQuad;
        } // latitude
    } // longitude

    // Set number of vertices in object to the actual amount created.
    result->count = currentVertex;

    return result;
}


static GLOBJECT * createGroundPlane()
{
    const KDint scale = 4;
    const KDint yBegin = -15, yEnd = 15;    // ends are non-inclusive
    const KDint xBegin = -15, xEnd = 15;
    const KDint64 triangleCount = (yEnd - yBegin) * (xEnd - xBegin) * 2;
    const KDint64 vertices = triangleCount * 3;
    GLOBJECT *result;
    KDint x, y;
    KDint64 currentVertex, currentQuad;

    result = newGLObject(vertices, 2, 0);
    if (result == KD_NULL)
        return KD_NULL;

    currentQuad = 0;
    currentVertex = 0;

    for (y = yBegin; y < yEnd; ++y)
    {
        for (x = xBegin; x < xEnd; ++x)
        {
            GLubyte color;
            KDint i, a;
            color = (GLubyte)((randomUInt() & 0x5f) + 81);  // 101 1111
            for (i = currentVertex * 4; i < (currentVertex + 6) * 4; i += 4)
            {
                result->colorArray[i] = color;
                result->colorArray[i + 1] = color;
                result->colorArray[i + 2] = color;
                result->colorArray[i + 3] = 0;
            }

            // Axis bits for quad triangles:
            // x: 011100 (0x1c), y: 110001 (0x31)  (clockwise)
            // x: 001110 (0x0e), y: 100011 (0x23)  (counter-clockwise)
            for (a = 0; a < 6; ++a)
            {
                const KDint xm = x + ((0x1c >> a) & 1);
                const KDint ym = y + ((0x31 >> a) & 1);
                const KDfloat32 m = (kdCosf(xm * 2) * kdSinf(ym * 4) * 0.75f);
                result->vertexArray[currentVertex * 2] =
                    FIXED(xm * scale + m);
                result->vertexArray[currentVertex * 2 + 1] =
                    FIXED(ym * scale + m);
                ++currentVertex;
            }
            ++currentQuad;
        }
    }
    return result;
}


static void drawGroundPlane()
{
    glDisable(GL_CULL_FACE);
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_ZERO, GL_SRC_COLOR);
    glDisable(GL_LIGHTING);

    drawGLObject(sGroundPlane);

    glEnable(GL_LIGHTING);
    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
}


static void drawFadeQuad()
{
    static const GLfixed quadVertices[] = {
        -0x10000, -0x10000,
         0x10000, -0x10000,
        -0x10000,  0x10000,
         0x10000, -0x10000,
         0x10000,  0x10000,
        -0x10000,  0x10000
    };

    const KDint beginFade = sTick - sCurrentCamTrackStartTick;
    const KDint endFade = sNextCamTrackStartTick - sTick;
    const KDint minFade = beginFade < endFade ? beginFade : endFade;

    if (minFade < 1024)
    {
        const GLfixed fadeColor = minFade << 6;
        glColor4x(fadeColor, fadeColor, fadeColor, 0);

        glDisable(GL_DEPTH_TEST);
        glEnable(GL_BLEND);
        glBlendFunc(GL_ZERO, GL_SRC_COLOR);
        glDisable(GL_LIGHTING);

        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();

        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();

        glDisableClientState(GL_COLOR_ARRAY);
        glDisableClientState(GL_NORMAL_ARRAY);
        glVertexPointer(2, GL_FIXED, 0, quadVertices);
        glDrawArrays(GL_TRIANGLES, 0, 6);

        glEnableClientState(GL_COLOR_ARRAY);

        glMatrixMode(GL_MODELVIEW);

        glEnable(GL_LIGHTING);
        glDisable(GL_BLEND);
        glEnable(GL_DEPTH_TEST);
    }
}


// Called from the app framework.
void appInit()
{
    KDint a;

    glEnable(GL_NORMALIZE);
    glEnable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glShadeModel(GL_FLAT);

    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    glEnable(GL_LIGHT1);
    glEnable(GL_LIGHT2);

    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_COLOR_ARRAY);

    seedRandom(15);

    for (a = 0; a < SUPERSHAPE_COUNT; ++a)
    {
        sSuperShapeObjects[a] = createSuperShape(sSuperShapeParams[a]);
        kdAssert(sSuperShapeObjects[a] != KD_NULL);
    }
    sGroundPlane = createGroundPlane();
    kdAssert(sGroundPlane != KD_NULL);
}


// Called from the app framework.
void appDeinit()
{
    KDint a;
    for (a = 0; a < SUPERSHAPE_COUNT; ++a)
        freeGLObject(sSuperShapeObjects[a]);
    freeGLObject(sGroundPlane);
}


static void gluPerspective(GLfloat fovy, GLfloat aspect,
                           GLfloat zNear, GLfloat zFar)
{
    GLfloat xmin, xmax, ymin, ymax;

    ymax = zNear * (GLfloat)kdTanf(fovy * KD_PI_F / 360);
    ymin = -ymax;
    xmin = ymin * aspect;
    xmax = ymax * aspect;

    glFrustumx((GLfixed)(xmin * 65536), (GLfixed)(xmax * 65536),
               (GLfixed)(ymin * 65536), (GLfixed)(ymax * 65536),
               (GLfixed)(zNear * 65536), (GLfixed)(zFar * 65536));
}


static void prepareFrame(KDint width, KDint height)
{
    glViewport(0, 0, width, height);

    glClearColorx((GLfixed)(0.1f * 65536),
                  (GLfixed)(0.2f * 65536),
                  (GLfixed)(0.3f * 65536), 0x10000);
    glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(45, (KDfloat32)width / height, 0.5f, 150);

    glMatrixMode(GL_MODELVIEW);

    glLoadIdentity();
}


static void configureLightAndMaterial()
{
    static GLfixed light0Position[] = { -0x40000, 0x10000, 0x10000, 0 };
    static GLfixed light0Diffuse[] = { 0x10000, 0x6666, 0, 0x10000 };
    static GLfixed light1Position[] = { 0x10000, -0x20000, -0x10000, 0 };
    static GLfixed light1Diffuse[] = { 0x11eb, 0x23d7, 0x5999, 0x10000 };
    static GLfixed light2Position[] = { -0x10000, 0, -0x40000, 0 };
    static GLfixed light2Diffuse[] = { 0x11eb, 0x2b85, 0x23d7, 0x10000 };
    static GLfixed materialSpecular[] = { 0x10000, 0x10000, 0x10000, 0x10000 };

    glLightxv(GL_LIGHT0, GL_POSITION, light0Position);
    glLightxv(GL_LIGHT0, GL_DIFFUSE, light0Diffuse);
    glLightxv(GL_LIGHT1, GL_POSITION, light1Position);
    glLightxv(GL_LIGHT1, GL_DIFFUSE, light1Diffuse);
    glLightxv(GL_LIGHT2, GL_POSITION, light2Position);
    glLightxv(GL_LIGHT2, GL_DIFFUSE, light2Diffuse);
    glMaterialxv(GL_FRONT_AND_BACK, GL_SPECULAR, materialSpecular);

    glMaterialx(GL_FRONT_AND_BACK, GL_SHININESS, 60 << 16);
    glEnable(GL_COLOR_MATERIAL);
}


static void drawModels(KDfloat32 zScale)
{
    const KDint translationScale = 9;
    KDint x, y;

    seedRandom(9);

    glScalex(1 << 16, 1 << 16, (GLfixed)(zScale * 65536));

    for (y = -5; y <= 5; ++y)
    {
        for (x = -5; x <= 5; ++x)
        {
            KDfloat32 buildingScale;
            GLfixed fixedScale;

            KDint curShape = randomUInt() % SUPERSHAPE_COUNT;
            buildingScale = sSuperShapeParams[curShape][SUPERSHAPE_PARAMS - 1];
            fixedScale = (GLfixed)(buildingScale * 65536);

            glPushMatrix();
            glTranslatex((x * translationScale) * 65536,
                         (y * translationScale) * 65536,
                         0);
            glRotatex((GLfixed)((randomUInt() % 360) << 16), 0, 0, 1 << 16);
            glScalex(fixedScale, fixedScale, fixedScale);

            drawGLObject(sSuperShapeObjects[curShape]);
            glPopMatrix();
        }
    }

    for (x = -2; x <= 2; ++x)
    {
        const KDint shipScale100 = translationScale * 500;
        const KDint offs100 = x * shipScale100 + (sTick % shipScale100);
        KDfloat32 offs = offs100 * 0.01f;
        GLfixed fixedOffs = (GLfixed)(offs * 65536);
        glPushMatrix();
        glTranslatex(fixedOffs, -4 * 65536, 2 << 16);
        drawGLObject(sSuperShapeObjects[SUPERSHAPE_COUNT - 1]);
        glPopMatrix();
        glPushMatrix();
        glTranslatex(-4 * 65536, fixedOffs, 4 << 16);
        glRotatex(90 << 16, 0, 0, 1 << 16);
        drawGLObject(sSuperShapeObjects[SUPERSHAPE_COUNT - 1]);
        glPopMatrix();
    }
}


/* Following gluLookAt implementation is adapted from the
 * Mesa 3D Graphics library. http://www.mesa3d.org
 */
static void gluLookAt(GLfloat eyex, GLfloat eyey, GLfloat eyez,
	              GLfloat centerx, GLfloat centery, GLfloat centerz,
	              GLfloat upx, GLfloat upy, GLfloat upz)
{
    GLfloat m[16];
    GLfloat x[3], y[3], z[3];
    GLfloat mag;

    /* Make rotation matrix */

    /* Z vector */
    z[0] = eyex - centerx;
    z[1] = eyey - centery;
    z[2] = eyez - centerz;
    mag = kdSqrtf(z[0] * z[0] + z[1] * z[1] + z[2] * z[2]);
    if (mag) {			/* mpichler, 19950515 */
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
    if (mag) {
        x[0] /= mag;
        x[1] /= mag;
        x[2] /= mag;
    }

    mag = kdSqrtf(y[0] * y[0] + y[1] * y[1] + y[2] * y[2]);
    if (mag) {
        y[0] /= mag;
        y[1] /= mag;
        y[2] /= mag;
    }

#define M(row,col)  m[col*4+row]
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
    {
        KDint a;
        GLfixed fixedM[16];
        for (a = 0; a < 16; ++a)
            fixedM[a] = (GLfixed)(m[a] * 65536);
        glMultMatrixx(fixedM);
    }

    /* Translate Eye to Origin */
    glTranslatex((GLfixed)(-eyex * 65536),
                 (GLfixed)(-eyey * 65536),
                 (GLfixed)(-eyez * 65536));
}


static void camTrack()
{
    KDfloat32 lerp[5];
    KDfloat32 eX, eY, eZ, cX, cY, cZ;
    KDfloat32 trackPos;
    CAMTRACK *cam;
    KDust currentCamTick;
    KDint a;

    if (sNextCamTrackStartTick <= sTick)
    {
        ++sCurrentCamTrack;
        sCurrentCamTrackStartTick = sNextCamTrackStartTick;
    }
    sNextCamTrackStartTick = sCurrentCamTrackStartTick +
                             sCamTracks[sCurrentCamTrack].len * CAMTRACK_LEN;

    cam = &sCamTracks[sCurrentCamTrack];
    currentCamTick = sTick - sCurrentCamTrackStartTick;
    trackPos = (KDfloat32)currentCamTick / (CAMTRACK_LEN * cam->len);

    for (a = 0; a < 5; ++a)
        lerp[a] = (cam->src[a] + cam->dest[a] * trackPos) * 0.01f;

    if (cam->dist)
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

static KDint gAppAlive = 1;
// Called from the app framework.
/* The tick is current time in milliseconds, width and height
 * are the image dimensions to be rendered.
 */
void appRender(KDust tick, int width, int height)
{
    if (sStartTick == 0)
        sStartTick = tick;
    if (!gAppAlive)
        return;

    // Actual tick value is "blurred" a little bit.
    sTick = (sTick + tick - sStartTick) >> 1;

    // Terminate application after running through the demonstration once.
    if (sTick >= RUN_LENGTH)
    {
        gAppAlive = 0;
        return;
    }

    // Prepare OpenGL ES for rendering of the frame.
    prepareFrame(width, height);

    // Update the camera position and set the lookat.
    camTrack();

    // Configure environment.
    configureLightAndMaterial();

    // Draw the reflection by drawing models with negated Z-axis.
    glPushMatrix();
    drawModels(-1);
    glPopMatrix();

    // Blend the ground plane to the window.
    drawGroundPlane();

    // Draw all the models normally.
    drawModels(1);

    // Draw fade quad over whole window (when changing cameras).
    drawFadeQuad();
}

static void KD_APIENTRY kd_callback(const KDEvent *event)
{
    switch(event->type)
    {
        case(KD_EVENT_WINDOW_CLOSE):
        {
            gAppAlive = 0;
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
static void GL_APIENTRY gl_callback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar *message, const void *userParam)
{
    kdLogMessage(message);
}
#endif

KDint KD_APIENTRY kdMain(KDint argc, const KDchar *const *argv)
{
    const EGLint egl_attributes[] =
    {
        EGL_SURFACE_TYPE,       EGL_WINDOW_BIT,
        EGL_RENDERABLE_TYPE,    EGL_OPENGL_ES_BIT,
        EGL_RED_SIZE,           8,
        EGL_GREEN_SIZE,         8,
        EGL_BLUE_SIZE,          8,
        EGL_ALPHA_SIZE,         EGL_DONT_CARE,
        EGL_DEPTH_SIZE,         16,
        EGL_STENCIL_SIZE,       EGL_DONT_CARE,
        EGL_SAMPLE_BUFFERS,     1,
        EGL_SAMPLES,            4,
        EGL_NONE
    };

    const EGLint egl_context_attributes[] =
    {
        EGL_CONTEXT_CLIENT_VERSION, 1,
#if defined(EGL_KHR_create_context)    
        EGL_CONTEXT_FLAGS_KHR, EGL_CONTEXT_OPENGL_DEBUG_BIT_KHR,
#endif
        EGL_NONE,
    };

    EGLDisplay egl_display = eglGetDisplay(EGL_DEFAULT_DISPLAY);

    eglInitialize(egl_display, 0, 0);
    eglBindAPI(EGL_OPENGL_ES_API);

    EGLint egl_num_configs = 0;
    EGLConfig egl_config;
    eglChooseConfig(egl_display, egl_attributes, &egl_config, 1, &egl_num_configs);
    EGLContext egl_context = eglCreateContext(egl_display, egl_config, EGL_NO_CONTEXT, egl_context_attributes);

    KDWindow *kd_window = kdCreateWindow(egl_display, egl_config, KD_NULL);
    EGLNativeWindowType native_window;
    kdRealizeWindow(kd_window, &native_window);

    EGLSurface egl_surface = eglCreateWindowSurface(egl_display, egl_config, native_window, KD_NULL);

    eglMakeCurrent(egl_display, egl_surface, egl_surface, egl_context);

    if(eglGetError() != EGL_SUCCESS)
    {
        kdAssert(0);
    }

#if defined(GL_KHR_debug)
    if(kdStrstrVEN((const KDchar *)glGetString(GL_EXTENSIONS), "GL_KHR_debug"))
    {
        glEnable(GL_DEBUG_OUTPUT_KHR);
        glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS_KHR);
        PFNGLDEBUGMESSAGECALLBACKKHRPROC glDebugMessageCallback = (PFNGLDEBUGMESSAGECALLBACKKHRPROC)eglGetProcAddress("glDebugMessageCallbackKHR");
        glDebugMessageCallback(&gl_callback, KD_NULL);
    }
#endif

    kdInstallCallback(&kd_callback, KD_EVENT_WINDOW_CLOSE, KD_NULL);

    appInit();
    while(gAppAlive)
    {
        kdPumpEvents();

        if(eglSwapBuffers(egl_display, egl_surface) == EGL_FALSE)
        {
            EGLint egl_error = eglGetError();
            switch(egl_error)
            {
                case(EGL_BAD_SURFACE):
                {
                    eglMakeCurrent(egl_display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
                    eglDestroySurface(egl_display, egl_surface);
                    kdRealizeWindow(kd_window, &native_window);
                    egl_surface = eglCreateWindowSurface(egl_display, egl_config, native_window, KD_NULL);
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
        }
        else
        {
            KDint32 windowsize[2];
            kdGetWindowPropertyiv(kd_window, KD_WINDOWPROPERTY_SIZE, windowsize);
            appRender(kdGetTimeUST() / 1000000 , windowsize[0], windowsize[1]);
        }
    }
    appDeinit();

    eglDestroyContext(egl_display, egl_context);
    eglDestroySurface(egl_display, egl_surface);
    eglTerminate(egl_display);

    kdDestroyWindow(kd_window);

    return 0;
}
