/*
 * Copyright (c) 2007 NVIDIA Corporation.  All rights reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA Corporation is strictly prohibited.
 */


#ifndef __kd_NV_extwindowprops_h_
#define __kd_NV_extwindowprops_h_
#include <KD/kd.h>
#include <EGL/eglext.h>

#ifdef __cplusplus
extern "C" {
#endif



/* KD_WINDOWPROPERTY_FULLSCREEN: Control over resizing a window to fill the complete screen */

// KDboolean - set the window to fullscreen mode
#define KD_WINDOWPROPERTY_FULLSCREEN_NV 9999

// KDint - set which KD_DISPLAY_* display that the window should be opened on
#define KD_WINDOWPROPERTY_DISPLAY_NV    9998

// KDboolean - sets whether overlay should be used to create window
#define KD_WINDOWPROPERTY_OVERLAY_NV    9997

#define KD_DISPLAY_PRIMARY_NV           0
#define KD_DISPLAY_INTERNAL0_NV         0
#define KD_DISPLAY_INTERNAL1_NV         1
#define KD_DISPLAY_INTERNAL2_NV         2
#define KD_DISPLAY_INTERNAL3_NV         3
#define KD_DISPLAY_EXTERNAL0_NV         1000
#define KD_DISPLAY_EXTERNAL1_NV         1001
#define KD_DISPLAY_EXTERNAL2_NV         1002
#define KD_DISPLAY_EXTERNAL3_NV         1003


#ifdef __cplusplus
}
#endif

#endif /* __kd_NV_extwindowprops_h_ */
