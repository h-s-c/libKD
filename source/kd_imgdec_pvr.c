// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com

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

/******************************************************************************
 * KD includes
 ******************************************************************************/

/* clang-format off */
#if defined(__clang__)
#   pragma clang diagnostic push
#   pragma clang diagnostic ignored "-Wpadded"
#   if __has_warning("-Wreserved-id-macro")
#       pragma clang diagnostic ignored "-Wreserved-id-macro"
#   endif
#endif
#include <KD/kd.h>
#include <KD/kdext.h>
#if defined(__clang__)
#   pragma clang diagnostic pop
#endif

#include "kd_internal.h"
/* clang-format on */

/******************************************************************************
 * OpenKODE Core extension: KD_ATX_imgdec_pvr
 ******************************************************************************/
/******************************************************************************
 * The MIT License (MIT)
 * Copyright (c) Imagination Technologies Ltd.
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.

 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 ******************************************************************************/

typedef struct Pixel32 Pixel32;
struct Pixel32
{
    KDuint8 red, green, blue, alpha;
};

typedef struct Pixel128S Pixel128S;
struct Pixel128S
{
    KDint32 red, green, blue, alpha;
};

typedef struct PVRTCWord PVRTCWord;
struct PVRTCWord
{
    KDuint32 u32ModulationData;
    KDuint32 u32ColorData;
};

typedef struct PVRTCWordIndices PVRTCWordIndices;
struct PVRTCWordIndices
{
    KDint P[2], Q[2], R[2], S[2];
};

static Pixel32 getColorA(KDuint32 u32ColorData)
{
    Pixel32 color;

    // Opaque Color Mode - RGB 554
    if ((u32ColorData & 0x8000) != 0)
    {
        color.red   = (KDuint8)((u32ColorData & 0x7c00) >> 10); // 5->5 bits
        color.green = (KDuint8)((u32ColorData & 0x3e0)  >> 5); // 5->5 bits
        color.blue  = (KDuint8)(u32ColorData  & 0x1e) | ((u32ColorData & 0x1e) >> 4); // 4->5 bits
        color.alpha = (KDuint8)0xf;// 0->4 bits
    }
    // Transparent Color Mode - ARGB 3443
    else
    {
        color.red   = (KDuint8)((u32ColorData & 0xf00)  >> 7) | ((u32ColorData & 0xf00) >> 11); // 4->5 bits
        color.green = (KDuint8)((u32ColorData & 0xf0)   >> 3) | ((u32ColorData & 0xf0)  >> 7); // 4->5 bits
        color.blue  = (KDuint8)((u32ColorData & 0xe)    << 1) | ((u32ColorData & 0xe)   >> 2); // 3->5 bits
        color.alpha = (KDuint8)((u32ColorData & 0x7000) >> 11);// 3->4 bits - note 0 at right
    }

    return color;
}

static Pixel32 getColorB(KDuint32 u32ColorData)
{
    Pixel32 color;

    // Opaque Color Mode - RGB 555
    if (u32ColorData & 0x80000000)
    {
        color.red   = (KDuint8)((u32ColorData & 0x7c000000) >> 26); // 5->5 bits
        color.green = (KDuint8)((u32ColorData & 0x3e00000)  >> 21); // 5->5 bits
        color.blue  = (KDuint8)((u32ColorData & 0x1f0000)   >> 16); // 5->5 bits
        color.alpha = (KDuint8)0xf;// 0 bits
    }
    // Transparent Color Mode - ARGB 3444
    else
    {
        color.red   = (KDuint8)(((u32ColorData & 0xf000000)  >> 23) | ((u32ColorData & 0xf000000) >> 27)); // 4->5 bits
        color.green = (KDuint8)(((u32ColorData & 0xf00000)   >> 19) | ((u32ColorData & 0xf00000)  >> 23)); // 4->5 bits
        color.blue  = (KDuint8)(((u32ColorData & 0xf0000)    >> 15) | ((u32ColorData & 0xf0000)   >> 19)); // 4->5 bits
        color.alpha = (KDuint8)((u32ColorData & 0x70000000) >> 27);// 3->4 bits - note 0 at right
    }

    return color;
}

static void interpolateColors(Pixel32 P, Pixel32 Q, Pixel32 R, Pixel32 S,
                              Pixel128S* pPixel, KDuint8 ui8Bpp)
{
    KDuint32 ui32WordWidth = 4;
    KDuint32 ui32WordHeight = 4;
    if (ui8Bpp == 2)
    {
        ui32WordWidth = 8;
    }

    //Convert to int 32.
    Pixel128S hP = {(KDint32)P.red, (KDint32)P.green, (KDint32)P.blue, (KDint32)P.alpha};
    Pixel128S hQ = {(KDint32)Q.red, (KDint32)Q.green, (KDint32)Q.blue, (KDint32)Q.alpha};
    Pixel128S hR = {(KDint32)R.red, (KDint32)R.green, (KDint32)R.blue, (KDint32)R.alpha};
    Pixel128S hS = {(KDint32)S.red, (KDint32)S.green, (KDint32)S.blue, (KDint32)S.alpha};

    //Get vectors.
    Pixel128S QminusP = {hQ.red - hP.red, hQ.green - hP.green, hQ.blue - hP.blue, hQ.alpha - hP.alpha};
    Pixel128S SminusR = {hS.red - hR.red, hS.green - hR.green, hS.blue - hR.blue, hS.alpha - hR.alpha};

    //Multiply colors.
    hP.red    *=  ui32WordWidth;
    hP.green  *=  ui32WordWidth;
    hP.blue   *=  ui32WordWidth;
    hP.alpha  *=  ui32WordWidth;
    hR.red    *=  ui32WordWidth;
    hR.green  *=  ui32WordWidth;
    hR.blue   *=  ui32WordWidth;
    hR.alpha  *=  ui32WordWidth;

    if (ui8Bpp == 2)
    {
        //Loop through pixels to achieve results.
        for (KDuint x = 0; x < ui32WordWidth; x++)
        {
            Pixel128S result = {4 * hP.red, 4 * hP.green, 4 * hP.blue, 4 * hP.alpha};
            Pixel128S dY = {hR.red - hP.red, hR.green - hP.green, hR.blue - hP.blue, hR.alpha - hP.alpha};

            for (KDuint y = 0; y < ui32WordHeight; y++)
            {
                pPixel[y * ui32WordWidth + x].red = (KDint32)((result.red >> 7) + (result.red >> 2));
                pPixel[y * ui32WordWidth + x].green = (KDint32)((result.green >> 7) + (result.green >> 2));
                pPixel[y * ui32WordWidth + x].blue = (KDint32)((result.blue >> 7) + (result.blue >> 2));
                pPixel[y * ui32WordWidth + x].alpha = (KDint32)((result.alpha >> 5) + (result.alpha >> 1));

                result.red += dY.red;
                result.green += dY.green;
                result.blue += dY.blue;
                result.alpha += dY.alpha;
            }

            hP.red    += QminusP.red;
            hP.green  += QminusP.green;
            hP.blue   += QminusP.blue;
            hP.alpha  += QminusP.alpha;

            hR.red    += SminusR.red;
            hR.green  += SminusR.green;
            hR.blue   += SminusR.blue;
            hR.alpha  += SminusR.alpha;
        }
    }
    else
    {
        //Loop through pixels to achieve results.
        for (KDuint y = 0; y < ui32WordHeight; y++)
        {
            Pixel128S result = {4 * hP.red, 4 * hP.green, 4 * hP.blue, 4 * hP.alpha};
            Pixel128S dY = {hR.red - hP.red, hR.green - hP.green, hR.blue - hP.blue, hR.alpha - hP.alpha};

            for (KDuint x = 0; x < ui32WordWidth; x++)
            {
                pPixel[y * ui32WordWidth + x].red   = (KDint32)((result.red   >> 6) + (result.red   >> 1));
                pPixel[y * ui32WordWidth + x].green = (KDint32)((result.green >> 6) + (result.green >> 1));
                pPixel[y * ui32WordWidth + x].blue  = (KDint32)((result.blue  >> 6) + (result.blue  >> 1));
                pPixel[y * ui32WordWidth + x].alpha = (KDint32)((result.alpha >> 4) + (result.alpha));

                result.red += dY.red;
                result.green += dY.green;
                result.blue += dY.blue;
                result.alpha += dY.alpha;
            }

            hP.red += QminusP.red;
            hP.green += QminusP.green;
            hP.blue += QminusP.blue;
            hP.alpha += QminusP.alpha;

            hR.red += SminusR.red;
            hR.green += SminusR.green;
            hR.blue += SminusR.blue;
            hR.alpha += SminusR.alpha;
        }
    }
}

static void unpackModulations(const PVRTCWord *word, KDint offsetX, KDint offsetY, KDint32 i32ModulationValues[16][8],
                              KDint32 i32ModulationModes[16][8], KDuint8 ui8Bpp)
{
    KDuint32 WordModMode = word->u32ColorData & 0x1;
    KDuint32 ModulationBits = word->u32ModulationData;

    // Unpack differently depending on 2bpp or 4bpp modes.
    if (ui8Bpp == 2)
    {
        if (WordModMode)
        {
            // determine which of the three modes are in use:

            // If this is the either the H-only or V-only interpolation mode...
            if (ModulationBits & 0x1)
            {
                // look at the "LSB" for the "centre" (V=2,H=4) texel. Its LSB is now
                // actually used to indicate whether it's the H-only mode or the V-only...

                // The centre texel data is the at (y==2, x==4) and so its LSB is at bit 20.
                if (ModulationBits & (0x1 << 20))
                {
                    // This is the V-only mode
                    WordModMode = 3;
                }
                else
                {
                    // This is the H-only mode
                    WordModMode = 2;
                }

                // Create an extra bit for the centre pixel so that it looks like
                // we have 2 actual bits for this texel. It makes later coding much easier.
                if (ModulationBits & (0x1 << 21))
                {
                    // set it to produce code for 1.0
                    ModulationBits |= (0x1 << 20);
                }
                else
                {
                    // clear it to produce 0.0 code
                    ModulationBits &= (KDuint)~(0x1 << 20);
                }
            }// end if H-Only or V-Only interpolation mode was chosen

            if (ModulationBits & 0x2)
            {
                ModulationBits |= 0x1; /*set it*/
            }
            else
            {
                ModulationBits &= (KDuint)~0x1; /*clear it*/
            }

            // run through all the pixels in the block. Note we can now treat all the
            // "stored" values as if they have 2bits (even when they didn't!)
            for (KDint y = 0; y < 4; y++)
            {
                for (KDint x = 0; x < 8; x++)
                {
                    i32ModulationModes[x + offsetX][y + offsetY] = (KDint32)WordModMode;

                    // if this is a stored value...
                    if (((x ^ y) & 1) == 0)
                    {
                        i32ModulationValues[x + offsetX][y + offsetY] = ModulationBits & 3;
                        ModulationBits >>= 2;
                    }
                }
            } // end for y
        }
        // else if direct encoded 2bit mode - i.e. 1 mode bit per pixel
        else
        {
            for (KDint y = 0; y < 4; y++)
            {
                for (KDint x = 0; x < 8; x++)
                {
                    i32ModulationModes[x + offsetX][y + offsetY] = (KDint32)WordModMode;

                    /*
                    // double the bits so 0=> 00, and 1=>11
                    */
                    if (ModulationBits & 1)
                    {
                        i32ModulationValues[x + offsetX][y + offsetY] = 0x3;
                    }
                    else
                    {
                        i32ModulationValues[x + offsetX][y + offsetY] = 0x0;
                    }
                    ModulationBits >>= 1;
                }
            }// end for y
        }
    }
    else
    {
        //Much simpler than the 2bpp decompression, only two modes, so the n/8 values are set directly.
        // run through all the pixels in the word.
        if (WordModMode)
        {
            for (KDint y = 0; y < 4; y++)
            {
                for (KDint x = 0; x < 4; x++)
                {
                    i32ModulationValues[y + offsetY][x + offsetX] = ModulationBits & 3;
                    //if (i32ModulationValues==0) {}; don't need to check 0, 0 = 0/8.
                    if (i32ModulationValues[y + offsetY][x + offsetX] == 1)
                    {
                        i32ModulationValues[y + offsetY][x + offsetX] = 4;
                    }
                    else if (i32ModulationValues[y + offsetY][x + offsetX] == 2)
                    {
                        i32ModulationValues[y + offsetY][x + offsetX] = 14; //+10 tells the decompressor to punch through alpha.
                    }
                    else if (i32ModulationValues[y + offsetY][x + offsetX] == 3)
                    {
                        i32ModulationValues[y + offsetY][x + offsetX] = 8;
                    }
                    ModulationBits >>= 2;
                } // end for x
            } // end for y
        }
        else
        {
            for (KDint y = 0; y < 4; y++)
            {
                for (KDint x = 0; x < 4; x++)
                {
                    i32ModulationValues[y + offsetY][x + offsetX] = ModulationBits & 3;
                    i32ModulationValues[y + offsetY][x + offsetX] *= 3;
                    if (i32ModulationValues[y + offsetY][x + offsetX] > 3) { i32ModulationValues[y + offsetY][x + offsetX] -= 1; }
                    ModulationBits >>= 2;
                } // end for x
            } // end for y
        }
    }
}

static KDint32 getModulationValues(KDint32 i32ModulationValues[16][8], KDint32 i32ModulationModes[16][8], KDuint32 xPos, KDuint32 yPos,
                                 KDuint8 ui8Bpp)
{
    if (ui8Bpp == 2)
    {
        const KDint RepVals0[4] = {0, 3, 5, 8};

        // extract the modulation value. If a simple encoding
        if (i32ModulationModes[xPos][yPos] == 0)
        {
            return RepVals0[i32ModulationValues[xPos][yPos]];
        }
        else
        {
            // if this is a stored value
            if (((xPos ^ yPos) & 1) == 0)
            {
                return RepVals0[i32ModulationValues[xPos][yPos]];
            }

            // else average from the neighbours
            // if H&V interpolation...
            else if (i32ModulationModes[xPos][yPos] == 1)
            {
                return (RepVals0[i32ModulationValues[xPos][yPos - 1]] +
                        RepVals0[i32ModulationValues[xPos][yPos + 1]] +
                        RepVals0[i32ModulationValues[xPos - 1][yPos]] +
                        RepVals0[i32ModulationValues[xPos + 1][yPos]] + 2) / 4;
            }
            // else if H-Only
            else if (i32ModulationModes[xPos][yPos] == 2)
            {
                return (RepVals0[i32ModulationValues[xPos - 1][yPos]] +
                        RepVals0[i32ModulationValues[xPos + 1][yPos]] + 1) / 2;
            }
            // else it's V-Only
            else
            {
                return (RepVals0[i32ModulationValues[xPos][yPos - 1]] +
                        RepVals0[i32ModulationValues[xPos][yPos + 1]] + 1) / 2;
            }
        }
    }
    else if (ui8Bpp == 4)
    {
        return i32ModulationValues[xPos][yPos];
    }

    return 0;
}

static void pvrtcGetDecompressedPixels(const PVRTCWord *P, const PVRTCWord *Q,
                                       const PVRTCWord *R, const PVRTCWord *S,
                                       Pixel32* pColorData,
                                       KDuint8 ui8Bpp)
{
    //4bpp only needs 8*8 values, but 2bpp needs 16*8, so rather than wasting processor time we just statically allocate 16*8.
    KDint32 i32ModulationValues[16][8];
    //Only 2bpp needs this.
    KDint32 i32ModulationModes[16][8];
    //4bpp only needs 16 values, but 2bpp needs 32, so rather than wasting processor time we just statically allocate 32.
    Pixel128S upscaledColorA[32];
    Pixel128S upscaledColorB[32];

    KDuint32 ui32WordWidth = 4;
    KDuint32 ui32WordHeight = 4;
    if (ui8Bpp == 2)
    {
        ui32WordWidth = 8;
    }

    //Get the modulations from each word.
    unpackModulations(P, 0, 0, i32ModulationValues, i32ModulationModes, ui8Bpp);
    unpackModulations(Q, (KDint)ui32WordWidth, 0, i32ModulationValues, i32ModulationModes, ui8Bpp);
    unpackModulations(R, 0, (KDint)ui32WordHeight, i32ModulationValues, i32ModulationModes, ui8Bpp);
    unpackModulations(S, (KDint)ui32WordWidth, (KDint)ui32WordHeight, i32ModulationValues, i32ModulationModes, ui8Bpp);

    // Bilinear upscale image data from 2x2 -> 4x4
    interpolateColors(getColorA(P->u32ColorData), getColorA(Q->u32ColorData),
                      getColorA(R->u32ColorData), getColorA(S->u32ColorData),
                      upscaledColorA, ui8Bpp);
    interpolateColors(getColorB(P->u32ColorData), getColorB(Q->u32ColorData),
                      getColorB(R->u32ColorData), getColorB(S->u32ColorData),
                      upscaledColorB, ui8Bpp);

    for (KDuint y = 0; y < ui32WordHeight; y++)
    {
        for (KDuint x = 0; x < ui32WordWidth; x++)
        {
            KDint32 mod = getModulationValues(i32ModulationValues, i32ModulationModes, x + ui32WordWidth / 2, y + ui32WordHeight / 2, ui8Bpp);
            KDboolean punchthroughAlpha = KD_FALSE;
            if (mod > 10)
            {
                punchthroughAlpha = KD_TRUE;
                mod -= 10;
            }

            Pixel128S result;
            result.red   = (upscaledColorA[y * ui32WordWidth + x].red * (8 - mod) + upscaledColorB[y * ui32WordWidth + x].red * mod) / 8;
            result.green = (upscaledColorA[y * ui32WordWidth + x].green * (8 - mod) + upscaledColorB[y * ui32WordWidth + x].green * mod) /
                           8;
            result.blue  = (upscaledColorA[y * ui32WordWidth + x].blue * (8 - mod) + upscaledColorB[y * ui32WordWidth + x].blue * mod) / 8;
            if (punchthroughAlpha) { result.alpha = 0; }
            else { result.alpha = (upscaledColorA[y * ui32WordWidth + x].alpha * (8 - mod) + upscaledColorB[y * ui32WordWidth + x].alpha * mod) / 8; }

            //Convert the 32bit precision Result to 8 bit per channel color.
            if (ui8Bpp == 2)
            {
                pColorData[y * ui32WordWidth + x].red = (KDuint8)result.red;
                pColorData[y * ui32WordWidth + x].green = (KDuint8)result.green;
                pColorData[y * ui32WordWidth + x].blue = (KDuint8)result.blue;
                pColorData[y * ui32WordWidth + x].alpha = (KDuint8)result.alpha;
            }
            else if (ui8Bpp == 4)
            {
                pColorData[y + x * ui32WordHeight].red = (KDuint8)result.red;
                pColorData[y + x * ui32WordHeight].green = (KDuint8)result.green;
                pColorData[y + x * ui32WordHeight].blue = (KDuint8)result.blue;
                pColorData[y + x * ui32WordHeight].alpha = (KDuint8)result.alpha;
            }
        }
    }
}

static KDuint wrapWordIndex(KDuint numWords, KDint word)
{
    return (((KDuint)word + numWords) % numWords);
}

/// <summary>Check that a number is an integer power of two, i.e.: 1, 2, 4, 8, ... etc.</summary>
/// <param name="input">Value to be checked</param>
/// <returns>true if the number is a non-zero, integer power of two, else false.</returns>
static KDboolean isPowerOf2(KDuint input)
{
    KDuint minus1;

    if (!input) { return KD_FALSE; }

    minus1 = input - 1;
    return ((input | minus1) == (input ^ minus1));
}

static KDuint32 TwiddleUV(KDuint32 XSize, KDuint32 YSize, KDuint32 XPos, KDuint32 YPos)
{
    //Initially assume X is the larger size.
    KDuint32 MinDimension = XSize;
    KDuint32 MaxValue = YPos;
    KDuint32 Twiddled = 0;
    KDuint32 SrcBitPos = 1;
    KDuint32 DstBitPos = 1;
    KDint ShiftCount = 0;

    //Check the sizes are valid.
    kdAssert(YPos < YSize);
    kdAssert(XPos < XSize);
    kdAssert(isPowerOf2(YSize));
    kdAssert(isPowerOf2(XSize));

    //If Y is the larger dimension - switch the min/max values.
    if (YSize < XSize)
    {
        MinDimension = YSize;
        MaxValue   = XPos;
    }

    // Step through all the bits in the "minimum" dimension
    while (SrcBitPos < MinDimension)
    {
        if (YPos & SrcBitPos)
        {
            Twiddled |= DstBitPos;
        }

        if (XPos & SrcBitPos)
        {
            Twiddled |= (DstBitPos << 1);
        }

        SrcBitPos <<= 1;
        DstBitPos <<= 2;
        ShiftCount += 1;
    }

    // Prepend any unused bits
    MaxValue >>= ShiftCount;
    Twiddled |= (MaxValue << (2 * ShiftCount));

    return Twiddled;
}

static void mapDecompressedData(Pixel32* pOutput, KDint width,
                                const Pixel32* pWord,
                                const PVRTCWordIndices *words,
                                KDuint8 ui8Bpp)
{
    KDuint32 ui32WordWidth = 4;
    KDuint32 ui32WordHeight = 4;
    if (ui8Bpp == 2)
    {
        ui32WordWidth = 8;
    }

    for (KDuint y = 0; y < ui32WordHeight / 2; y++)
    {
        for (KDuint x = 0; x < ui32WordWidth / 2; x++)
        {
            pOutput[((((KDuint)words->P[1] * ui32WordHeight) + y + ui32WordHeight / 2)
                     * (KDuint)width + (KDuint)words->P[0] *ui32WordWidth + x + ui32WordWidth / 2)]  = pWord[y * ui32WordWidth + x];     // map P

            pOutput[((((KDuint)words->Q[1] * ui32WordHeight) + y + ui32WordHeight / 2)
                     * (KDuint)width + (KDuint)words->Q[0] *ui32WordWidth + x)]          = pWord[y * ui32WordWidth + x + ui32WordWidth / 2];   // map Q

            pOutput[((((KDuint)words->R[1] * ui32WordHeight) + y)
                     * (KDuint)width + (KDuint)words->R[0] *ui32WordWidth + x + ui32WordWidth / 2)]  = pWord[(y + ui32WordHeight / 2) * ui32WordWidth + x];    // map R

            pOutput[((((KDuint)words->S[1] * ui32WordHeight) + y)
                     * (KDuint)width + (KDuint)words->S[0] *ui32WordWidth + x)]          = pWord[(y + ui32WordHeight / 2) * ui32WordWidth + x + ui32WordWidth / 2];  // map S
        }
    }
}
static KDint pvrtcDecompress(const KDuint8* pCompressedData,
                           Pixel32* pDecompressedData,
                           KDuint32 ui32Width,
                           KDuint32 ui32Height,
                           KDuint8 ui8Bpp)
{
    KDuint32 ui32WordWidth = 4;
    KDuint32 ui32WordHeight = 4;
    if (ui8Bpp == 2)
    {
        ui32WordWidth = 8;
    }

    const KDuint8* pWordMembers = pCompressedData;
    Pixel32* pOutData = pDecompressedData;

    // Calculate number of words
    KDint i32NumXWords = (KDint)(ui32Width / ui32WordWidth);
    KDint i32NumYWords = (KDint)(ui32Height / ui32WordHeight);

    // Structs used for decompression
    PVRTCWordIndices indices;
    Pixel32* pPixels;
    pPixels = (Pixel32*)kdMalloc(ui32WordWidth * ui32WordHeight * sizeof(Pixel32));

    // For each row of words
    for (KDint wordY = -1; wordY < i32NumYWords - 1; wordY++)
    {
        // for each column of words
        for (KDint wordX = -1; wordX < i32NumXWords - 1; wordX++)
        {
            indices.P[0] = (KDint)wrapWordIndex((KDuint)i32NumXWords, wordX);
            indices.P[1] = (KDint)wrapWordIndex((KDuint)i32NumYWords, wordY);
            indices.Q[0] = (KDint)wrapWordIndex((KDuint)i32NumXWords, wordX + 1);
            indices.Q[1] = (KDint)wrapWordIndex((KDuint)i32NumYWords, wordY);
            indices.R[0] = (KDint)wrapWordIndex((KDuint)i32NumXWords, wordX);
            indices.R[1] = (KDint)wrapWordIndex((KDuint)i32NumYWords, wordY + 1);
            indices.S[0] = (KDint)wrapWordIndex((KDuint)i32NumXWords, wordX + 1);
            indices.S[1] = (KDint)wrapWordIndex((KDuint)i32NumYWords, wordY + 1);

            //Work out the offsets into the twiddle structs, multiply by two as there are two members per word.
            KDuint32 WordOffsets[4] =
            {
                TwiddleUV((KDuint)i32NumXWords, (KDuint)i32NumYWords, (KDuint)indices.P[0], (KDuint)indices.P[1]) * 2,
                TwiddleUV((KDuint)i32NumXWords, (KDuint)i32NumYWords, (KDuint)indices.Q[0], (KDuint)indices.Q[1]) * 2,
                TwiddleUV((KDuint)i32NumXWords, (KDuint)i32NumYWords, (KDuint)indices.R[0], (KDuint)indices.R[1]) * 2,
                TwiddleUV((KDuint)i32NumXWords, (KDuint)i32NumYWords, (KDuint)indices.S[0], (KDuint)indices.S[1]) * 2,
            };

            //Access individual elements to fill out PVRTCWord
            PVRTCWord P, Q, R, S;
            P.u32ColorData = pWordMembers[WordOffsets[0] + 1];
            P.u32ModulationData = pWordMembers[WordOffsets[0]];
            Q.u32ColorData = pWordMembers[WordOffsets[1] + 1];
            Q.u32ModulationData = pWordMembers[WordOffsets[1]];
            R.u32ColorData = pWordMembers[WordOffsets[2] + 1];
            R.u32ModulationData = pWordMembers[WordOffsets[2]];
            S.u32ColorData = pWordMembers[WordOffsets[3] + 1];
            S.u32ModulationData = pWordMembers[WordOffsets[3]];

            // assemble 4 words into struct to get decompressed pixels from
            pvrtcGetDecompressedPixels(&P, &Q, &R, &S, pPixels, ui8Bpp);
            mapDecompressedData(pOutData, (KDint)ui32Width, pPixels, &indices, ui8Bpp);

        } // for each word
    } // for each row of words

    kdFree(pPixels);
    //Return the data size
    return (KDint)(ui32Width * ui32Height / (ui32WordWidth / 2));
}

KDint __kdDecompressPVRTC(const void* pCompressedData,
                        KDint Do2bitMode,
                        KDint XDim,
                        KDint YDim,
                        KDuint8* pResultImage)
{
    //Cast the output buffer to a Pixel32 pointer.
    Pixel32* pDecompressedData = (Pixel32*)pResultImage;

    //Check the X and Y values are at least the minimum size.
    KDint XTrueDim = kdMaxVEN(XDim, ((Do2bitMode == 1) ? 16 : 8));
    KDint YTrueDim = kdMaxVEN(YDim, 8);

    //If the dimensions aren't correct, we need to create a new buffer instead of just using the provided one, as the buffer will overrun otherwise.
    if (XTrueDim != XDim || YTrueDim != YDim)
    {
        pDecompressedData = (Pixel32*)kdMalloc((KDsize)XTrueDim * (KDsize)YTrueDim * sizeof(Pixel32));
    }

    //Decompress the surface.
    KDint retval = pvrtcDecompress((const KDuint8*)pCompressedData, pDecompressedData, (KDuint32)XTrueDim, (KDuint32)YTrueDim, (Do2bitMode == 1 ? 2 : 4));

    //If the dimensions were too small, then copy the new buffer back into the output buffer.
    if (XTrueDim != XDim || YTrueDim != YDim)
    {
        //Loop through all the required pixels.
        for (KDint x = 0; x < XDim; ++x)
        {
            for (KDint y = 0; y < YDim; ++y)
            {
                ((Pixel32*)pResultImage)[x + y * XDim] = pDecompressedData[x + y * XTrueDim];
            }
        }

        //Free the temporary buffer.
        kdFree(pDecompressedData);
    }
    return retval;
}
