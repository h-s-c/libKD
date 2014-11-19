/*******************************************************
 * OpenKODE Core extension: KD_EXT_platform_window
 *******************************************************/
#ifndef __kd_EXT_platform_window_h_
#define __kd_EXT_platform_window_h_
#include <KD/kd.h>
#include <EGL/eglext.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifdef KD_WINDOW_SUPPORTED
/* kdPumpPlatformEvents: Pump the thread's event queue, performing callbacks. */
KD_API KDint KD_APIENTRY kdPumpPlatformEvents(EGLenum platform);

/* kdCreatePlatformWindow: Create a window. */
KD_API KDWindow *KD_APIENTRY kdCreatePlatformWindow(EGLenum platform, EGLDisplay display, EGLConfig config, void *eventuserptr);

/* kdRealizeWindow: Realize the window as a displayable entity and get the native handles for passing to EGL. */
KD_API KDint KD_APIENTRY kdRealizePlatformWindow(KDWindow *window, void *nativewindow, void *nativedisplay);
#endif

#ifdef __cplusplus
}
#endif

#endif /* __kd_EXT_platform_window_h_ */