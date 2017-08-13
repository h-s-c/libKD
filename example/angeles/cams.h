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

#ifndef CAMS_H_INCLUDED
#define CAMS_H_INCLUDED


/* Length in milliseconds of one camera track base unit.
 * The value originates from the music synchronization.
 */
#define CAMTRACK_LEN    5442


// Camera track definition for one camera trucking shot.
typedef struct
{
    /* Five parameters of src[5] and dest[5]:
     * eyeX, eyeY, eyeZ, viewAngle, viewHeightOffs
     */
    short src[5], dest[5];
    unsigned char dist;     // if >0, cam rotates around eye xy on dist * 0.1
    unsigned char len;      // length multiplier
} CAMTRACK;

static CAMTRACK sCamTracks[] =
{
    { { 4500, 2700, 100, 70, -30 }, { 50, 50, -90, -100, 0 }, 20, 1 },
    { { -1448, 4294, 25, 363, 0 }, { -136, 202, 125, -98, 100 }, 0, 1 },
    { { 1437, 4930, 200, -275, -20 }, { 1684, 0, 0, 9, 0 }, 0, 1 },
    { { 1800, 3609, 200, 0, 675 }, { 0, 0, 0, 300, 0 }, 0, 1 },
    { { 923, 996, 50, 2336, -80 }, { 0, -20, -50, 0, 170 }, 0, 1 },
    { { -1663, -43, 600, 2170, 0 }, { 20, 0, -600, 0, 100 }, 0, 1 },
    { { 1049, -1420, 175, 2111, -17 }, { 0, 0, 0, -334, 0 }, 0, 2 },
    { { 0, 0, 50, 300, 25 }, { 0, 0, 0, 300, 0 }, 70, 2 },
    { { -473, -953, 3500, -353, -350 }, { 0, 0, -2800, 0, 0 }, 0, 2 },
    { { 191, 1938, 35, 1139, -17 }, { 1205, -2909, 0, 0, 0 }, 0, 2 },
    { { -1449, -2700, 150, 0, 0 }, { 0, 2000, 0, 0, 0 }, 0, 2 },
    { { 5273, 4992, 650, 373, -50 }, { -4598, -3072, 0, 0, 0 }, 0, 2 },
    { { 3223, -3282, 1075, -393, -25 }, { 1649, -1649, 0, 0, 0 }, 0, 2 }
};
#define CAMTRACK_COUNT (sizeof(camTracks) / sizeof(camTracks[0]))


#endif // !CAMS_H_INCLUDED
