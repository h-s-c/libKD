// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com

/******************************************************************************
 * libKD
 * zlib/libpng License
 ******************************************************************************
 * Copyright (c) 2014-2020 Kevin Schmidt
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

#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wpadded"
#if __has_warning("-Wreserved-id-macro")
#pragma clang diagnostic ignored "-Wreserved-id-macro"
#endif
#endif
#include "kdplatform.h"  // for KDint32, KDuint32, kdAssert, KDsize
#include <KD/kd.h>       // for KDint, KDuint, KDuint8, kdFree, kdMalloc
#include <KD/kdext.h>    // for kdMaxVEN
#if defined(__clang__)
#pragma clang diagnostic pop
#endif

#include "kd_internal.h"  // IWYU pragma: keep

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

/******************************************************************************
 * Defines and consts
 ******************************************************************************/

#define PT_INDEX (2)  // The Punch-through index

#define BLK_Y_SIZE (4)  // always 4 for all 2D block types

#define BLK_X_2BPP (8)  // dimensions for the two formats
#define BLK_X_4BPP (4)

#define WRAP_COORD(Val, Size) ((Val) & ((Size)-1))

#define PVRT_CLAMP(x, l, h) (kdMinVEN((h), kdMaxVEN((x), (l))))

/*
 Define an expression to either wrap or clamp large or small vals to the
 legal coordinate range
 */
#define LIMIT_COORD(Val, Size, AssumeImageTiles) \
    ((AssumeImageTiles) ? WRAP_COORD((Val), (Size)) : PVRT_CLAMP((Val), 0, (Size)-1))

/***********************************************************
 * Decompression routines
 ************************************************************/

typedef struct
{
    /* Uses 64 bits pre block */
    KDuint32 PackedData[2];
} AMTC_BLOCK_STRUCT;

/******************************************************************************
 * Unpack5554Colour: Given a block, extract the colour information and convert
 * to 5554 formats
 ******************************************************************************/
static void Unpack5554Colour(const AMTC_BLOCK_STRUCT *pBlock, KDint ABColours[2][4])
{
    KDuint32 RawBits[2];

    // Extract A and B
    RawBits[0] = pBlock->PackedData[1] & (0xFFFE);  // 15 bits (shifted up by one)
    RawBits[1] = pBlock->PackedData[1] >> 16;       // 16 bits

    // step through both colours
    for(KDint i = 0; i < 2; i++)
    {
        // If completely opaque
        if(RawBits[i] & (1 << 15))
        {
            // Extract R and G (both 5 bit)
            ABColours[i][0] = (RawBits[i] >> 10) & 0x1F;
            ABColours[i][1] = (RawBits[i] >> 5) & 0x1F;

            /*
             The precision of Blue depends on  A or B. If A then we need to
             replicate the top bit to get 5 bits in total
             */
            ABColours[i][2] = RawBits[i] & 0x1F;
            if(i == 0)
            {
                ABColours[0][2] |= ABColours[0][2] >> 4;
            }

            // set 4bit alpha fully on...
            ABColours[i][3] = 0xF;
        }
        else  // Else if colour has variable translucency
        {
            /*
             Extract R and G (both 4 bit).
             (Leave a space on the end for the replication of bits
             */
            ABColours[i][0] = (RawBits[i] >> (8 - 1)) & 0x1E;
            ABColours[i][1] = (RawBits[i] >> (4 - 1)) & 0x1E;

            // replicate bits to truly expand to 5 bits
            ABColours[i][0] |= ABColours[i][0] >> 4;
            ABColours[i][1] |= ABColours[i][1] >> 4;

            // grab the 3(+padding) or 4 bits of blue and add an extra padding bit
            ABColours[i][2] = (KDint)(RawBits[i] & 0xF) << 1;

            /*
             expand from 3 to 5 bits if this is from colour A, or 4 to 5 bits if from
             colour B
             */
            if(i == 0)
            {
                ABColours[0][2] |= ABColours[0][2] >> 3;
            }
            else
            {
                ABColours[0][2] |= ABColours[0][2] >> 4;
            }

            // Set the alpha bits to be 3 + a zero on the end
            ABColours[i][3] = (RawBits[i] >> 11) & 0xE;
        }
    }
}

/******************************************************************************
 * UnpackModulations: Given the block and the texture type and it's relative
 * position in the 2x2 group of blocks, extract the bit patterns for the fully 
 * defined pixels.
 ******************************************************************************/
static void UnpackModulations(const AMTC_BLOCK_STRUCT *pBlock, const KDint Do2bitMode, KDint ModulationVals[8][16], KDint ModulationModes[8][16], KDint StartX, KDint StartY)
{
    KDint BlockModMode;
    KDuint32 ModulationBits;

    KDint x;
    KDint y;

    BlockModMode = pBlock->PackedData[1] & 1;
    ModulationBits = pBlock->PackedData[0];

    // if it's in an interpolated mode
    if(Do2bitMode && BlockModMode)
    {
        /*
         run through all the pixels in the block. Note we can now treat all the
         "stored" values as if they have 2bits (even when they didn't!)
         */
        for(y = 0; y < BLK_Y_SIZE; y++)
        {
            for(x = 0; x < BLK_X_2BPP; x++)
            {
                ModulationModes[y + StartY][x + StartX] = BlockModMode;

                // if this is a stored value...
                if(((x ^ y) & 1) == 0)
                {
                    ModulationVals[y + StartY][x + StartX] = ModulationBits & 3;
                    ModulationBits >>= 2;
                }
            }
        }
    }
    else if(Do2bitMode)  // else if direct encoded 2bit mode - i.e. 1 mode bit per pixel
    {
        for(y = 0; y < BLK_Y_SIZE; y++)
        {
            for(x = 0; x < BLK_X_2BPP; x++)
            {
                ModulationModes[y + StartY][x + StartX] = BlockModMode;

                // double the bits so 0=> 00, and 1=>11
                if(ModulationBits & 1)
                {
                    ModulationVals[y + StartY][x + StartX] = 0x3;
                }
                else
                {
                    ModulationVals[y + StartY][x + StartX] = 0x0;
                }
                ModulationBits >>= 1;
            }
        }
    }
    else  // else its the 4bpp mode so each value has 2 bits
    {
        for(y = 0; y < BLK_Y_SIZE; y++)
        {
            for(x = 0; x < BLK_X_4BPP; x++)
            {
                ModulationModes[y + StartY][x + StartX] = BlockModMode;

                ModulationVals[y + StartY][x + StartX] = ModulationBits & 3;
                ModulationBits >>= 2;
            }
        }
    }

    // make sure nothing is left over
    kdAssert(ModulationBits == 0);
}

/******************************************************************************
 * InterpolateColours: This performs a HW bit accurate interpolation of either 
 * the A or B colours for a particular pixel.
 *
 * NOTE: It is assumed that the source colours are in ARGB 5554 format. 
 * This means that some "preparation" of the values will be necessary.
 ******************************************************************************/
static void InterpolateColours(const KDint ColourP[4], const KDint ColourQ[4], const KDint ColourR[4], const KDint ColourS[4], const KDint Do2bitMode, const KDint x, const KDint y, KDint Result[4])
{
    KDint u;
    KDint v;
    KDint uscale;
    KDint k;

    KDint tmp1;
    KDint tmp2;

    KDint P[4];
    KDint Q[4];
    KDint R[4];
    KDint S[4];

    // Copy the colours
    for(k = 0; k < 4; k++)
    {
        P[k] = ColourP[k];
        Q[k] = ColourQ[k];
        R[k] = ColourR[k];
        S[k] = ColourS[k];
    }

    // put the x and y values into the right range
    v = (y & 0x3) | ((~y & 0x2) << 1);

    if(Do2bitMode)
    {
        u = (x & 0x7) | ((~x & 0x4) << 1);
    }
    else
    {
        u = (x & 0x3) | ((~x & 0x2) << 1);
    }

    // get the u and v scale amounts
    v = v - BLK_Y_SIZE / 2;

    if(Do2bitMode)
    {
        u = u - BLK_X_2BPP / 2;
        uscale = 8;
    }
    else
    {
        u = u - BLK_X_4BPP / 2;
        uscale = 4;
    }

    for(k = 0; k < 4; k++)
    {
        tmp1 = P[k] * uscale + u * (Q[k] - P[k]);
        tmp2 = R[k] * uscale + u * (S[k] - R[k]);

        tmp1 = tmp1 * 4 + v * (tmp2 - tmp1);

        Result[k] = tmp1;
    }

    // Lop off the appropriate number of bits to get us to 8 bit precision
    if(Do2bitMode)
    {
        // do RGB
        for(k = 0; k < 3; k++)
        {
            Result[k] >>= 2;
        }

        Result[3] >>= 1;
    }
    else
    {
        // do RGB  (A is ok)
        for(k = 0; k < 3; k++)
        {
            Result[k] >>= 1;
        }
    }

    // sanity check
    for(k = 0; k < 4; k++)
    {
        kdAssert(Result[k] < 256);
    }


    /*
     Convert from 5554 to 8888
     
     do RGB 5.3 => 8
     */
    for(k = 0; k < 3; k++)
    {
        Result[k] += Result[k] >> 5;
    }

    Result[3] += Result[3] >> 4;

    // 2nd sanity check
    for(k = 0; k < 4; k++)
    {
        kdAssert(Result[k] < 256);
    }
}

/******************************************************************************
 * GetModulationValue: Get the modulation value as a numerator of 
 * a fraction of 8ths
 ******************************************************************************/
static void GetModulationValue(KDint x, KDint y, const KDint Do2bitMode, const KDint ModulationVals[8][16], const KDint ModulationModes[8][16], KDint *Mod, KDint *DoPT)
{
    static const KDint RepVals0[4] = {0, 3, 5, 8};
    static const KDint RepVals1[4] = {0, 4, 4, 8};

    KDint ModVal;

    // Map X and Y into the local 2x2 block
    y = (y & 0x3) | ((~y & 0x2) << 1);

    if(Do2bitMode)
    {
        x = (x & 0x7) | ((~x & 0x4) << 1);
    }
    else
    {
        x = (x & 0x3) | ((~x & 0x2) << 1);
    }

    // assume no PT for now
    *DoPT = 0;

    // extract the modulation value. If a simple encoding
    if(ModulationModes[y][x] == 0)
    {
        ModVal = RepVals0[ModulationVals[y][x]];
    }
    else if(Do2bitMode)
    {
        // if this is a stored value
        if(((x ^ y) & 1) == 0)
        {
            ModVal = RepVals0[ModulationVals[y][x]];
        }
        else if(ModulationModes[y][x] == 1)  // else average from the neighbours if H&V interpolation..
        {
            ModVal = (RepVals0[ModulationVals[y - 1][x]] +
                         RepVals0[ModulationVals[y + 1][x]] +
                         RepVals0[ModulationVals[y][x - 1]] +
                         RepVals0[ModulationVals[y][x + 1]] + 2) /
                4;
        }
        else if(ModulationModes[y][x] == 2)  // else if H-Only
        {
            ModVal = (RepVals0[ModulationVals[y][x - 1]] +
                         RepVals0[ModulationVals[y][x + 1]] + 1) /
                2;
        }
        else  // else it's V-Only
        {
            ModVal = (RepVals0[ModulationVals[y - 1][x]] +
                         RepVals0[ModulationVals[y + 1][x]] + 1) /
                2;
        }
    }
    else  // else it's 4BPP and PT encoding
    {
        ModVal = RepVals1[ModulationVals[y][x]];

        *DoPT = ModulationVals[y][x] == PT_INDEX;
    }

    *Mod = ModVal;
}

/******************************************************************************
 * TwiddleUV: Given the Block (or pixel) coordinates and the dimension of
 * the texture in blocks (or pixels) this returns the twiddled
 * offset of the block (or pixel) from the start of the map.
 * 
 * NOTE: The dimensions of the texture must be a power of 2
 ******************************************************************************/
static KDint DisableTwiddlingRoutine = 0;

static KDint PowerOfTwo(KDint input)
{
    KDint minus1;

    if(!input)
    {
        return 0;
    }
    minus1 = input - 1;
    return ((input | minus1) == (input ^ minus1)) ? 1 : 0;
}

static KDint TwiddleUV(KDint YSize, KDint XSize, KDint YPos, KDint XPos)
{
    KDint Twiddled;

    KDint MinDimension;
    KDint MaxValue;

    KDint SrcBitPos;
    KDint DstBitPos;

    KDint ShiftCount;

    kdAssert(YPos < YSize);
    kdAssert(XPos < XSize);

    kdAssert(PowerOfTwo(YSize));
    kdAssert(PowerOfTwo(XSize));


    if(YSize < XSize)
    {
        MinDimension = YSize;
        MaxValue = XPos;
    }
    else
    {
        MinDimension = XSize;
        MaxValue = YPos;
    }

    // Nasty hack to disable twiddling
    if(DisableTwiddlingRoutine)
    {
        return (YPos * XSize + XPos);
    }

    // Step through all the bits in the "minimum" dimension
    SrcBitPos = 1;
    DstBitPos = 1;
    Twiddled = 0;
    ShiftCount = 0;

    while(SrcBitPos < MinDimension)
    {
        if(YPos & SrcBitPos)
        {
            Twiddled |= DstBitPos;
        }

        if(XPos & SrcBitPos)
        {
            Twiddled |= (DstBitPos << 1);
        }


        SrcBitPos <<= 1;
        DstBitPos <<= 2;
        ShiftCount += 1;
    }

    // prepend any unused bits
    MaxValue >>= ShiftCount;

    Twiddled |= (MaxValue << (2 * ShiftCount));

    return Twiddled;
}

/******************************************************************************
 * PVRDecompress: Decompresses PVRTC to RGBA 8888
 ******************************************************************************/
static void PVRDecompress(AMTC_BLOCK_STRUCT *pCompressedData, const KDboolean Do2bitMode, const KDint XDim, const KDint YDim, const KDint AssumeImageTiles, KDuint8 *pResultImage)
{
    KDint x;
    KDint y;
    KDint i;
    KDint j;

    KDint BlkX;
    KDint BlkY;
    KDint BlkXp1;
    KDint BlkYp1;
    KDint XBlockSize;
    KDint BlkXDim;
    KDint BlkYDim;

    KDint StartX;
    KDint StartY;

    KDint ModulationVals[8][16];
    KDint ModulationModes[8][16];

    KDint Mod;
    KDint DoPT;

    KDint uPosition;

    // local neighbourhood of blocks
    AMTC_BLOCK_STRUCT *pBlocks[2][2];

    AMTC_BLOCK_STRUCT *pPrevious[2][2] = {{KD_NULL, KD_NULL}, {KD_NULL, KD_NULL}};

    // Low precision colours extracted from the blocks
    struct
    {
        KDint Reps[2][4];
    } Colours5554[2][2];

    // Interpolated A and B colours for the pixel
    KDint ASig[4];
    KDint BSig[4];

    KDint Result[4];

    if(Do2bitMode)
    {
        XBlockSize = BLK_X_2BPP;
    }
    else
    {
        XBlockSize = BLK_X_4BPP;
    }

    // For MBX don't allow the sizes to get too small
    BlkXDim = kdMaxVEN(2, XDim / XBlockSize);
    BlkYDim = kdMaxVEN(2, YDim / BLK_Y_SIZE);

    /*
     Step through the pixels of the image decompressing each one in turn
     
     Note that this is a hideously inefficient way to do this!
     */
    for(y = 0; y < YDim; y++)
    {
        for(x = 0; x < XDim; x++)
        {
            // map this pixel to the top left neighbourhood of blocks
            BlkX = (x - XBlockSize / 2);
            BlkY = (y - BLK_Y_SIZE / 2);

            BlkX = LIMIT_COORD(BlkX, XDim, AssumeImageTiles);
            BlkY = LIMIT_COORD(BlkY, YDim, AssumeImageTiles);


            BlkX /= XBlockSize;
            BlkY /= BLK_Y_SIZE;

            // compute the positions of the other 3 blocks
            BlkXp1 = LIMIT_COORD(BlkX + 1, BlkXDim, AssumeImageTiles);
            BlkYp1 = LIMIT_COORD(BlkY + 1, BlkYDim, AssumeImageTiles);

            // Map to block memory locations
            pBlocks[0][0] = pCompressedData + TwiddleUV(BlkYDim, BlkXDim, BlkY, BlkX);
            pBlocks[0][1] = pCompressedData + TwiddleUV(BlkYDim, BlkXDim, BlkY, BlkXp1);
            pBlocks[1][0] = pCompressedData + TwiddleUV(BlkYDim, BlkXDim, BlkYp1, BlkX);
            pBlocks[1][1] = pCompressedData + TwiddleUV(BlkYDim, BlkXDim, BlkYp1, BlkXp1);


            /*
             extract the colours and the modulation information IF the previous values
             have changed.
             */
            if(kdMemcmp(pPrevious, pBlocks, 4 * sizeof(void *)) != 0)
            {
                StartY = 0;
                for(i = 0; i < 2; i++)
                {
                    StartX = 0;
                    for(j = 0; j < 2; j++)
                    {
                        Unpack5554Colour(pBlocks[i][j], Colours5554[i][j].Reps);

                        UnpackModulations(pBlocks[i][j],
                            Do2bitMode,
                            ModulationVals,
                            ModulationModes,
                            StartX, StartY);

                        StartX += XBlockSize;
                    }

                    StartY += BLK_Y_SIZE;
                }

                // make a copy of the new pointers
                kdMemcpy(pPrevious, pBlocks, 4 * sizeof(void *));
            }

            // decompress the pixel.  First compute the interpolated A and B signals
            InterpolateColours(Colours5554[0][0].Reps[0],
                Colours5554[0][1].Reps[0],
                Colours5554[1][0].Reps[0],
                Colours5554[1][1].Reps[0],
                Do2bitMode, x, y,
                ASig);

            InterpolateColours(Colours5554[0][0].Reps[1],
                Colours5554[0][1].Reps[1],
                Colours5554[1][0].Reps[1],
                Colours5554[1][1].Reps[1],
                Do2bitMode, x, y,
                BSig);

            GetModulationValue(x, y, Do2bitMode, (const KDint(*)[16])ModulationVals, (const KDint(*)[16])ModulationModes,
                &Mod, &DoPT);

            // compute the modulated colour
            for(i = 0; i < 4; i++)
            {
                Result[i] = ASig[i] * 8 + Mod * (BSig[i] - ASig[i]);
                Result[i] >>= 3;
            }

            if(DoPT)
            {
                Result[3] = 0;
            }

            // Store the result in the output image
            uPosition = (x + y * XDim) << 2;
            pResultImage[uPosition + 0] = (KDuint8)Result[0];
            pResultImage[uPosition + 1] = (KDuint8)Result[1];
            pResultImage[uPosition + 2] = (KDuint8)Result[2];
            pResultImage[uPosition + 3] = (KDuint8)Result[3];
        }
    }
}

KDint __kdDecompressPVRTC(const KDuint8 *pCompressedData, KDboolean Do2bitMode, KDint XDim, KDint YDim, KDuint8 *pResultImage)
{
    AMTC_BLOCK_STRUCT *dataptr = KD_NULL;
    kdMemcpy(&dataptr, &pCompressedData, sizeof(pCompressedData));
    PVRDecompress(dataptr, Do2bitMode, XDim, YDim, 1, pResultImage);
    return XDim * YDim / 2;
}
