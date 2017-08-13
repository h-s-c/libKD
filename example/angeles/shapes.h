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

#ifndef SHAPES_H_INCLUDED
#define SHAPES_H_INCLUDED


#define SUPERSHAPE_PARAMS 15

static const float sSuperShapeParams[][SUPERSHAPE_PARAMS] =
{
    // m  a     b     n1      n2     n3     m     a     b     n1     n2      n3   res1 res2 scale  (org.res1,res2)
    { 10, 1,    2,    90,      1,   -45,    8,    1,    1,    -1,     1,  -0.4f,   20,  30, 2 }, // 40, 60
    { 10, 1,    2,    90,      1,   -45,    4,    1,    1,    10,     1,  -0.4f,   20,  20, 4 }, // 40, 40
    { 10, 1,    2,    60,      1,   -10,    4,    1,    1,    -1,    -2,  -0.4f,   41,  41, 1 }, // 82, 82
    {  6, 1,    1,    60,      1,   -70,    8,    1,    1,  0.4f,     3,  0.25f,   20,  20, 1 }, // 40, 40
    {  4, 1,    1,    30,      1,    20,   12,    1,    1,  0.4f,     3,  0.25f,   10,  30, 1 }, // 20, 60
    {  8, 1,    1,    30,      1,    -4,    8,    2,    1,    -1,     5,   0.5f,   25,  26, 1 }, // 60, 60
    { 13, 1,    1,    30,      1,    -4,   13,    1,    1,     1,     5,      1,   30,  30, 6 }, // 60, 60
    { 10, 1, 1.1f, -0.5f,   0.1f,    70,   60,    1,    1,   -90,     0, -0.25f,   20,  60, 8 }, // 60, 180
    {  7, 1,    1,    20,  -0.3f, -3.5f,    6,    1,    1,    -1,  4.5f,   0.5f,   10,  20, 4 }, // 60, 80
    {  4, 1,    1,    10,     10,    10,    4,    1,    1,    10,    10,     10,   10,  20, 1 }, // 20, 40
    {  4, 1,    1,     1,      1,     1,    4,    1,    1,     1,     1,      1,   10,  10, 2 }, // 10, 10
    {  1, 1,    1,    38, -0.25f,    19,    4,    1,    1,    10,    10,     10,   10,  15, 2 }, // 20, 40
    {  2, 1,    1,  0.7f,   0.3f,  0.2f,    3,    1,    1,   100,   100,    100,   10,  25, 2 }, // 20, 50
    {  6, 1,    1,     1,      1,     1,    3,    1,    1,     1,     1,      1,   30,  30, 2 }, // 60, 60
    {  3, 1,    1,     1,      1,     1,    6,    1,    1,     2,     1,      1,   10,  20, 2 }, // 20, 40
    {  6, 1,    1,     6,   5.5f,   100,    6,    1,    1,    25,    10,     10,   30,  20, 2 }, // 60, 40
    {  3, 1,    1,  0.5f,   1.7f,  1.7f,    2,    1,    1,    10,    10,     10,   20,  20, 2 }, // 40, 40
    {  5, 1,    1,  0.1f,   1.7f,  1.7f,    1,    1,    1,  0.3f,  0.5f,   0.5f,   20,  20, 4 }, // 40, 40
    {  2, 1,    1,     6,   5.5f,   100,    6,    1,    1,     4,    10,     10,   10,  22, 1 }, // 40, 40
    {  6, 1,    1,    -1,     70,  0.1f,    9,    1, 0.5f,   -98, 0.05f,    -45,   20,  30, 4 }, // 60, 91
    {  6, 1,    1,    -1,     90, -0.1f,    7,    1,    1,    90,  1.3f,     34,   13,  16, 1 }, // 32, 60
};
#define SUPERSHAPE_COUNT (sizeof(sSuperShapeParams) / sizeof(sSuperShapeParams[0]))


#endif // !SHAPES_H_INCLUDED
