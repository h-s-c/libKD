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

typedef KDfloat32 Matrix4x4[16];
typedef KDfloat32 Matrix3x3[9];

void Matrix4x4_Copy(Matrix4x4 dst, Matrix4x4 src)
{
    KDint i;
    for (i = 0; i < 16; ++i)
        dst[i] = src[i];
}


void Matrix4x4_Multiply(Matrix4x4 result, Matrix4x4 mat1, Matrix4x4 mat2)
{
    Matrix4x4 tmp;
    KDint i, j, k;
    for (i = 0; i < 4; i++)
    {
        for (j = 0; j < 4; ++j) {
            tmp[i*4 + j] = 0;
            for (k = 0; k < 4; ++k)
                tmp[i*4 + j] += mat1[i*4 + k] * mat2[k*4 + j];
        }
    }
    Matrix4x4_Copy(result, tmp);
}


void Matrix4x4_LoadIdentity(Matrix4x4 mat)
{
    KDint i;
    for (i = 0; i < 16; ++i)
        mat[i] = 0;
    for (i = 0; i < 4; ++i)
        mat[i*4 + i] = 1.f;
}


void Matrix4x4_Scale(Matrix4x4 mat, KDfloat32 sx, KDfloat32 sy, KDfloat32 sz)
{
    KDint i;
    for (i = 0; i < 4; ++i)
    {
        mat[0*4 + i] *= sx;
        mat[1*4 + i] *= sy;
        mat[2*4 + i] *= sz;
    }
}


void Matrix4x4_Translate(Matrix4x4 mat, KDfloat32 tx, KDfloat32 ty, KDfloat32 tz)
{
    int i;
    for (i = 0; i < 4; ++i)
        mat[3*4 + i] += mat[0*4 + i] * tx + 
                        mat[1*4 + i] * ty +
                        mat[2*4 + i] * tz;
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


void Matrix4x4_Rotate(Matrix4x4 mat, KDfloat32 angle,
                      KDfloat32 ax, KDfloat32 ay, KDfloat32 az)
{
    Matrix4x4 rot;
    KDfloat32 r = angle * KD_PI_F / 180.f;
    KDfloat32 s = kdSinf(r);
    KDfloat32 c = kdCosf(r);
    KDfloat32 one_c = 1.f - c;
    KDfloat32 xx, yy, zz, xy, yz, xz, xs, ys, zs;
    KDfloat32 norm = normalize(&ax, &ay, &az);

    if (norm == 0 || angle == 0)
        return;

    xx = ax * ax;
    yy = ay * ay;
    zz = az * az;
    xy = ax * ay;
    yz = ay * az;
    xz = ax * az;
    xs = ax * s;
    ys = ay * s;
    zs = az * s;

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

    Matrix4x4_Multiply(mat, rot, mat);
}


void Matrix4x4_Frustum(Matrix4x4 mat,
                       KDfloat32 left, KDfloat32 right,
                       KDfloat32 bottom, KDfloat32 top,
                       KDfloat32 near, KDfloat32 far)
{
    KDfloat32 dx = right - left;
    KDfloat32 dy = top - bottom;
    KDfloat32 dz = far - near;
    Matrix4x4 frust;

    if (near <= 0.f || far <= 0.f || dx <= 0.f || dy <= 0.f || dz <= 0.f)
        return;

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

    Matrix4x4_Multiply(mat, frust, mat);
}


void Matrix4x4_Perspective(Matrix4x4 mat,
                           KDfloat32 fovy, KDfloat32 aspect,
                           KDfloat32 nearZ, KDfloat32 farZ)
{
    KDfloat32 frustumW, frustumH;
    frustumH = kdTanf(fovy / 360.f * KD_PI_F) * nearZ;
    frustumW = frustumH * aspect;

    Matrix4x4_Frustum(mat, -frustumW, frustumW,
                      -frustumH, frustumH, nearZ, farZ);
}


void Matrix4x4_Transform(Matrix4x4 mat, KDfloat32 *x, KDfloat32 *y, KDfloat32 *z)
{
    KDfloat32 tx = mat[0*4 + 0] * (*x) + mat[1*4 + 0] * (*y) + mat[2*4 + 0] * (*z);
    KDfloat32 ty = mat[0*4 + 1] * (*x) + mat[1*4 + 1] * (*y) + mat[2*4 + 1] * (*z);
    KDfloat32 tz = mat[0*4 + 2] * (*x) + mat[1*4 + 2] * (*y) + mat[2*4 + 2] * (*z);
    *x = tx;
    *y = ty;
    *z = tz;
}

