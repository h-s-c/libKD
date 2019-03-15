// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com

/******************************************************************************
 * libKD
 * zlib/libpng License
 ******************************************************************************
 * Copyright (c) 2014-2019 Kevin Schmidt
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
 * Implementation notes
 *
 * - Only one window is supported
 * - To receive orientation changes AndroidManifest.xml should include
 *   android:configChanges="orientation|keyboardHidden|screenSize"
 *
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
#include <KD/kd.h>
#include "KD/ATX_keyboard.h"        // for KDEventInputKeyATX, KD_KEY_...
#include "KD/KHR_thread_storage.h"  // for kdSetThreadStorageKHR
#include "KD/NV_extwindowprops.h"   // for KD_WINDOWPROPERTY_FULLSCREE...
#include <KD/kdext.h>               // for kdGetEnvVEN, kdStrstrVEN
#if defined(__clang__)
#pragma clang diagnostic pop
#endif

#if defined(KD_WINDOW_SUPPORTED)
#include <EGL/egl.h>     // for EGLConfig, EGLDisplay, EGL_...
#include <EGL/eglext.h>  // for EGL_PLATFORM_WAYLAND_KHR
#endif

#include "kd_internal.h"  // for _KDCallback, KDThread, __kd...

/******************************************************************************
 * C includes
 ******************************************************************************/

/* freestanding safe */
#include <stdlib.h>  // for exit, EXIT_FAILURE, EXIT_SU...

/******************************************************************************
 * Platform includes
 ******************************************************************************/

#if defined(__unix__) || defined(__APPLE__) || defined(__EMSCRIPTEN__)
// IWYU pragma: no_include  <bits/stdint-uintn.h>
#if !defined(__ANDROID__)
#include <dlfcn.h>     // for dlclose, dlerror, dlopen
#include <sys/mman.h>  // for mmap, munmap, MAP_SHARED
#endif
#if defined(__ANDROID__)
#include <android/input.h>            // for AInputEvent_getType, AInputQueue
#include <android/keycodes.h>         // for ::AKEYCODE_ALT_LEFT, ::AKEYCODE...
#include <android/native_activity.h>  // for ANativeActivity, ANativeActivit...
#include <android/native_window.h>    // for ANativeWindow, ANativeWindow_se...
#include <android/window.h>           // for ::AWINDOW_FLAG_KEEP_SCREEN_ON
#endif
#if defined(__EMSCRIPTEN__)
#include <emscripten/emscripten.h>
#include <emscripten/html5.h>
#endif
#if defined(KD_WINDOW_X11) || defined(KD_WINDOW_WAYLAND)
#ifndef EGL_PLATFORM_X11_KHR
#define EGL_PLATFORM_X11_KHR 0x31D5
#endif
#ifndef EGL_PLATFORM_WAYLAND_KHR
#define EGL_PLATFORM_WAYLAND_KHR 0x31D8
#endif
#include <xcb/xcb.h>                      // for xcb_flush, xcb_connect, xcb...
#include <xkbcommon/xkbcommon-keysyms.h>  // for XKB_KEY_Alt_L, XKB_KEY_Alt_R
#include <xkbcommon/xkbcommon-names.h>    // for XKB_MOD_NAME_ALT, XKB_MOD_N...
#include <xkbcommon/xkbcommon.h>          // for xkb_state_mod_name_is_active
#endif
#if defined(KD_WINDOW_WAYLAND)
// IWYU pragma: no_include <wayland-client-core.h>
// IWYU pragma: no_include <wayland-client-protocol.h>
// IWYU pragma: no_include <wayland-egl-core.h>
// IWYU pragma: no_include <wayland-util.h>
// IWYU pragma: no_include <wayland-version.h>
#include <wayland-client.h>  // IWYU pragma: keep
// IWYU pragma: no_forward_declare wl_keyboard
// IWYU pragma: no_forward_declare wl_pointer
// IWYU pragma: no_forward_declare wl_registry
// IWYU pragma: no_forward_declare wl_seat
// IWYU pragma: no_forward_declare wl_shell_surface
// IWYU pragma: no_forward_declare wl_surface
#include <wayland-egl.h>  // IWYU pragma: keep
#endif
#if defined(KD_WINDOW_X11)
#include <xcb/xcb_ewmh.h>             // for xcb_ewmh_connection_t, xcb_...
#include <xcb/xcb_icccm.h>            // for xcb_icccm_set_wm_name, xcb_...
#include <xcb/xkb.h>                  // for xcb_xkb_state_notify_event_t
#include <xcb/xproto.h>               // for xcb_intern_atom, xcb_intern...
#include <xkbcommon/xkbcommon-x11.h>  // for xkb_x11_get_core_keyboard_d...
#endif
#endif

#if defined(_WIN32)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <winsock2.h> /* WSA.. */
#endif

/******************************************************************************
 * Events
 ******************************************************************************/

/* kdWaitEvent: Get next event from thread's event queue. */
KD_API const KDEvent *KD_APIENTRY kdWaitEvent(KDust timeout)
{
    _KDQueue *eventqueue = kdThreadSelf()->eventqueue;
    KDEvent *lastevent = kdThreadSelf()->lastevent;
    if(lastevent)
    {
        kdFreeEvent(lastevent);
    }
    if(timeout != -1)
    {
        kdThreadSleepVEN(timeout);
    }
    kdPumpEvents();
    if(__kdQueueSize(eventqueue) > 0)
    {
        lastevent = (KDEvent *)__kdQueuePull(eventqueue);
    }
    else
    {
        lastevent = KD_NULL;
        kdSetError(KD_EAGAIN);
    }
    return lastevent;
}

/* kdSetEventUserptr: Set the userptr for global events. */
static void *__kd_userptr = KD_NULL;
static KDThreadMutex *__kd_userptrmtx = KD_NULL;
KD_API void KD_APIENTRY kdSetEventUserptr(KD_UNUSED void *userptr)
{
    kdThreadMutexLock(__kd_userptrmtx);
    __kd_userptr = userptr;
    kdThreadMutexUnlock(__kd_userptrmtx);
}

/* kdDefaultEvent: Perform default processing on an unrecognized event. */
KD_API void KD_APIENTRY kdDefaultEvent(KD_UNUSED const KDEvent *event)
{
}

/* kdPumpEvents: Pump the thread's event queue, performing callbacks. */
struct _KDCallback {
    KDCallbackFunc *func;
    void *eventuserptr;
    KDint eventtype;
    KDint8 padding[4];
};
static KDboolean __kdExecCallback(KDEvent *event)
{
    KDint callbackindex = kdThreadSelf()->callbackindex;
    _KDCallback **callbacks = kdThreadSelf()->callbacks;
    for(KDint i = 0; i < callbackindex; i++)
    {
        if(callbacks[i]->func)
        {
            KDboolean typematch = (callbacks[i]->eventtype == event->type) || (callbacks[i]->eventtype == 0);
            KDboolean userptrmatch = (callbacks[i]->eventuserptr == event->userptr);
            if(typematch && userptrmatch)
            {
                callbacks[i]->func(event);
                kdFreeEvent(event);
                return KD_TRUE;
            }
        }
    }
    return KD_FALSE;
}

#ifdef KD_WINDOW_SUPPORTED
struct KDWindow {
    void *nativewindow;
    void *nativedisplay;
    EGLenum platform;
    EGLint format;
    void *eventuserptr;
    KDThread *originthr;
    struct
    {
        KDchar caption[256];
        KDboolean focused;
        KDboolean visible;
        KDboolean fullscreen;
        KDint32 width;
        KDint32 height;
    } properties;
    struct
    {
        struct
        {
            KDint32 availability;
            KDint32 select;
            KDint32 x;
            KDint32 y;
        } pointer;
        struct
        {
            KDint32 availability;
            KDint32 character;
            KDint32 keycode;
            KDuint32 flags;
            KDuint32 charflags;
        } keyboard;
        struct
        {
            KDint32 availability;
            KDint32 up;
            KDint32 left;
            KDint32 right;
            KDint32 down;
            KDint32 select;
        } dpad;
        struct
        {
            KDint32 availability;
            KDint32 up;
            KDint32 left;
            KDint32 right;
            KDint32 down;
            KDint32 fire;
        } gamekeys;
    } states;
#if defined(KD_WINDOW_WAYLAND) || defined(KD_WINDOW_X11)
    struct
    {
        struct xkb_context *context;
        struct xkb_state *state;
        struct xkb_keymap *keymap;
        KDuint8 firstevent;
        KDint8 padding[7];
    } xkb;
#endif
#if defined(KD_WINDOW_WAYLAND)
    struct
    {
        struct wl_surface *surface;
        struct wl_shell_surface *shell_surface;
        struct wl_registry *registry;
        struct wl_compositor *compositor;
        struct wl_shell *shell;
        struct wl_seat *seat;
        struct wl_keyboard *keyboard;
        struct wl_pointer *pointer;
    } wayland;
#endif
#if defined(KD_WINDOW_WAYLAND)
    struct
    {
        xcb_ewmh_connection_t ewmh;
    } xcb;
#endif
};

static KDWindow *__kd_window = KD_NULL;

#ifdef __ANDROID__
static AInputQueue *__kd_androidinputqueue = KD_NULL;
static KDThreadMutex *__kd_androidinputqueue_mutex = KD_NULL;
#endif

#if defined(KD_WINDOW_WAYLAND)
static struct wl_display *__kd_wl_display;
#endif

static void __kdHandleSpecialKeys(KDWindow *window, KDEventInputKeyATX *keyevent)
{
    KDEvent *dpadevent = kdCreateEvent();
    dpadevent->type = KD_EVENT_INPUT;
    KDEvent *gamekeysevent = kdCreateEvent();
    gamekeysevent->type = KD_EVENT_INPUT;

    switch(keyevent->keycode)
    {
        case(KD_KEY_UP_ATX):
        {
            window->states.dpad.up = keyevent->flags & KD_KEY_PRESS_ATX;
            dpadevent->data.input.index = KD_INPUT_DPAD_UP;
            dpadevent->data.input.value.i = window->states.dpad.up;

            window->states.gamekeys.up = keyevent->flags & KD_KEY_PRESS_ATX;
            gamekeysevent->data.input.index = KD_INPUT_GAMEKEYS_UP;
            gamekeysevent->data.input.value.i = window->states.gamekeys.up;
            break;
        }
        case(KD_KEY_LEFT_ATX):
        {
            window->states.dpad.left = keyevent->flags & KD_KEY_PRESS_ATX;
            dpadevent->data.input.index = KD_INPUT_DPAD_LEFT;
            dpadevent->data.input.value.i = window->states.dpad.left;

            window->states.gamekeys.left = keyevent->flags & KD_KEY_PRESS_ATX;
            gamekeysevent->data.input.index = KD_INPUT_GAMEKEYS_LEFT;
            gamekeysevent->data.input.value.i = window->states.gamekeys.left;
            break;
        }
        case(KD_KEY_RIGHT_ATX):
        {
            window->states.dpad.right = keyevent->flags & KD_KEY_PRESS_ATX;
            dpadevent->data.input.index = KD_INPUT_DPAD_RIGHT;
            dpadevent->data.input.value.i = window->states.dpad.right;

            window->states.gamekeys.right = keyevent->flags & KD_KEY_PRESS_ATX;
            gamekeysevent->data.input.index = KD_INPUT_GAMEKEYS_RIGHT;
            gamekeysevent->data.input.value.i = window->states.gamekeys.right;
            break;
        }
        case(KD_KEY_DOWN_ATX):
        {
            window->states.dpad.down = keyevent->flags & KD_KEY_PRESS_ATX;
            dpadevent->data.input.index = KD_INPUT_DPAD_DOWN;
            dpadevent->data.input.value.i = window->states.dpad.down;

            window->states.gamekeys.down = keyevent->flags & KD_KEY_PRESS_ATX;
            gamekeysevent->data.input.index = KD_INPUT_GAMEKEYS_DOWN;
            gamekeysevent->data.input.value.i = window->states.gamekeys.down;
            break;
        }
        case(KD_KEY_ENTER_ATX):
        {
            window->states.dpad.select = keyevent->flags & KD_KEY_PRESS_ATX;
            dpadevent->data.input.index = KD_INPUT_DPAD_SELECT;
            dpadevent->data.input.value.i = window->states.dpad.select;

            window->states.gamekeys.fire = keyevent->flags & KD_KEY_PRESS_ATX;
            gamekeysevent->data.input.index = KD_INPUT_GAMEKEYS_FIRE;
            gamekeysevent->data.input.value.i = window->states.gamekeys.fire;
            break;
        }
        default:
        {
            kdFreeEvent(dpadevent);
            kdFreeEvent(gamekeysevent);
            return;
        }
    }

    if(!__kdExecCallback(dpadevent))
    {
        kdPostEvent(dpadevent);
    }
    if(!__kdExecCallback(gamekeysevent))
    {
        kdPostEvent(gamekeysevent);
    }
}

#if defined(__ANDROID__)
/* Silence -Wunused-function */
static KD_UNUSED void (*__dummyfunc)(KDWindow *, KDEventInputKeyATX *) = &__kdHandleSpecialKeys;
#endif

static KDint32 __KDKeycodeLookup(KDint32 keycode)
{
    switch(keycode)
    {
#if defined(KD_WINDOW_ANDROID)
        /* KD_KEY_ACCEPT_ATX */
        /* KD_KEY_AGAIN_ATX */
        /* KD_KEY_ALLCANDIDATES_ATX */
        case(AKEYCODE_EISU):
        {
            return KD_KEY_ALPHANUMERIC_ATX;
        }
        case(AKEYCODE_ALT_LEFT):
        case(AKEYCODE_ALT_RIGHT):
        {
            return KD_KEY_ALT_ATX;
        }
        /* KD_KEY_ALTGRAPH_ATX
           KD_KEY_APPS_ATX
           KD_KEY_ATTN_ATX */
        case(AKEYCODE_BACK):
        {
            return KD_KEY_BROWSERBACK_ATX;
        }
        /* KD_KEY_BROWSERFAVORITES_ATX
           KD_KEY_BROWSERFORWARD_ATX
           KD_KEY_BROWSERHOME_ATX
           KD_KEY_BROWSERREFRESH_ATX
           KD_KEY_BROWSERSEARCH_ATX
           KD_KEY_BROWSERSTOP_ATX */
        case(AKEYCODE_CAPS_LOCK):
        {
            return KD_KEY_CAPSLOCK_ATX;
        }
        case(AKEYCODE_CLEAR):
        {
            return KD_KEY_CLEAR_ATX;
        }
        /* KD_KEY_CODEINPUT_ATX */
        /* KD_KEY_COMPOSE_ATX */
        case(AKEYCODE_CTRL_LEFT):
        case(AKEYCODE_CTRL_RIGHT):
        {
            return KD_KEY_CONTROL_ATX;
        }
        /* KD_KEY_CRSEL_ATX
           KD_KEY_CONVERT_ATX */
        case(AKEYCODE_COPY):
        {
            return KD_KEY_COPY_ATX;
        }
        case(AKEYCODE_CUT):
        {
            return KD_KEY_CUT_ATX;
        }
        case(AKEYCODE_DPAD_DOWN):
        {
            return KD_KEY_DOWN_ATX;
        }
        /* KD_KEY_END_ATX */
        case(AKEYCODE_ENTER):
        {
            return KD_KEY_ENTER_ATX;
        }
        /* KD_KEY_ERASEEOF_ATX
           KD_KEY_EXECUTE_ATX
           KD_KEY_EXSEL_ATX */
        case(AKEYCODE_F1):
        {
            return KD_KEY_F1_ATX;
        }
        case(AKEYCODE_F2):
        {
            return KD_KEY_F2_ATX;
        }
        case(AKEYCODE_F3):
        {
            return KD_KEY_F3_ATX;
        }
        case(AKEYCODE_F4):
        {
            return KD_KEY_F4_ATX;
        }
        case(AKEYCODE_F5):
        {
            return KD_KEY_F5_ATX;
        }
        case(AKEYCODE_F6):
        {
            return KD_KEY_F6_ATX;
        }
        case(AKEYCODE_F7):
        {
            return KD_KEY_F7_ATX;
        }
        case(AKEYCODE_F8):
        {
            return KD_KEY_F8_ATX;
        }
        case(AKEYCODE_F9):
        {
            return KD_KEY_F9_ATX;
        }
        case(AKEYCODE_F10):
        {
            return KD_KEY_F10_ATX;
        }
        case(AKEYCODE_F11):
        {
            return KD_KEY_F11_ATX;
        }
        case(AKEYCODE_F12):
        {
            return KD_KEY_F12_ATX;
        }
        /* KD_KEY_F13_ATX
           KD_KEY_F14_ATX
           KD_KEY_F15_ATX
           KD_KEY_F16_ATX
           KD_KEY_F17_ATX
           KD_KEY_F18_ATX
           KD_KEY_F19_ATX
           KD_KEY_F20_ATX
           KD_KEY_F21_ATX
           KD_KEY_F22_ATX
           KD_KEY_F23_ATX
           KD_KEY_F24_ATX
           KD_KEY_FINALMODE_ATX
           KD_KEY_FIND_ATX
           KD_KEY_FULLWIDTH_ATX
           KD_KEY_HALFWIDTH_ATX
           KD_KEY_HANGULMODE_ATX
           KD_KEY_HANJAMODE_ATX */
        case(AKEYCODE_HELP):
        {
            return KD_KEY_HELP_ATX;
        }
        /* KD_KEY_HIRAGANA_ATX */
        case(AKEYCODE_HOME):
        {
            return KD_KEY_HOME_ATX;
        }
        case(AKEYCODE_DEL):
        {
            return KD_KEY_INSERT_ATX;
        }
        /* KD_KEY_JAPANESEHIRAGANA_ATX
           KD_KEY_JAPANESEKATAKANA_ATX
           KD_KEY_JAPANESEROMAJI_ATX
           KD_KEY_JUNJAMODE_ATX
           KD_KEY_KANAMODE_ATX
           KD_KEY_KANJIMODE_ATX
           KD_KEY_KATAKANA_ATX */
        /* KD_KEY_LAUNCHAPPLICATION1_ATX
           KD_KEY_LAUNCHAPPLICATION2_ATX */
        case(AKEYCODE_ENVELOPE):
        {
            return KD_KEY_LAUNCHMAIL_ATX;
        }
        case(AKEYCODE_SOFT_LEFT):
        case(AKEYCODE_DPAD_LEFT):
        {
            return KD_KEY_LEFT_ATX;
        }
        /* KD_KEY_META_ATX */
        case(AKEYCODE_MEDIA_NEXT):
        {
            return KD_KEY_MEDIANEXTTRACK_ATX;
        }
        case(AKEYCODE_MEDIA_PLAY_PAUSE):
        {
            return KD_KEY_MEDIAPLAYPAUSE_ATX;
        }
        case(AKEYCODE_MEDIA_PREVIOUS):
        {
            return KD_KEY_MEDIAPREVIOUSTRACK_ATX;
        }
        case(AKEYCODE_MEDIA_STOP):
        {
            return KD_KEY_MEDIASTOP_ATX;
        }
        /* KD_KEY_MODECHANGE_ATX
           KD_KEY_NONCONVERT_ATX */
        case(AKEYCODE_NUM_LOCK):
        {
            return KD_KEY_NUMLOCK_ATX;
        }
        case(AKEYCODE_PAGE_DOWN):
        {
            return KD_KEY_PAGEDOWN_ATX;
        }
        case(AKEYCODE_PAGE_UP):
        {
            return KD_KEY_PAGEUP_ATX;
        }
        case(AKEYCODE_MEDIA_PAUSE):
        {
            return KD_KEY_PAUSE_ATX;
        }
        case(AKEYCODE_MEDIA_PLAY):
        {
            return KD_KEY_PLAY_ATX;
        }
        /* KD_KEY_PREVIOUSCANDIDATE_ATX */
        case(AKEYCODE_SYSRQ):
        {
            return KD_KEY_PRINTSCREEN_ATX;
        }
        /* KD_KEY_PROCESS_ATX
           KD_KEY_PROPS_ATX */
        case(AKEYCODE_SOFT_RIGHT):
        case(AKEYCODE_DPAD_RIGHT):
        {
            return KD_KEY_RIGHT_ATX;
        }
        /* KD_KEY_ROMANCHARACTERS_ATX */
        case(AKEYCODE_MOVE_END):
        {
            return KD_KEY_SCROLL_ATX;
        }
        case(AKEYCODE_BUTTON_SELECT):
        {
            return KD_KEY_SELECT_ATX;
        }
        case(AKEYCODE_MEDIA_TOP_MENU):
        {
            return KD_KEY_SELECTMEDIA_ATX;
        }
        case(AKEYCODE_SHIFT_LEFT):
        case(AKEYCODE_SHIFT_RIGHT):
        {
            return KD_KEY_SHIFT_ATX;
        }
        /* KD_KEY_STOP_ATX */
        case(AKEYCODE_DPAD_UP):
        {
            return KD_KEY_UP_ATX;
        }
        /* KD_KEY_UNDO_ATX */
        case(AKEYCODE_VOLUME_DOWN):
        {
            return KD_KEY_VOLUMEDOWN_ATX;
        }
        case(AKEYCODE_VOLUME_MUTE):
        {
            return KD_KEY_VOLUMEMUTE_ATX;
        }
        case(AKEYCODE_VOLUME_UP):
        {
            return KD_KEY_VOLUMEUP_ATX;
        }
        /* KD_KEY_WIN_ATX */
        case(AKEYCODE_ZOOM_IN):
        {
            return KD_KEY_ZOOM_ATX;
        }
#elif defined(KD_WINDOW_WIN32)
        case(VK_ACCEPT):
        {
            return KD_KEY_ACCEPT_ATX;
        }
        /* KD_KEY_AGAIN_ATX */
        /* KD_KEY_ALLCANDIDATES_ATX */
        /* KD_KEY_ALPHANUMERIC_ATX */
        case(VK_MENU):
        {
            return KD_KEY_ALT_ATX;
        }
        /* KD_KEY_ALTGRAPH_ATX */
        case(VK_APPS):
        {
            return KD_KEY_APPS_ATX;
        }
        case(VK_ATTN):
        {
            return KD_KEY_ATTN_ATX;
        }
        case(VK_BACK):
        {
            return KD_KEY_BROWSERBACK_ATX;
        }
        case(VK_BROWSER_FAVORITES):
        {
            return KD_KEY_BROWSERFAVORITES_ATX;
        }
        case(VK_BROWSER_FORWARD):
        {
            return KD_KEY_BROWSERFORWARD_ATX;
        }
        case(VK_BROWSER_HOME):
        {
            return KD_KEY_BROWSERHOME_ATX;
        }
        case(VK_BROWSER_REFRESH):
        {
            return KD_KEY_BROWSERREFRESH_ATX;
        }
        case(VK_BROWSER_SEARCH):
        {
            return KD_KEY_BROWSERSEARCH_ATX;
        }
        case(VK_BROWSER_STOP):
        {
            return KD_KEY_BROWSERSTOP_ATX;
        }
        case(VK_CAPITAL):
        {
            return KD_KEY_CAPSLOCK_ATX;
        }
        case(VK_CLEAR):
        case(VK_OEM_CLEAR):
        {
            return KD_KEY_CLEAR_ATX;
        }
        /* KD_KEY_CODEINPUT_ATX */
        /* KD_KEY_COMPOSE_ATX */
        case(VK_CONTROL):
        {
            return KD_KEY_CONTROL_ATX;
        }
        case(VK_CRSEL):
        {
            return KD_KEY_CRSEL_ATX;
        }
        case(VK_CONVERT):
        {
            return KD_KEY_CONVERT_ATX;
        }
        /* KD_KEY_COPY_ATX */
        /* KD_KEY_CUT_ATX */
        case(VK_DOWN):
        {
            return KD_KEY_DOWN_ATX;
        }
        case(VK_END):
        {
            return KD_KEY_END_ATX;
        }
        case(VK_RETURN):
        {
            return KD_KEY_ENTER_ATX;
        }
        case(VK_EREOF):
        {
            return KD_KEY_ERASEEOF_ATX;
        }
        case(VK_EXECUTE):
        {
            return KD_KEY_EXECUTE_ATX;
        }
        case(VK_EXSEL):
        {
            return KD_KEY_EXSEL_ATX;
        }
        case(VK_F1):
        {
            return KD_KEY_F1_ATX;
        }
        case(VK_F2):
        {
            return KD_KEY_F2_ATX;
        }
        case(VK_F3):
        {
            return KD_KEY_F3_ATX;
        }
        case(VK_F4):
        {
            return KD_KEY_F4_ATX;
        }
        case(VK_F5):
        {
            return KD_KEY_F5_ATX;
        }
        case(VK_F6):
        {
            return KD_KEY_F6_ATX;
        }
        case(VK_F7):
        {
            return KD_KEY_F7_ATX;
        }
        case(VK_F8):
        {
            return KD_KEY_F8_ATX;
        }
        case(VK_F9):
        {
            return KD_KEY_F9_ATX;
        }
        case(VK_F10):
        {
            return KD_KEY_F10_ATX;
        }
        case(VK_F11):
        {
            return KD_KEY_F11_ATX;
        }
        case(VK_F12):
        {
            return KD_KEY_F12_ATX;
        }
        case(VK_F13):
        {
            return KD_KEY_F13_ATX;
        }
        case(VK_F14):
        {
            return KD_KEY_F14_ATX;
        }
        case(VK_F15):
        {
            return KD_KEY_F15_ATX;
        }
        case(VK_F16):
        {
            return KD_KEY_F16_ATX;
        }
        case(VK_F17):
        {
            return KD_KEY_F17_ATX;
        }
        case(VK_F18):
        {
            return KD_KEY_F18_ATX;
        }
        case(VK_F19):
        {
            return KD_KEY_F19_ATX;
        }
        case(VK_F20):
        {
            return KD_KEY_F20_ATX;
        }
        case(VK_F21):
        {
            return KD_KEY_F21_ATX;
        }
        case(VK_F22):
        {
            return KD_KEY_F22_ATX;
        }
        case(VK_F23):
        {
            return KD_KEY_F23_ATX;
        }
        case(VK_F24):
        {
            return KD_KEY_F24_ATX;
        }
        case(VK_FINAL):
        {
            return KD_KEY_FINALMODE_ATX;
        }
        /* KD_KEY_FIND_ATX
           KD_KEY_FULLWIDTH_ATX
           KD_KEY_HALFWIDTH_ATX */
        case(VK_HANGUL):
        {
            return KD_KEY_HANGULMODE_ATX;
        }
        case(VK_HANJA):
        {
            return KD_KEY_HANJAMODE_ATX;
        }
        case(VK_HELP):
        {
            return KD_KEY_HELP_ATX;
        }
        /* KD_KEY_HIRAGANA_ATX */
        case(VK_HOME):
        {
            return KD_KEY_HOME_ATX;
        }
        case(VK_INSERT):
        {
            return KD_KEY_INSERT_ATX;
        }
        /* KD_KEY_JAPANESEHIRAGANA_ATX
           KD_KEY_JAPANESEKATAKANA_ATX
           KD_KEY_JAPANESEROMAJI_ATX
           KD_KEY_JUNJAMODE_ATX
           KD_KEY_KANAMODE_ATX
           KD_KEY_KANJIMODE_ATX
           KD_KEY_KATAKANA_ATX */
        case(VK_LAUNCH_APP1):
        {
            return KD_KEY_LAUNCHAPPLICATION1_ATX;
        }
        case(VK_LAUNCH_APP2):
        {
            return KD_KEY_LAUNCHAPPLICATION2_ATX;
        }
        case(VK_LAUNCH_MAIL):
        {
            return KD_KEY_LAUNCHMAIL_ATX;
        }
        case(VK_LEFT):
        {
            return KD_KEY_LEFT_ATX;
        }
        /* KD_KEY_META_ATX */
        case(VK_MEDIA_NEXT_TRACK):
        {
            return KD_KEY_MEDIANEXTTRACK_ATX;
        }
        case(VK_MEDIA_PLAY_PAUSE):
        {
            return KD_KEY_MEDIAPLAYPAUSE_ATX;
        }
        case(VK_MEDIA_PREV_TRACK):
        {
            return KD_KEY_MEDIAPREVIOUSTRACK_ATX;
        }
        case(VK_MEDIA_STOP):
        {
            return KD_KEY_MEDIASTOP_ATX;
        }
        case(VK_MODECHANGE):
        {
            return KD_KEY_MODECHANGE_ATX;
        }
        case(VK_NONCONVERT):
        {
            return KD_KEY_NONCONVERT_ATX;
        }
        case(VK_NUMLOCK):
        {
            return KD_KEY_NUMLOCK_ATX;
        }
        case(VK_NEXT):
        {
            return KD_KEY_PAGEDOWN_ATX;
        }
        case(VK_PRIOR):
        {
            return KD_KEY_PAGEUP_ATX;
        }
        case(VK_PAUSE):
        {
            return KD_KEY_PAUSE_ATX;
        }
        case(VK_PLAY):
        {
            return KD_KEY_PLAY_ATX;
        }
        /* KD_KEY_PREVIOUSCANDIDATE_ATX */
        case(VK_PRINT):
        {
            return KD_KEY_PRINTSCREEN_ATX;
        }
        case(VK_PROCESSKEY):
        {
            return KD_KEY_PROCESS_ATX;
        }
        /* KD_KEY_PROPS_ATX */
        case(VK_RIGHT):
        {
            return KD_KEY_RIGHT_ATX;
        }
        /* KD_KEY_ROMANCHARACTERS_ATX */
        case(VK_SCROLL):
        {
            return KD_KEY_SCROLL_ATX;
        }
        case(VK_SELECT):
        {
            return KD_KEY_SELECT_ATX;
        }
        case(VK_LAUNCH_MEDIA_SELECT):
        {
            return KD_KEY_SELECTMEDIA_ATX;
        }
        case(VK_SHIFT):
        {
            return KD_KEY_SHIFT_ATX;
        }
        case(VK_CANCEL):
        {
            return KD_KEY_STOP_ATX;
        }
        case(VK_UP):
        {
            return KD_KEY_UP_ATX;
        }
        /* KD_KEY_UNDO_ATX */
        case(VK_VOLUME_DOWN):
        {
            return KD_KEY_VOLUMEDOWN_ATX;
        }
        case(VK_VOLUME_MUTE):
        {
            return KD_KEY_VOLUMEMUTE_ATX;
        }
        case(VK_VOLUME_UP):
        {
            return KD_KEY_VOLUMEUP_ATX;
        }
        case(VK_LWIN):
        case(VK_RWIN):
        {
            return KD_KEY_WIN_ATX;
        }
        case(VK_ZOOM):
        {
            return KD_KEY_ZOOM_ATX;
        }
#elif defined(KD_WINDOW_EMSCRIPTEN)
        case(30):
        {
            return KD_KEY_ACCEPT_ATX;
        }
        /* KD_KEY_AGAIN_ATX */
        /* KD_KEY_ALLCANDIDATES_ATX */
        /* KD_KEY_ALPHANUMERIC_ATX */
        case(18):
        {
            return KD_KEY_ALT_ATX;
        }
        /* KD_KEY_ALTGRAPH_ATX */
        /* KD_KEY_APPS_ATX */
        case(240):
        case(246):
        {
            return KD_KEY_ATTN_ATX;
        }
        /* KD_KEY_BROWSERBACK_ATX
           KD_KEY_BROWSERFAVORITES_ATX
           KD_KEY_BROWSERFORWARD_ATX
           KD_KEY_BROWSERHOME_ATX
           KD_KEY_BROWSERREFRESH_ATX
           KD_KEY_BROWSERSEARCH_ATX
           KD_KEY_BROWSERSTOP_ATX */
        case(20):
        {
            return KD_KEY_CAPSLOCK_ATX;
        }
        case(12):
        case(230):
        case(254):
        {
            return KD_KEY_CLEAR_ATX;
        }
        /* KD_KEY_CODEINPUT_ATX
           KD_KEY_COMPOSE_ATX */
        case(17):
        {
            return KD_KEY_CONTROL_ATX;
        }
        case(247):
        {
            return KD_KEY_CRSEL_ATX;
        }
        case(28):
        {
            return KD_KEY_CONVERT_ATX;
        }
        case(242):
        {
            return KD_KEY_COPY_ATX;
        }
        /* KD_KEY_CUT_ATX */
        case(40):
        {
            return KD_KEY_DOWN_ATX;
        }
        case(35):
        {
            return KD_KEY_END_ATX;
        }
        case(13):
        {
            return KD_KEY_ENTER_ATX;
        }
        case(249):
        {
            return KD_KEY_ERASEEOF_ATX;
        }
        case(43):
        {
            return KD_KEY_EXECUTE_ATX;
        }
        case(248):
        {
            return KD_KEY_EXSEL_ATX;
        }
        case(112):
        {
            return KD_KEY_F1_ATX;
        }
        case(113):
        {
            return KD_KEY_F2_ATX;
        }
        case(114):
        {
            return KD_KEY_F3_ATX;
        }
        case(115):
        {
            return KD_KEY_F4_ATX;
        }
        case(116):
        {
            return KD_KEY_F5_ATX;
        }
        case(117):
        {
            return KD_KEY_F6_ATX;
        }
        case(118):
        {
            return KD_KEY_F7_ATX;
        }
        case(119):
        {
            return KD_KEY_F8_ATX;
        }
        case(120):
        {
            return KD_KEY_F9_ATX;
        }
        case(121):
        {
            return KD_KEY_F10_ATX;
        }
        case(122):
        {
            return KD_KEY_F11_ATX;
        }
        case(123):
        {
            return KD_KEY_F12_ATX;
        }
        case(124):
        {
            return KD_KEY_F13_ATX;
        }
        case(125):
        {
            return KD_KEY_F14_ATX;
        }
        case(126):
        {
            return KD_KEY_F15_ATX;
        }
        case(127):
        {
            return KD_KEY_F16_ATX;
        }
        case(128):
        {
            return KD_KEY_F17_ATX;
        }
        case(129):
        {
            return KD_KEY_F18_ATX;
        }
        case(130):
        {
            return KD_KEY_F19_ATX;
        }
        case(131):
        {
            return KD_KEY_F20_ATX;
        }
        case(132):
        {
            return KD_KEY_F21_ATX;
        }
        case(133):
        {
            return KD_KEY_F22_ATX;
        }
        case(134):
        {
            return KD_KEY_F23_ATX;
        }
        case(135):
        {
            return KD_KEY_F24_ATX;
        }
        case(24):
        {
            return KD_KEY_FINALMODE_ATX;
        }
        /* KD_KEY_FIND_ATX
           KD_KEY_FULLWIDTH_ATX
           KD_KEY_HALFWIDTH_ATX */
        case(21):
        {
            return KD_KEY_HANGULMODE_ATX;
        }
        case(25):
        {
            return KD_KEY_HANJAMODE_ATX;
        }
        case(6):
        {
            return KD_KEY_HELP_ATX;
        }
        /* KD_KEY_HIRAGANA_ATX */
        case(36):
        {
            return KD_KEY_HOME_ATX;
        }
        case(45):
        {
            return KD_KEY_INSERT_ATX;
        }
        /* KD_KEY_JAPANESEHIRAGANA_ATX
           KD_KEY_JAPANESEKATAKANA_ATX
           KD_KEY_JAPANESEROMAJI_ATX
           KD_KEY_JUNJAMODE_ATX
           KD_KEY_KANAMODE_ATX
           KD_KEY_KANJIMODE_ATX
           KD_KEY_KATAKANA_ATX */
        /* KD_KEY_LAUNCHAPPLICATION1_ATX
           KD_KEY_LAUNCHAPPLICATION2_ATX
           KD_KEY_LAUNCHMAIL_ATX*/
        case(37):
        {
            return KD_KEY_LEFT_ATX;
        }
        case(224):
        {
            return KD_KEY_META_ATX;
        }
        /* KD_KEY_MEDIANEXTTRACK_ATX;
           KD_KEY_MEDIAPLAYPAUSE_ATX
           KD_KEY_MEDIAPREVIOUSTRACK_ATX
           KD_KEY_MEDIASTOP_ATX */
        case(31):
        {
            return KD_KEY_MODECHANGE_ATX;
        }
        case(29):
        {
            return KD_KEY_NONCONVERT_ATX;
        }
        case(144):
        {
            return KD_KEY_NUMLOCK_ATX;
        }
        case(34):
        {
            return KD_KEY_PAGEDOWN_ATX;
        }
        case(33):
        {
            return KD_KEY_PAGEUP_ATX;
        }
        case(19):
        {
            return KD_KEY_PAUSE_ATX;
        }
        case(250):
        {
            return KD_KEY_PLAY_ATX;
        }
        /* KD_KEY_PREVIOUSCANDIDATE_ATX */
        case(42):
        {
            return KD_KEY_PRINTSCREEN_ATX;
        }
        /* KD_KEY_PROCESS_ATX */
        /* KD_KEY_PROPS_ATX */
        case(39):
        {
            return KD_KEY_RIGHT_ATX;
        }
        /* KD_KEY_ROMANCHARACTERS_ATX */
        case(145):
        {
            return KD_KEY_SCROLL_ATX;
        }
        case(41):
        {
            return KD_KEY_SELECT_ATX;
        }
        /* KD_KEY_SELECTMEDIA_ATX */
        case(16):
        {
            return KD_KEY_SHIFT_ATX;
        }
        case(3):
        {
            return KD_KEY_STOP_ATX;
        }
        case(38):
        {
            return KD_KEY_UP_ATX;
        }
        /* KD_KEY_UNDO_ATX */
        case(182):
        {
            return KD_KEY_VOLUMEDOWN_ATX;
        }
        case(181):
        {
            return KD_KEY_VOLUMEMUTE_ATX;
        }
        case(183):
        {
            return KD_KEY_VOLUMEUP_ATX;
        }
        case(91):
        {
            return KD_KEY_WIN_ATX;
        }
        case(251):
        {
            return KD_KEY_ZOOM_ATX;
        }
#elif defined(KD_WINDOW_WAYLAND) || defined(KD_WINDOW_X11)
        /* KD_KEY_ACCEPT_ATX */
        case(XKB_KEY_Redo):
        {
            return KD_KEY_AGAIN_ATX;
        }
        /* KD_KEY_ALLCANDIDATES_ATX */
        /* KD_KEY_ALPHANUMERIC_ATX */
        case(XKB_KEY_Alt_L):
        case(XKB_KEY_Alt_R):
        {
            return KD_KEY_ALT_ATX;
        }
        /* KD_KEY_ALTGRAPH_ATX */
        /* KD_KEY_APPS_ATX */
        /* KD_KEY_ATTN_ATX */
        case(XKB_KEY_XF86Back):
        {
            return KD_KEY_BROWSERBACK_ATX;
        }
        case(XKB_KEY_XF86Favorites):
        {
            return KD_KEY_BROWSERFAVORITES_ATX;
        }
        case(XKB_KEY_XF86Forward):
        {
            return KD_KEY_BROWSERFORWARD_ATX;
        }
        case(XKB_KEY_XF86HomePage):
        {
            return KD_KEY_BROWSERHOME_ATX;
        }
        case(XKB_KEY_XF86Refresh):
        {
            return KD_KEY_BROWSERREFRESH_ATX;
        }
        case(XKB_KEY_XF86Search):
        {
            return KD_KEY_BROWSERSEARCH_ATX;
        }
        case(XKB_KEY_XF86Stop):
        {
            return KD_KEY_BROWSERSTOP_ATX;
        }
        case(XKB_KEY_Caps_Lock):
        {
            return KD_KEY_CAPSLOCK_ATX;
        }
        case(XKB_KEY_Clear):
        {
            return KD_KEY_CLEAR_ATX;
        }
        case(XKB_KEY_Codeinput):
        {
            return KD_KEY_CODEINPUT_ATX;
        }
        case(XKB_KEY_Multi_key):
        {
            return KD_KEY_COMPOSE_ATX;
        }
        case(XKB_KEY_Control_L):
        case(XKB_KEY_Control_R):
        {
            return KD_KEY_CONTROL_ATX;
        }
        /* KD_KEY_CRSEL_ATX */
        /* KD_KEY_CONVERT_ATX */
        case(XKB_KEY_XF86Copy):
        {
            return KD_KEY_COPY_ATX;
        }
        case(XKB_KEY_XF86Cut):
        {
            return KD_KEY_CUT_ATX;
        }
        case(XKB_KEY_Down):
        case(XKB_KEY_KP_Down):
        {
            return KD_KEY_DOWN_ATX;
        }
        case(XKB_KEY_End):
        case(XKB_KEY_KP_End):
        {
            return KD_KEY_END_ATX;
        }
        case(XKB_KEY_Return):
        case(XKB_KEY_KP_Enter):
        case(XKB_KEY_ISO_Enter):
        {
            return KD_KEY_ENTER_ATX;
        }
        /* KD_KEY_ERASEEOF_ATX */
        case(XKB_KEY_Execute):
        {
            return KD_KEY_EXECUTE_ATX;
        }
        /* KD_KEY_EXSEL_ATX */
        case(XKB_KEY_F1):
        case(XKB_KEY_KP_F1):
        {
            return KD_KEY_F1_ATX;
        }
        case(XKB_KEY_F2):
        case(XKB_KEY_KP_F2):
        {
            return KD_KEY_F2_ATX;
        }
        case(XKB_KEY_F3):
        case(XKB_KEY_KP_F3):
        {
            return KD_KEY_F3_ATX;
        }
        case(XKB_KEY_F4):
        case(XKB_KEY_KP_F4):
        {
            return KD_KEY_F4_ATX;
        }
        case(XKB_KEY_F5):
        {
            return KD_KEY_F5_ATX;
        }
        case(XKB_KEY_F6):
        {
            return KD_KEY_F6_ATX;
        }
        case(XKB_KEY_F7):
        {
            return KD_KEY_F7_ATX;
        }
        case(XKB_KEY_F8):
        {
            return KD_KEY_F8_ATX;
        }
        case(XKB_KEY_F9):
        {
            return KD_KEY_F9_ATX;
        }
        case(XKB_KEY_F10):
        {
            return KD_KEY_F10_ATX;
        }
        case(XKB_KEY_F11):
        {
            return KD_KEY_F11_ATX;
        }
        case(XKB_KEY_F12):
        {
            return KD_KEY_F12_ATX;
        }
        case(XKB_KEY_F13):
        {
            return KD_KEY_F13_ATX;
        }
        case(XKB_KEY_F14):
        {
            return KD_KEY_F14_ATX;
        }
        case(XKB_KEY_F15):
        {
            return KD_KEY_F15_ATX;
        }
        case(XKB_KEY_F16):
        {
            return KD_KEY_F16_ATX;
        }
        case(XKB_KEY_F17):
        {
            return KD_KEY_F17_ATX;
        }
        case(XKB_KEY_F18):
        {
            return KD_KEY_F18_ATX;
        }
        case(XKB_KEY_F19):
        {
            return KD_KEY_F19_ATX;
        }
        case(XKB_KEY_F20):
        {
            return KD_KEY_F20_ATX;
        }
        case(XKB_KEY_F21):
        {
            return KD_KEY_F21_ATX;
        }
        case(XKB_KEY_F22):
        {
            return KD_KEY_F22_ATX;
        }
        case(XKB_KEY_F23):
        {
            return KD_KEY_F23_ATX;
        }
        case(XKB_KEY_F24):
        {
            return KD_KEY_F24_ATX;
        }
        /* KD_KEY_FINALMODE_ATX */
        case(XKB_KEY_Find):
        {
            return KD_KEY_FIND_ATX;
        }
        /* KD_KEY_FULLWIDTH_ATX
           KD_KEY_HALFWIDTH_ATX
           KD_KEY_HANGULMODE_ATX
           KD_KEY_HANJAMODE_ATX */
        case(XKB_KEY_Help):
        {
            return KD_KEY_HELP_ATX;
        }
        case(XKB_KEY_Hiragana):
        {
            return KD_KEY_HIRAGANA_ATX;
        }
        case(XKB_KEY_Home):
        {
            return KD_KEY_HOME_ATX;
        }
        case(XKB_KEY_Insert):
        {
            return KD_KEY_INSERT_ATX;
        }
        /* KD_KEY_JAPANESEHIRAGANA_ATX
           KD_KEY_JAPANESEKATAKANA_ATX
           KD_KEY_JAPANESEROMAJI_ATX
           KD_KEY_JUNJAMODE_ATX
           KD_KEY_KANAMODE_ATX
           KD_KEY_KANJIMODE_ATX
           KD_KEY_KATAKANA_ATX */
        case(XKB_KEY_XF86Launch0):
        {
            return KD_KEY_LAUNCHAPPLICATION1_ATX;
        }
        case(XKB_KEY_XF86Launch1):
        {
            return KD_KEY_LAUNCHAPPLICATION2_ATX;
        }
        case(XKB_KEY_XF86Mail):
        {
            return KD_KEY_LAUNCHMAIL_ATX;
        }
        case(XKB_KEY_Left):
        {
            return KD_KEY_LEFT_ATX;
        }
        case(XKB_KEY_Meta_L):
        case(XKB_KEY_Meta_R):
        {
            return KD_KEY_META_ATX;
        }
        case(XKB_KEY_XF86AudioForward):
        {
            return KD_KEY_MEDIANEXTTRACK_ATX;
        }
        case(XKB_KEY_XF86AudioPlay):
        case(XKB_KEY_XF86AudioPause):
        {
            return KD_KEY_MEDIAPLAYPAUSE_ATX;
        }
        case(XKB_KEY_XF86AudioPrev):
        {
            return KD_KEY_MEDIAPREVIOUSTRACK_ATX;
        }
        case(XKB_KEY_XF86AudioStop):
        {
            return KD_KEY_MEDIASTOP_ATX;
        }
        case(XKB_KEY_Mode_switch):
        {
            return KD_KEY_MODECHANGE_ATX;
        }
        /* KD_KEY_NONCONVERT_ATX */
        case(XKB_KEY_Num_Lock):
        {
            return KD_KEY_NUMLOCK_ATX;
        }
        case(XKB_KEY_Page_Down):
        case(XKB_KEY_KP_Page_Down):
        {
            return KD_KEY_PAGEDOWN_ATX;
        }
        case(XKB_KEY_KP_Up):
        case(XKB_KEY_KP_Page_Up):
        {
            return KD_KEY_PAGEUP_ATX;
        }
        case(XKB_KEY_Pause):
        {
            return KD_KEY_PAUSE_ATX;
        }
        /* KD_KEY_PLAY_ATX */
        /* KD_KEY_PREVIOUSCANDIDATE_ATX */
        case(XKB_KEY_Print):
        {
            return KD_KEY_PRINTSCREEN_ATX;
        }
        /* KD_KEY_PROCESS_ATX */
        /* KD_KEY_PROPS_ATX */
        case(XKB_KEY_Right):
        {
            return KD_KEY_RIGHT_ATX;
        }
        /* KD_KEY_ROMANCHARACTERS_ATX */
        /* KD_KEY_SCROLL_ATX */
        case(XKB_KEY_Select):
        {
            return KD_KEY_SELECT_ATX;
        }
        case(XKB_KEY_XF86AudioMedia):
        {
            return KD_KEY_SELECTMEDIA_ATX;
        }
        case(XKB_KEY_Shift_L):
        case(XKB_KEY_Shift_R):
        {
            return KD_KEY_SHIFT_ATX;
        }
        case(XKB_KEY_Cancel):
        {
            return KD_KEY_STOP_ATX;
        }
        case(XKB_KEY_Up):
        {
            return KD_KEY_UP_ATX;
        }
        case(XKB_KEY_Undo):
        {
            return KD_KEY_UNDO_ATX;
        }
        case(XKB_KEY_XF86AudioLowerVolume):
        {
            return KD_KEY_VOLUMEDOWN_ATX;
        }
        case(XKB_KEY_XF86AudioMute):
        {
            return KD_KEY_VOLUMEMUTE_ATX;
        }
        case(XKB_KEY_XF86AudioRaiseVolume):
        {
            return KD_KEY_VOLUMEUP_ATX;
        }
        case(XKB_KEY_Super_L):
        case(XKB_KEY_Super_R):
        {
            return KD_KEY_WIN_ATX;
        }
/* KD_KEY_ZOOM_ATX */
#endif
        default:
        {
            return 0;
        }
    }
}
#endif

KD_API KDint KD_APIENTRY kdPumpEvents(void)
{
    KDsize queuesize = __kdQueueSize(kdThreadSelf()->eventqueue);
    for(KDuint i = 0; i < queuesize; i++)
    {
        KDEvent *callbackevent = __kdQueuePull(kdThreadSelf()->eventqueue);
        if(callbackevent)
        {
            if(!__kdExecCallback(callbackevent))
            {
                /* Not a callback */
                kdPostEvent(callbackevent);
            }
        }
    }

#ifdef KD_WINDOW_SUPPORTED
    KD_UNUSED KDWindow *window = __kd_window;
#if defined(KD_WINDOW_ANDROID)
    AInputEvent *aevent = KD_NULL;
    kdThreadMutexLock(__kd_androidinputqueue_mutex);
    if(__kd_androidinputqueue)
    {
        while(AInputQueue_getEvent(__kd_androidinputqueue, &aevent) >= 0)
        {
            AInputQueue_preDispatchEvent(__kd_androidinputqueue, aevent);
            KDEvent *event = kdCreateEvent();
            switch(AInputEvent_getType(aevent))
            {
                case(AINPUT_EVENT_TYPE_KEY):
                {
                    event->type = KD_EVENT_INPUT_KEY_ATX;
                    KDEventInputKeyATX *keyevent = (KDEventInputKeyATX *)(&event->data);

                    if(AKeyEvent_getAction(aevent) == AKEY_EVENT_ACTION_DOWN)
                    {
                        keyevent->flags = KD_KEY_PRESS_ATX;
                    }
                    else
                    {
                        keyevent->flags = 0;
                    }

                    KDint32 keycode = AKeyEvent_getKeyCode(aevent);
                    keyevent->keycode = __KDKeycodeLookup(keycode);
                    if(!keyevent->keycode)
                    {
                        event->type = KD_EVENT_INPUT_KEYCHAR_ATX;
                        KDEventInputKeyCharATX *keycharevent = (KDEventInputKeyCharATX *)(&event->data);
                        keycharevent->character = keycode;
                    }

                    if(!__kdExecCallback(event))
                    {
                        kdPostEvent(event);
                    }
                    break;
                }
                default:
                {
                    kdFreeEvent(event);
                    break;
                }
            }
            AInputQueue_finishEvent(__kd_androidinputqueue, aevent, 1);
        }
    }
    kdThreadMutexUnlock(__kd_androidinputqueue_mutex);
#elif defined(KD_WINDOW_WIN32)
    if(window)
    {
        MSG msg = {0};
        while(PeekMessage(&msg, window->nativewindow, 0, 0, PM_REMOVE) != 0)
        {
            KDEvent *kdevent = kdCreateEvent();
            kdevent->userptr = window->eventuserptr;
            switch(msg.message)
            {
                case WM_CLOSE:
                case WM_DESTROY:
                case WM_QUIT:
                {
                    ShowWindow(window->nativewindow, SW_HIDE);
                    kdevent->type = KD_EVENT_WINDOW_CLOSE;
                    if(!__kdExecCallback(kdevent))
                    {
                        kdPostEvent(kdevent);
                    }
                    break;
                }
                case WM_INPUT:
                {
                    KDchar buffer[sizeof(RAWINPUT)] = {0};
                    KDsize size = sizeof(RAWINPUT);
                    GetRawInputData((HRAWINPUT)msg.lParam, RID_INPUT, buffer, (PUINT)&size, sizeof(RAWINPUTHEADER));
                    RAWINPUT *raw = (RAWINPUT *)buffer;
                    if(raw->header.dwType == RIM_TYPEMOUSE)
                    {
                        if(raw->data.mouse.usButtonFlags == RI_MOUSE_LEFT_BUTTON_DOWN ||
                            raw->data.mouse.usButtonFlags == RI_MOUSE_RIGHT_BUTTON_DOWN ||
                            raw->data.mouse.usButtonFlags == RI_MOUSE_MIDDLE_BUTTON_DOWN)
                        {
                            kdevent->type = KD_EVENT_INPUT_POINTER;
                            kdevent->data.inputpointer.index = KD_INPUT_POINTER_SELECT;
                            kdevent->data.inputpointer.select = 1;
                            kdevent->data.inputpointer.x = raw->data.mouse.lLastX;
                            kdevent->data.inputpointer.y = raw->data.mouse.lLastY;

                            window->states.pointer.select = kdevent->data.inputpointer.select;
                            window->states.pointer.x = kdevent->data.inputpointer.x;
                            window->states.pointer.y = kdevent->data.inputpointer.y;
                        }
                        else if(raw->data.mouse.usButtonFlags == RI_MOUSE_LEFT_BUTTON_UP ||
                            raw->data.mouse.usButtonFlags == RI_MOUSE_RIGHT_BUTTON_UP ||
                            raw->data.mouse.usButtonFlags == RI_MOUSE_MIDDLE_BUTTON_UP)
                        {
                            kdevent->type = KD_EVENT_INPUT_POINTER;
                            kdevent->data.inputpointer.index = KD_INPUT_POINTER_SELECT;
                            kdevent->data.inputpointer.select = 0;
                            kdevent->data.inputpointer.x = raw->data.mouse.lLastX;
                            kdevent->data.inputpointer.y = raw->data.mouse.lLastY;

                            window->states.pointer.select = kdevent->data.inputpointer.select;
                            window->states.pointer.x = kdevent->data.inputpointer.x;
                            window->states.pointer.y = kdevent->data.inputpointer.y;
                        }
                        else if(raw->data.keyboard.Flags & MOUSE_MOVE_ABSOLUTE)
                        {
                            kdevent->type = KD_EVENT_INPUT_POINTER;
                            kdevent->data.inputpointer.index = KD_INPUT_POINTER_X;
                            kdevent->data.inputpointer.x = raw->data.mouse.lLastX;
                            kdevent->data.inputpointer.y = raw->data.mouse.lLastY;

                            window->states.pointer.x = kdevent->data.inputpointer.x;
                            window->states.pointer.y = kdevent->data.inputpointer.y;
                        }
                    }
                    else if(raw->header.dwType == RIM_TYPEKEYBOARD)
                    {
                        kdevent->type = KD_EVENT_INPUT_KEY_ATX;
                        KDint32 character = 0;
#if defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable : 6313)
#endif
                        if(raw->data.keyboard.Flags & RI_KEY_MAKE)
#if defined(_MSC_VER)
#pragma warning(pop)
#endif
                        {
                            GetKeyNameText((KDint64)MapVirtualKey(raw->data.keyboard.VKey, MAPVK_VK_TO_VSC) << 16, (KDchar *)&character, sizeof(KDint32));
                        }

                        if(character)
                        {
                            kdevent->type = KD_EVENT_INPUT_KEYCHAR_ATX;
                            KDEventInputKeyCharATX *keycharevent = (KDEventInputKeyCharATX *)(&kdevent->data);
                            keycharevent->flags = 0;
                            keycharevent->character = character;

                            window->states.keyboard.charflags = keycharevent->flags;
                            window->states.keyboard.character = keycharevent->character;
                        }
                        else
                        {
                            KDint32 keycode = __KDKeycodeLookup(raw->data.keyboard.VKey);
                            if(keycode)
                            {
                                kdevent->type = KD_EVENT_INPUT_KEY_ATX;
                                KDEventInputKeyATX *keyevent = (KDEventInputKeyATX *)(&kdevent->data);

                                keyevent->flags = 0;
#if defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable : 6313)
#endif
                                if(raw->data.keyboard.Flags & RI_KEY_MAKE)
#if defined(_MSC_VER)
#pragma warning(pop)
#endif
                                {
                                    keyevent->flags |= KD_KEY_PRESS_ATX;
                                }
                                keyevent->keycode = keycode;

                                window->states.keyboard.flags = keyevent->flags;
                                window->states.keyboard.keycode = keyevent->keycode;

                                __kdHandleSpecialKeys(window, keyevent);
                            }
                            else
                            {
                                kdFreeEvent(kdevent);
                                break;
                            }
                        }
                    }
                    if(!__kdExecCallback(kdevent))
                    {
                        kdPostEvent(kdevent);
                    }
                    break;
                }
                default:
                {
                    kdFreeEvent(kdevent);
                    DispatchMessage(&msg);
                    break;
                }
            }
        }
    }
#elif defined(KD_WINDOW_EMSCRIPTEN)
    emscripten_sleep(1);
#elif defined(KD_WINDOW_WAYLAND) || defined(KD_WINDOW_X11)
#if defined(KD_WINDOW_X11)
    if(window && window->platform == EGL_PLATFORM_X11_KHR)
    {
        xcb_generic_event_t *event;
        while((event = xcb_poll_for_event(window->nativedisplay)))
        {
            KDEvent *kdevent = kdCreateEvent();
            kdevent->userptr = window->eventuserptr;
            KDuint type = event->response_type & ~0x80;
            switch(type)
            {
                case XCB_BUTTON_PRESS:
                case XCB_BUTTON_RELEASE:
                {
                    xcb_button_press_event_t *press = (xcb_button_press_event_t *)event;
                    kdevent->type = KD_EVENT_INPUT_POINTER;
                    kdevent->data.inputpointer.index = KD_INPUT_POINTER_SELECT;
                    kdevent->data.inputpointer.select = (type == XCB_BUTTON_PRESS) ? 1 : 0;
                    kdevent->data.inputpointer.x = press->event_x;
                    kdevent->data.inputpointer.y = press->event_y;

                    window->states.pointer.select = kdevent->data.inputpointer.select;
                    window->states.pointer.x = kdevent->data.inputpointer.x;
                    window->states.pointer.y = kdevent->data.inputpointer.y;

                    if(!__kdExecCallback(kdevent))
                    {
                        kdPostEvent(kdevent);
                    }
                    break;
                }
                case XCB_KEY_PRESS:
                case XCB_KEY_RELEASE:
                {
                    xcb_key_press_event_t *press = (xcb_key_press_event_t *)event;
                    xkb_keysym_t keysym = xkb_state_key_get_one_sym(window->xkb.state, press->detail);

                    KDuint32 flags = 0;
                    KDint32 character = 0;
                    static xcb_key_press_event_t *lastpress = KD_NULL;
                    if(type == XCB_KEY_PRESS)
                    {
                        if(lastpress && ((lastpress->response_type & ~0x80) == XCB_KEY_RELEASE) && (lastpress->detail == press->detail) && (lastpress->time == press->time))
                        {
                            flags = KD_KEY_AUTOREPEAT_ATX;
                        }

                        /* Printable ASCII range. */
                        if((keysym >= 20) && (keysym <= 126))
                        {
                            character = (KDint32)keysym;
                        }
                    }
                    lastpress = press;

                    if(character)
                    {
                        kdevent->type = KD_EVENT_INPUT_KEYCHAR_ATX;
                        KDEventInputKeyCharATX *keycharevent = (KDEventInputKeyCharATX *)(&kdevent->data);
                        keycharevent->flags = flags;
                        keycharevent->character = character;

                        window->states.keyboard.charflags = keycharevent->flags;
                        window->states.keyboard.character = keycharevent->character;
                    }
                    else
                    {
                        KDint32 keycode = __KDKeycodeLookup((KDint32)keysym);
                        if(keycode)
                        {
                            kdevent->type = KD_EVENT_INPUT_KEY_ATX;
                            KDEventInputKeyATX *keyevent = (KDEventInputKeyATX *)(&kdevent->data);
                            keyevent->flags = 0;
                            if(type == XCB_KEY_PRESS)
                            {
                                keyevent->flags |= KD_KEY_PRESS_ATX;
                            }
                            if(xkb_state_mod_name_is_active(window->xkb.state, XKB_MOD_NAME_SHIFT, XKB_STATE_MODS_EFFECTIVE) > 0)
                            {
                                keyevent->flags |= KD_KEY_MODIFIER_SHIFT_ATX;
                            }
                            if(xkb_state_mod_name_is_active(window->xkb.state, XKB_MOD_NAME_CTRL, XKB_STATE_MODS_EFFECTIVE) > 0)
                            {
                                keyevent->flags |= KD_KEY_MODIFIER_CTRL_ATX;
                            }
                            if(xkb_state_mod_name_is_active(window->xkb.state, XKB_MOD_NAME_ALT, XKB_STATE_MODS_EFFECTIVE) > 0)
                            {
                                keyevent->flags |= KD_KEY_MODIFIER_ALT_ATX;
                            }
                            if(xkb_state_mod_name_is_active(window->xkb.state, XKB_MOD_NAME_LOGO, XKB_STATE_MODS_EFFECTIVE) > 0)
                            {
                                keyevent->flags |= KD_KEY_MODIFIER_META_ATX;
                            }
                            keyevent->keycode = keycode;

                            window->states.keyboard.flags = keyevent->flags;
                            window->states.keyboard.keycode = keyevent->keycode;

                            __kdHandleSpecialKeys(window, keyevent);
                        }
                        else
                        {
                            kdFreeEvent(kdevent);
                            break;
                        }
                    }

                    if(!__kdExecCallback(kdevent))
                    {
                        kdPostEvent(kdevent);
                    }
                    break;
                }
                case XCB_MOTION_NOTIFY:
                {
                    xcb_motion_notify_event_t *motion = (xcb_motion_notify_event_t *)event;
                    kdevent->type = KD_EVENT_INPUT_POINTER;
                    kdevent->data.inputpointer.index = KD_INPUT_POINTER_X;
                    kdevent->data.inputpointer.select = 0;
                    kdevent->data.inputpointer.x = motion->event_x;
                    kdevent->data.inputpointer.y = motion->event_y;

                    window->states.pointer.select = kdevent->data.inputpointer.select;
                    window->states.pointer.x = kdevent->data.inputpointer.x;
                    window->states.pointer.y = kdevent->data.inputpointer.y;

                    if(!__kdExecCallback(kdevent))
                    {
                        kdPostEvent(kdevent);
                    }
                    break;
                }
                case XCB_ENTER_NOTIFY:
                case XCB_LEAVE_NOTIFY:
                {
                    kdevent->type = KD_EVENT_WINDOW_FOCUS;
                    kdevent->data.windowfocus.focusstate = (type == XCB_ENTER_NOTIFY) ? 1 : 0;

                    if(!__kdExecCallback(kdevent))
                    {
                        kdPostEvent(kdevent);
                    }
                    break;
                }
                case XCB_CLIENT_MESSAGE:
                {
                    xcb_intern_atom_cookie_t delcookie = xcb_intern_atom(window->nativedisplay, 0, 16, "WM_DELETE_WINDOW");
                    xcb_intern_atom_reply_t *delreply = xcb_intern_atom_reply(window->nativedisplay, delcookie, 0);
                    if((*(xcb_client_message_event_t *)event).data.data32[0] == (*delreply).atom)
                    {
                        kdevent->type = KD_EVENT_WINDOW_CLOSE;
                        if(!__kdExecCallback(kdevent))
                        {
                            kdPostEvent(kdevent);
                        }
                        break;
                    }
                    kdFreeEvent(kdevent);
                    break;
                }
                default:
                {
                    if(type == window->xkb.firstevent)
                    {
                        switch(event->pad0)
                        {
                            case XCB_XKB_NEW_KEYBOARD_NOTIFY:
                            case XCB_XKB_MAP_NOTIFY:
                            {
                                KDint32 device = xkb_x11_get_core_keyboard_device_id(window->nativedisplay);
                                xkb_keymap_unref(window->xkb.keymap);
                                window->xkb.keymap = xkb_x11_keymap_new_from_device(window->xkb.context, window->nativedisplay, device, XKB_KEYMAP_COMPILE_NO_FLAGS);
                                xkb_state_unref(window->xkb.state);
                                window->xkb.state = xkb_x11_state_new_from_device(window->xkb.keymap, window->nativedisplay, device);
                                break;
                            }
                            case XCB_XKB_STATE_NOTIFY:
                            {
                                xcb_xkb_state_notify_event_t *statenotify = (xcb_xkb_state_notify_event_t *)event;
                                xkb_state_update_mask(window->xkb.state,
                                    statenotify->baseMods, statenotify->latchedMods, statenotify->lockedMods,
                                    (KDuint)statenotify->baseGroup, (KDuint)statenotify->latchedGroup, statenotify->lockedGroup);
                                break;
                            }
                            default:
                            {
                                break;
                            }
                        }
                    }
                    kdFreeEvent(kdevent);
                    break;
                }
            }
        }
    }
#endif
#if defined(KD_WINDOW_WAYLAND)
    if(window && window->platform == EGL_PLATFORM_WAYLAND_KHR)
    {
        wl_display_dispatch_pending(window->nativedisplay);
    }
#endif
#endif
#endif
    return 0;
}

/* kdInstallCallback: Install or remove a callback function for event processing. */
KD_API KDint KD_APIENTRY kdInstallCallback(KDCallbackFunc *func, KDint eventtype, void *eventuserptr)
{
    KDint callbackindex = kdThreadSelf()->callbackindex;
    _KDCallback **callbacks = kdThreadSelf()->callbacks;
    for(KDint i = 0; i < callbackindex; i++)
    {
        KDboolean typematch = callbacks[i]->eventtype == eventtype || callbacks[i]->eventtype == 0;
        KDboolean userptrmatch = callbacks[i]->eventuserptr == eventuserptr;
        if(typematch && userptrmatch)
        {
            callbacks[i]->func = func;
            return 0;
        }
    }
    callbacks[callbackindex] = (_KDCallback *)kdMalloc(sizeof(_KDCallback));
    if(callbacks[callbackindex] == KD_NULL)
    {
        kdSetError(KD_ENOMEM);
        return -1;
    }
    callbacks[callbackindex]->func = func;
    callbacks[callbackindex]->eventtype = eventtype;
    callbacks[callbackindex]->eventuserptr = eventuserptr;
    kdThreadSelf()->callbackindex++;
    return 0;
}

/* kdCreateEvent: Create an event for posting. */
KD_API KDEvent *KD_APIENTRY kdCreateEvent(void)
{
    KDEvent *event = (KDEvent *)kdMalloc(sizeof(KDEvent));
    if(event == KD_NULL)
    {
        kdSetError(KD_ENOMEM);
        return KD_NULL;
    }
    event->timestamp = 0;
    event->type = -1;
    event->userptr = KD_NULL;
    return event;
}

/* kdPostEvent, kdPostThreadEvent: Post an event into a queue. */
KD_API KDint KD_APIENTRY kdPostEvent(KDEvent *event)
{
    return kdPostThreadEvent(event, kdThreadSelf());
}
KD_API KDint KD_APIENTRY kdPostThreadEvent(KDEvent *event, KDThread *thread)
{
    if(event->timestamp == 0)
    {
        event->timestamp = kdGetTimeUST();
    }
    KDint error = __kdQueuePush(thread->eventqueue, (void *)event);
    if(error == -1)
    {
        kdFreeEvent(event);
        kdSetError(KD_ENOMEM);
        return -1;
    }
    return 0;
}

/* kdFreeEvent: Abandon an event instead of posting it. */
KD_API void KD_APIENTRY kdFreeEvent(KDEvent *event)
{
    kdFree(event);
}

/******************************************************************************
 * Application startup and exit.
 ******************************************************************************/
#if defined(__ANDROID__)
/* All Android events are send to the mainthread */
static KDThread *__kd_androidmainthread = KD_NULL;
static KDThreadMutex *__kd_androidmainthread_mutex = KD_NULL;
static ANativeActivity *__kd_androidactivity = KD_NULL;
static KDThreadMutex *__kd_androidactivity_mutex = KD_NULL;
static void __kd_AndroidOnDestroy(KD_UNUSED ANativeActivity *activity)
{
    KDEvent *event = kdCreateEvent();
    event->type = KD_EVENT_WINDOW_CLOSE;
    kdPostThreadEvent(event, __kd_androidmainthread);
}

static void __kd_AndroidOnStart(KD_UNUSED ANativeActivity *activity)
{
    KDEvent *event = kdCreateEvent();
    event->type = KD_EVENT_RESUME;
    kdPostThreadEvent(event, __kd_androidmainthread);
}

static void __kd_AndroidOnResume(KD_UNUSED ANativeActivity *activity)
{
    KDEvent *event = kdCreateEvent();
    event->type = KD_EVENT_RESUME;
    kdPostThreadEvent(event, __kd_androidmainthread);
}

static void *__kd_AndroidOnSaveInstanceState(KD_UNUSED ANativeActivity *activity, KD_UNUSED KDsize *len)
{
    /* TODO: Save state */
    return KD_NULL;
}

static void __kd_AndroidOnPause(KD_UNUSED ANativeActivity *activity)
{
    KDEvent *event = kdCreateEvent();
    event->type = KD_EVENT_PAUSE;
    kdPostThreadEvent(event, __kd_androidmainthread);
}

static void __kd_AndroidOnStop(KD_UNUSED ANativeActivity *activity)
{
    KDEvent *event = kdCreateEvent();
    event->type = KD_EVENT_PAUSE;
    kdPostThreadEvent(event, __kd_androidmainthread);
}

static void __kd_AndroidOnConfigurationChanged(KD_UNUSED ANativeActivity *activity)
{
    KDEvent *event = kdCreateEvent();
    event->type = KD_EVENT_ORIENTATION;
    kdPostThreadEvent(event, __kd_androidmainthread);
}

static void __kd_AndroidOnLowMemory(KD_UNUSED ANativeActivity *activity)
{
    /* TODO: Avoid getting killed by Android */
}

static void __kd_AndroidOnWindowFocusChanged(KD_UNUSED ANativeActivity *activity, KDint focused)
{
    KDEvent *event = kdCreateEvent();
    event->type = KD_EVENT_WINDOW_FOCUS;
    event->data.windowfocus.focusstate = focused;
    kdPostThreadEvent(event, __kd_androidmainthread);
}

static ANativeWindow *__kd_androidwindow = KD_NULL;
static KDThreadMutex *__kd_androidwindow_mutex = KD_NULL;
static void __kd_AndroidOnNativeWindowCreated(KD_UNUSED ANativeActivity *activity, ANativeWindow *window)
{
    kdThreadMutexLock(__kd_androidwindow_mutex);
    __kd_androidwindow = window;
    kdThreadMutexUnlock(__kd_androidwindow_mutex);
}

static void __kd_AndroidOnNativeWindowDestroyed(KD_UNUSED ANativeActivity *activity, KD_UNUSED ANativeWindow *window)
{
    kdThreadMutexLock(__kd_androidwindow_mutex);
    __kd_androidwindow = KD_NULL;
    kdThreadMutexUnlock(__kd_androidwindow_mutex);
    KDEvent *event = kdCreateEvent();
    event->type = KD_EVENT_WINDOW_CLOSE;
    kdPostThreadEvent(event, __kd_androidmainthread);
}

static void __kd_AndroidOnInputQueueCreated(KD_UNUSED ANativeActivity *activity, AInputQueue *queue)
{
    kdThreadMutexLock(__kd_androidinputqueue_mutex);
    __kd_androidinputqueue = queue;
    kdThreadMutexUnlock(__kd_androidinputqueue_mutex);
}

static void __kd_AndroidOnInputQueueDestroyed(KD_UNUSED ANativeActivity *activity, KD_UNUSED AInputQueue *queue)
{
    kdThreadMutexLock(__kd_androidinputqueue_mutex);
    __kd_androidinputqueue = KD_NULL;
    kdThreadMutexUnlock(__kd_androidinputqueue_mutex);
}
#endif

static KDint __kdPreMain(KDint argc, KDchar **argv)
{
#if defined(_WIN32)
    WSADATA wsadata;
    kdMemset(&wsadata, 0, sizeof(WSADATA));
    if(WSAStartup(0x202, &wsadata) != 0)
    {
        kdLogMessage("Winsock2 error.");
        kdExit(-1);
    }
#endif

    __kd_userptrmtx = kdThreadMutexCreate(KD_NULL);
    __kd_tls_mutex = kdThreadMutexCreate(KD_NULL);

    KDThread *thread = KD_NULL;
#if defined(__ANDROID__)
    kdThreadMutexLock(__kd_androidmainthread_mutex);
    thread = __kd_androidmainthread;
    kdThreadMutexUnlock(__kd_androidmainthread_mutex);
#else
    thread = __kdThreadInit();
#endif
    kdThreadOnce(&__kd_threadinit_once, __kdThreadInitOnce);
    kdSetThreadStorageKHR(__kd_threadlocal, thread);

    KDint result = 0;
#if defined(__ANDROID__) || defined(__EMSCRIPTEN__) || (defined(__MINGW32__) && !defined(__MINGW64__))
    result = kdMain(argc, (const KDchar *const *)argv);
#if defined(__ANDROID__)
    kdThreadMutexFree(__kd_androidactivity_mutex);
    kdThreadMutexFree(__kd_androidinputqueue_mutex);
    kdThreadMutexFree(__kd_androidwindow_mutex);
    kdThreadMutexFree(__kd_androidmainthread_mutex);
#endif
#else
    typedef KDint(KD_APIENTRY * KDMAIN)(KDint, const KDchar *const *);
    KDMAIN kdmain = KD_NULL;
#if defined(_WIN32)
    HMODULE app = GetModuleHandle(KD_NULL);
    FARPROC rawptr = GetProcAddress(app, "kdMain");
    if(rawptr == KD_NULL)
    {
        kdLogMessage("Unable to locate kdMain.");
        kdExit(-1);
    }
    kdMemcpy(&kdmain, &rawptr, sizeof(rawptr));
    result = kdmain(argc, (const KDchar *const *)argv);
#else
    void *app = dlopen(KD_NULL, RTLD_NOW);
    void *rawptr = dlsym(app, "kdMain");
    if(dlerror())
    {
        kdLogMessage("Unable to locate kdMain.");
        kdExit(-1);
    }
    kdMemcpy(&kdmain, &rawptr, sizeof(rawptr));
    result = kdmain(argc, (const KDchar *const *)argv);
    dlclose(app);
#endif
#endif

    __kdCleanupThreadStorageKHR();
#if !defined(__ANDROID__)
    __kdThreadFree(thread);
#endif
    kdThreadMutexFree(__kd_tls_mutex);
    kdThreadMutexFree(__kd_userptrmtx);

#if defined(_WIN32)
    WSACleanup();
#endif
    return result;
}

#ifdef __ANDROID__
static void *__kdAndroidPreMain(KD_UNUSED void *arg)
{
    __kdPreMain(0, KD_NULL);
    return 0;
}
void ANativeActivity_onCreate(ANativeActivity *activity, KD_UNUSED void *savedState, KD_UNUSED KDsize savedStateSize)
{
    __kd_androidmainthread_mutex = kdThreadMutexCreate(KD_NULL);
    __kd_androidwindow_mutex = kdThreadMutexCreate(KD_NULL);
    __kd_androidinputqueue_mutex = kdThreadMutexCreate(KD_NULL);
    __kd_androidactivity_mutex = kdThreadMutexCreate(KD_NULL);

    activity->callbacks->onDestroy = __kd_AndroidOnDestroy;
    activity->callbacks->onStart = __kd_AndroidOnStart;
    activity->callbacks->onResume = __kd_AndroidOnResume;
    activity->callbacks->onSaveInstanceState = __kd_AndroidOnSaveInstanceState;
    activity->callbacks->onPause = __kd_AndroidOnPause;
    activity->callbacks->onStop = __kd_AndroidOnStop;
    activity->callbacks->onConfigurationChanged = __kd_AndroidOnConfigurationChanged;
    activity->callbacks->onLowMemory = __kd_AndroidOnLowMemory;
    activity->callbacks->onWindowFocusChanged = __kd_AndroidOnWindowFocusChanged;
    activity->callbacks->onNativeWindowCreated = __kd_AndroidOnNativeWindowCreated;
    activity->callbacks->onNativeWindowDestroyed = __kd_AndroidOnNativeWindowDestroyed;
    activity->callbacks->onInputQueueCreated = __kd_AndroidOnInputQueueCreated;
    activity->callbacks->onInputQueueDestroyed = __kd_AndroidOnInputQueueDestroyed;

    kdThreadMutexLock(__kd_androidactivity_mutex);
    __kd_androidactivity = activity;
    ANativeActivity_setWindowFlags(__kd_androidactivity, AWINDOW_FLAG_KEEP_SCREEN_ON, 0);
    kdThreadMutexUnlock(__kd_androidactivity_mutex);

    kdThreadMutexLock(__kd_androidmainthread_mutex);
    __kd_androidmainthread = kdThreadCreate(KD_NULL, __kdAndroidPreMain, KD_NULL);
    kdThreadDetach(__kd_androidmainthread);
    kdThreadMutexUnlock(__kd_androidmainthread_mutex);
}
#endif

KD_API int main(int argc, char **argv)
{
    KDint result = __kdPreMain(argc, argv);
    return result;
}


#if defined(_WIN32)
int WINAPI WinMain(KD_UNUSED HINSTANCE hInstance, KD_UNUSED HINSTANCE hPrevInstance, KD_UNUSED LPSTR lpCmdLine, KD_UNUSED int nShowCmd)
{
    return __kdPreMain(0, KD_NULL); /* (__argc, __argv) */
}
#if defined(KD_FREESTANDING) && !defined(__MINGW32__)
void WINAPI WinMainCRTStartup(void)
{
    KDint result = WinMain(GetModuleHandle(KD_NULL), KD_NULL, GetCommandLine(), SW_SHOWDEFAULT);
    ExitProcess(result);
}
BOOL WINAPI DllMain(KD_UNUSED HINSTANCE hInstDll, KD_UNUSED DWORD fdwReason, KD_UNUSED LPVOID lpReserved)
{
    return 1;
}
BOOL WINAPI _DllMainCRTStartup(HINSTANCE hInstDll, DWORD fdwReason, LPVOID lpReserved)
{
    return DllMain(hInstDll, fdwReason, lpReserved);
}
void WINAPI mainCRTStartup(void)
{
    KDint result = main(0, KD_NULL); /* (__argc, __argv) */
    ExitProcess(result);
}
#endif
#endif

/* kdExit: Exit the application. */
KD_API KD_NORETURN void KD_APIENTRY kdExit(KDint status)
{
    if(status == 0)
    {
        status = EXIT_SUCCESS;
    }
    else
    {
        status = EXIT_FAILURE;
    }

#if defined(__EMSCRIPTEN__)
    emscripten_force_exit(status);
#endif

#if defined(_WIN32)
    ExitProcess(status);
    while(1)
    {
        ;
    }
#else
    exit(status);
#endif
}

/******************************************************************************
 * Input/output
 ******************************************************************************/
/* kdStateGeti, kdStateGetl, kdStateGetf: get state value(s) */
KD_API KDint KD_APIENTRY kdStateGeti(KDint startidx, KDuint numidxs, KDint32 *buffer)
{
    KDint idx = startidx;
    for(KDuint i = 0; i != numidxs; i++)
    {
        switch(idx)
        {
#if defined(KD_WINDOW_SUPPORTED)
            case KD_STATE_POINTER_AVAILABILITY:
            {
                buffer[i] = __kd_window->states.pointer.availability;
                break;
            }
            case KD_INPUT_POINTER_X:
            {
                buffer[i] = __kd_window->states.pointer.x;
                break;
            }
            case KD_INPUT_POINTER_Y:
            {
                buffer[i] = __kd_window->states.pointer.y;
                break;
            }
            case KD_INPUT_POINTER_SELECT:
            {
                buffer[i] = __kd_window->states.pointer.select;
                break;
            }
            case KD_STATE_KEYBOARD_AVAILABILITY_ATX:
            {
                buffer[i] = __kd_window->states.keyboard.availability;
                break;
            }
            case KD_INPUT_KEYBOARD_FLAGS_ATX:
            {
                buffer[i] = (KDint32)__kd_window->states.keyboard.flags;
                break;
            }
            case KD_INPUT_KEYBOARD_CHAR_ATX:
            {
                buffer[i] = __kd_window->states.keyboard.character;
                break;
            }
            case KD_INPUT_KEYBOARD_KEYCODE_ATX:
            {
                buffer[i] = __kd_window->states.keyboard.keycode;
                break;
            }
            case KD_INPUT_KEYBOARD_CHARFLAGS_ATX:
            {
                buffer[i] = (KDint32)__kd_window->states.keyboard.charflags;
                break;
            }
            case KD_STATE_DPAD_AVAILABILITY:
            {
                buffer[i] = __kd_window->states.dpad.availability;
                break;
            }
            case KD_STATE_DPAD_COPY:
            {
                buffer[i] = -1;
                break;
            }
            case KD_INPUT_DPAD_UP:
            {
                buffer[i] = __kd_window->states.dpad.up;
                break;
            }
            case KD_INPUT_DPAD_LEFT:
            {
                buffer[i] = __kd_window->states.dpad.left;
                break;
            }
            case KD_INPUT_DPAD_RIGHT:
            {
                buffer[i] = __kd_window->states.dpad.right;
                break;
            }
            case KD_INPUT_DPAD_DOWN:
            {
                buffer[i] = __kd_window->states.dpad.down;
                break;
            }
            case KD_INPUT_DPAD_SELECT:
            {
                buffer[i] = __kd_window->states.dpad.select;
                break;
            }
            case KD_STATE_GAMEKEYS_AVAILABILITY:
            {
                buffer[i] = __kd_window->states.gamekeys.availability;
                break;
            }
            case KD_INPUT_GAMEKEYS_UP:
            {
                buffer[i] = __kd_window->states.gamekeys.up;
                break;
            }
            case KD_INPUT_GAMEKEYS_LEFT:
            {
                buffer[i] = __kd_window->states.gamekeys.left;
                break;
            }
            case KD_INPUT_GAMEKEYS_RIGHT:
            {
                buffer[i] = __kd_window->states.gamekeys.right;
                break;
            }
            case KD_INPUT_GAMEKEYS_DOWN:
            {
                buffer[i] = __kd_window->states.gamekeys.down;
                break;
            }
            case KD_INPUT_GAMEKEYS_FIRE:
            {
                buffer[i] = __kd_window->states.gamekeys.fire;
                break;
            }
#endif
            case KD_STATE_EVENT_USING_BATTERY:
            {
#if defined(_WIN32)
                SYSTEM_POWER_STATUS status;
                GetSystemPowerStatus(&status);
                buffer[i] = (status.ACLineStatus == 0);
#else
                buffer[i] = 0;
#endif
                break;
            }
            case KD_STATE_EVENT_LOW_BATTERY:
            {
#if defined(_WIN32)
                SYSTEM_POWER_STATUS status;
                GetSystemPowerStatus(&status);
                buffer[i] = (status.BatteryFlag == 2) || (status.BatteryFlag == 4);
#else
                buffer[i] = 0;
#endif
                break;
            }
            case KD_STATE_BACKLIGHT_AVAILABILITY:
            {
#if defined(_WIN32)
                buffer[i] = 1;
#else
                buffer[i] = 0;
#endif
                break;
            }
            default:
            {
                kdSetError(KD_EIO);
                return -1;
            }
        }
        idx++;
    }
    return 0;
}

KD_API KDint KD_APIENTRY kdStateGetl(KD_UNUSED KDint startidx, KD_UNUSED KDuint numidxs, KD_UNUSED KDint64 *buffer)
{
    kdSetError(KD_EIO);
    return -1;
}

KD_API KDint KD_APIENTRY kdStateGetf(KD_UNUSED KDint startidx, KD_UNUSED KDuint numidxs, KD_UNUSED KDfloat32 *buffer)
{
    kdSetError(KD_EIO);
    return -1;
}


/* kdOutputSeti, kdOutputSetf: set outputs */
KD_API KDint KD_APIENTRY kdOutputSeti(KD_UNUSED KDint startidx, KD_UNUSED KDuint numidxs, KD_UNUSED const KDint32 *buffer)
{
#if defined(_WIN32)
    if(startidx == KD_OUTPUT_BACKLIGHT_FORCE)
    {
        if(buffer[0])
        {
            SetThreadExecutionState(ES_CONTINUOUS | ES_DISPLAY_REQUIRED);
            return 0;
        }
    }
#endif
    kdSetError(KD_EIO);
    return -1;
}

KD_API KDint KD_APIENTRY kdOutputSetf(KD_UNUSED KDint startidx, KD_UNUSED KDuint numidxs, KD_UNUSED const KDfloat32 *buffer)
{
    kdSetError(KD_EIO);
    return -1;
}

/******************************************************************************
 * Windowing
 ******************************************************************************/

#ifdef KD_WINDOW_SUPPORTED
/* kdCreateWindow: Create a window. */
#if defined(KD_WINDOW_WIN32)
static LRESULT CALLBACK __kdWindowsWindowCallback(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    switch(msg)
    {
        case WM_CLOSE:
        case WM_DESTROY:
        case WM_QUIT:
        {
            PostQuitMessage(0);
            break;
        }
        default:
        {
            return DefWindowProc(hwnd, msg, wparam, lparam);
        }
    }
    return 0;
}
#elif defined(KD_WINDOW_EMSCRIPTEN)
static EM_BOOL __kd_EmscriptenMouseCallback(KDint type, const EmscriptenMouseEvent *event, KD_UNUSED void *userptr)
{
    KDEvent *kdevent = kdCreateEvent();
    kdevent->userptr = __kd_window->eventuserptr;
    kdevent->type = KD_EVENT_INPUT_POINTER;
    if(type == EMSCRIPTEN_EVENT_MOUSEDOWN || type == EMSCRIPTEN_EVENT_MOUSEUP)
    {
        kdevent->data.inputpointer.index = KD_INPUT_POINTER_SELECT;
        kdevent->data.inputpointer.select = (type == EMSCRIPTEN_EVENT_MOUSEDOWN) ? 1 : 0;

        __kd_window->states.pointer.select = kdevent->data.inputpointer.select;
    }
    else if(type == EMSCRIPTEN_EVENT_MOUSEMOVE)
    {
        kdevent->data.inputpointer.index = KD_INPUT_POINTER_X;
    }
    kdevent->data.inputpointer.x = event->canvasX;
    kdevent->data.inputpointer.y = event->canvasY;

    __kd_window->states.pointer.x = kdevent->data.inputpointer.x;
    __kd_window->states.pointer.y = kdevent->data.inputpointer.y;

    if(!__kdExecCallback(kdevent))
    {
        kdPostEvent(kdevent);
    }
    return 1;
}

static EM_BOOL __kd_EmscriptenKeyboardCallback(KDint type, const EmscriptenKeyboardEvent *event, KD_UNUSED void *userptr)
{
    KDEvent *kdevent = kdCreateEvent();
    kdevent->userptr = __kd_window->eventuserptr;

    KDint32 keycode = __KDKeycodeLookup((KDint32)event->keyCode);
    if(keycode)
    {
        kdevent->type = KD_EVENT_INPUT_KEY_ATX;
        KDEventInputKeyATX *keyevent = (KDEventInputKeyATX *)(&kdevent->data);

        keyevent->flags = 0;
        if(type == EMSCRIPTEN_EVENT_KEYDOWN)
        {
            keyevent->flags |= KD_KEY_PRESS_ATX;
        }
        if(event->location == DOM_KEY_LOCATION_LEFT)
        {
            keyevent->flags |= KD_KEY_LOCATION_LEFT_ATX;
        }
        if(event->location == DOM_KEY_LOCATION_RIGHT)
        {
            keyevent->flags |= KD_KEY_LOCATION_RIGHT_ATX;
        }
        if(event->location == DOM_KEY_LOCATION_NUMPAD)
        {
            keyevent->flags |= KD_KEY_LOCATION_NUMPAD_ATX;
        }
        if(event->shiftKey)
        {
            keyevent->flags |= KD_KEY_MODIFIER_SHIFT_ATX;
        }
        if(event->ctrlKey)
        {
            keyevent->flags |= KD_KEY_MODIFIER_CTRL_ATX;
        }
        if(event->altKey)
        {
            keyevent->flags |= KD_KEY_MODIFIER_ALT_ATX;
        }
        if(event->metaKey)
        {
            keyevent->flags |= KD_KEY_MODIFIER_META_ATX;
        }
        keyevent->keycode = keycode;

        __kd_window->states.keyboard.flags = keyevent->flags;
        __kd_window->states.keyboard.keycode = keyevent->keycode;

        __kdHandleSpecialKeys(__kd_window, keyevent);
    }
    else if(type == EMSCRIPTEN_EVENT_KEYDOWN)
    {
        KDint32 character = 0;
        /* Printable ASCII range. */
        if((event->key[0] >= 20) && (event->key[0] <= 126))
        {
            character = event->key[0];
        }

        if(character)
        {
            kdevent->type = KD_EVENT_INPUT_KEYCHAR_ATX;
            KDEventInputKeyCharATX *keycharevent = (KDEventInputKeyCharATX *)(&kdevent->data);
            keycharevent->flags = 0;
            if(event->repeat)
            {
                keycharevent->flags = KD_KEY_AUTOREPEAT_ATX;
            }
            keycharevent->character = character;

            __kd_window->states.keyboard.charflags = keycharevent->flags;
            __kd_window->states.keyboard.character = keycharevent->character;
        }
        else
        {
            kdFreeEvent(kdevent);
            return 1;
        }
    }
    else
    {
        kdFreeEvent(kdevent);
        return 1;
    }

    if(!__kdExecCallback(kdevent))
    {
        kdPostEvent(kdevent);
    }
    return 1;
}

static EM_BOOL __kd_EmscriptenFocusCallback(KDint type, KD_UNUSED const EmscriptenFocusEvent *event, KD_UNUSED void *user)
{
    if(type == EMSCRIPTEN_EVENT_FOCUSIN)
    {
        __kd_window->properties.focused = 1;
    }
    else if(type == EMSCRIPTEN_EVENT_FOCUSOUT)
    {
        __kd_window->properties.focused = 0;
    }

    KDEvent *kdevent = kdCreateEvent();
    kdevent->userptr = __kd_window->eventuserptr;
    kdevent->type = KD_EVENT_WINDOW_FOCUS;
    kdevent->data.windowfocus.focusstate = __kd_window->properties.focused;

    if(!__kdExecCallback(kdevent))
    {
        kdPostEvent(kdevent);
    }
    return 1;
}

static EM_BOOL __kd_EmscriptenVisibilityCallback(KD_UNUSED KDint type, const EmscriptenVisibilityChangeEvent *event, KD_UNUSED void *user)
{
    __kd_window->properties.visible = !event->hidden;

    KDEvent *kdevent = kdCreateEvent();
    kdevent->userptr = __kd_window->eventuserptr;
    kdevent->type = KD_EVENT_WINDOWPROPERTY_CHANGE;
    kdevent->data.windowproperty.pname = KD_WINDOWPROPERTY_VISIBILITY;

    if(!__kdExecCallback(kdevent))
    {
        kdPostEvent(kdevent);
    }
    return 1;
}

static EM_BOOL __kd_EmscriptenResizeCallback(KD_UNUSED KDint type, KD_UNUSED const EmscriptenUiEvent *event, KD_UNUSED void *user)
{
    KDfloat64KHR width = 0.0;
    KDfloat64KHR height = 0.0;
    emscripten_get_element_css_size("#canvas", &width, (KDfloat64KHR *)&height);
    __kd_window->properties.width = (KDint32)width;
    __kd_window->properties.height = (KDint32)height;
    emscripten_set_canvas_element_size("#canvas", __kd_window->properties.width, __kd_window->properties.height);
    return 1;
}

#elif defined(KD_WINDOW_WAYLAND)
static void __kdWaylandPointerHandleEnter(void *data, KD_UNUSED struct wl_pointer *pointer, KD_UNUSED KDuint32 serial, KD_UNUSED struct wl_surface *surface, KD_UNUSED wl_fixed_t sx, KD_UNUSED wl_fixed_t sy)
{
    struct KDWindow *window = data;
    KDEvent *kdevent = kdCreateEvent();
    kdevent->userptr = window->eventuserptr;
    kdevent->type = KD_EVENT_WINDOW_FOCUS;
    kdevent->data.windowfocus.focusstate = 1;

    if(!__kdExecCallback(kdevent))
    {
        kdPostEvent(kdevent);
    }
}
static void __kdWaylandPointerHandleLeave(void *data, KD_UNUSED struct wl_pointer *pointer, KD_UNUSED KDuint32 serial, KD_UNUSED struct wl_surface *surface)
{
    struct KDWindow *window = data;
    KDEvent *kdevent = kdCreateEvent();
    kdevent->userptr = window->eventuserptr;
    kdevent->type = KD_EVENT_WINDOW_FOCUS;
    kdevent->data.windowfocus.focusstate = 0;

    if(!__kdExecCallback(kdevent))
    {
        kdPostEvent(kdevent);
    }
}
static void __kdWaylandPointerHandleMotion(void *data, KD_UNUSED struct wl_pointer *pointer, KD_UNUSED KDuint32 time, wl_fixed_t sx, wl_fixed_t sy)
{
    static KDuint32 lasttime = 0;
    if((lasttime + 15) < time)
    {
        struct KDWindow *window = data;

        KDEvent *kdevent = kdCreateEvent();
        kdevent->userptr = window->eventuserptr;
        kdevent->type = KD_EVENT_INPUT_POINTER;
        kdevent->data.inputpointer.index = KD_INPUT_POINTER_X;
        kdevent->data.inputpointer.x = wl_fixed_to_int(sx);
        kdevent->data.inputpointer.y = wl_fixed_to_int(sy);

        window->states.pointer.x = kdevent->data.inputpointer.x;
        window->states.pointer.y = kdevent->data.inputpointer.y;

        if(!__kdExecCallback(kdevent))
        {
            kdPostEvent(kdevent);
        }
    }
    lasttime = time;
}

static void __kdWaylandPointerHandleButton(void *data, KD_UNUSED struct wl_pointer *wl_pointer, KD_UNUSED KDuint32 serial, KD_UNUSED KDuint32 time, KD_UNUSED KDuint32 button, KDuint32 state)
{
    struct KDWindow *window = data;
    KDEvent *kdevent = kdCreateEvent();
    kdevent->userptr = window->eventuserptr;
    kdevent->type = KD_EVENT_INPUT_POINTER;
    kdevent->data.inputpointer.index = KD_INPUT_POINTER_SELECT;
    kdevent->data.inputpointer.select = (KDint32)state;
    kdevent->data.inputpointer.x = window->states.pointer.x;
    kdevent->data.inputpointer.y = window->states.pointer.y;

    window->states.pointer.select = kdevent->data.inputpointer.select;

    if(!__kdExecCallback(kdevent))
    {
        kdPostEvent(kdevent);
    }
}

static const struct wl_pointer_listener __kd_wl_pointer_listener = {
    __kdWaylandPointerHandleEnter,
    __kdWaylandPointerHandleLeave,
    __kdWaylandPointerHandleMotion,
    __kdWaylandPointerHandleButton,
    KD_NULL,
#if WAYLAND_VERSION_MAJOR >= 2 || (WAYLAND_VERSION_MAJOR >= 1 && WAYLAND_VERSION_MINOR >= 10)
    KD_NULL,
    KD_NULL,
    KD_NULL,
    KD_NULL
#endif
};

static void __kdWaylandKeyboardHandleKeymap(KD_UNUSED void *data, KD_UNUSED struct wl_keyboard *keyboard, KDuint32 format, KDint fd, KDuint32 size)
{
    struct KDWindow *window = data;
    if(format == WL_KEYBOARD_KEYMAP_FORMAT_XKB_V1)
    {
        KDchar *keymap_string = mmap(KD_NULL, size, PROT_READ, MAP_SHARED, fd, 0);
        window->xkb.keymap = xkb_keymap_new_from_string(window->xkb.context, keymap_string, XKB_KEYMAP_FORMAT_TEXT_V1, XKB_KEYMAP_COMPILE_NO_FLAGS);
        munmap(keymap_string, size);
        window->xkb.state = xkb_state_new(window->xkb.keymap);
    }
}
static void __kdWaylandKeyboardHandleEnter(KD_UNUSED void *data, KD_UNUSED struct wl_keyboard *keyboard, KD_UNUSED KDuint32 serial, KD_UNUSED struct wl_surface *surface, KD_UNUSED struct wl_array *keys) {}
static void __kdWaylandKeyboardHandleLeave(KD_UNUSED void *data, KD_UNUSED struct wl_keyboard *keyboard, KD_UNUSED KDuint32 serial, KD_UNUSED struct wl_surface *surface) {}
static void __kdWaylandKeyboardHandleKey(KD_UNUSED void *data, KD_UNUSED struct wl_keyboard *keyboard, KD_UNUSED KDuint32 serial, KD_UNUSED KDuint32 time, KDuint32 key, KDuint32 state)
{
    struct KDWindow *window = data;

    KDEvent *kdevent = kdCreateEvent();
    kdevent->userptr = window->eventuserptr;

    xkb_keysym_t keysym = xkb_state_key_get_one_sym(window->xkb.state, key + 8);

    KDint32 character = 0;
    if(state == WL_KEYBOARD_KEY_STATE_PRESSED)
    {
        /* Printable ASCII range. */
        if((keysym >= 20) && (keysym <= 126))
        {
            character = (KDint32)keysym;
        }
    }

    if(character)
    {
        kdevent->type = KD_EVENT_INPUT_KEYCHAR_ATX;
        KDEventInputKeyCharATX *keycharevent = (KDEventInputKeyCharATX *)(&kdevent->data);
        keycharevent->character = character;

        __kd_window->states.keyboard.charflags = 0;
        __kd_window->states.keyboard.character = keycharevent->character;
    }
    else
    {
        KDint32 keycode = __KDKeycodeLookup((KDint32)keysym);
        if(keycode)
        {
            kdevent->type = KD_EVENT_INPUT_KEY_ATX;
            KDEventInputKeyATX *keyevent = (KDEventInputKeyATX *)(&kdevent->data);

            if(state == WL_KEYBOARD_KEY_STATE_PRESSED)
            {
                keyevent->flags |= KD_KEY_PRESS_ATX;
            }
            if(xkb_state_mod_name_is_active(window->xkb.state, XKB_MOD_NAME_SHIFT, XKB_STATE_MODS_EFFECTIVE) > 0)
            {
                keyevent->flags |= KD_KEY_MODIFIER_SHIFT_ATX;
            }
            if(xkb_state_mod_name_is_active(window->xkb.state, XKB_MOD_NAME_CTRL, XKB_STATE_MODS_EFFECTIVE) > 0)
            {
                keyevent->flags |= KD_KEY_MODIFIER_CTRL_ATX;
            }
            if(xkb_state_mod_name_is_active(window->xkb.state, XKB_MOD_NAME_ALT, XKB_STATE_MODS_EFFECTIVE) > 0)
            {
                keyevent->flags |= KD_KEY_MODIFIER_ALT_ATX;
            }
            if(xkb_state_mod_name_is_active(window->xkb.state, XKB_MOD_NAME_LOGO, XKB_STATE_MODS_EFFECTIVE) > 0)
            {
                keyevent->flags |= KD_KEY_MODIFIER_META_ATX;
            }

            keyevent->keycode = keycode;

            __kd_window->states.keyboard.flags = keyevent->flags;
            __kd_window->states.keyboard.keycode = keyevent->keycode;

            __kdHandleSpecialKeys(__kd_window, keyevent);
        }
        else
        {
            kdFreeEvent(kdevent);
            return;
        }
    }

    if(!__kdExecCallback(kdevent))
    {
        kdPostEvent(kdevent);
    }
}
static void __kdWaylandKeyboardHandleModifiers(KD_UNUSED void *data, KD_UNUSED struct wl_keyboard *keyboard, KD_UNUSED KDuint32 serial, KDuint32 mods_depressed, KDuint32 mods_latched, KDuint32 mods_locked, KDuint32 group)
{
    struct KDWindow *window = data;
    xkb_state_update_mask(window->xkb.state, mods_depressed, mods_latched, mods_locked, 0, 0, group);
}
#if WAYLAND_VERSION_MAJOR >= 2 || (WAYLAND_VERSION_MAJOR >= 1 && WAYLAND_VERSION_MINOR >= 6)
static void __kdWaylandKeyboardHandleRepeat(KD_UNUSED void *data, KD_UNUSED struct wl_keyboard *wl_keyboard, KD_UNUSED KDint32 rate, KD_UNUSED KDint32 delay)
{
    /* TODO: KD_KEY_AUTOREPEAT_ATX */
}
#endif
static const struct wl_keyboard_listener __kd_wl_keyboard_listener = {
    __kdWaylandKeyboardHandleKeymap,
    __kdWaylandKeyboardHandleEnter,
    __kdWaylandKeyboardHandleLeave,
    __kdWaylandKeyboardHandleKey,
    __kdWaylandKeyboardHandleModifiers,
#if WAYLAND_VERSION_MAJOR >= 2 || (WAYLAND_VERSION_MAJOR >= 1 && WAYLAND_VERSION_MINOR >= 6)
    __kdWaylandKeyboardHandleRepeat
#endif
};

static void __kdWaylandSeatHandleCapabilities(void *data, struct wl_seat *seat, enum wl_seat_capability caps)
{
    struct KDWindow *window = data;
    if(caps & WL_SEAT_CAPABILITY_POINTER)
    {
        window->wayland.pointer = wl_seat_get_pointer(seat);
        wl_pointer_add_listener(window->wayland.pointer, &__kd_wl_pointer_listener, window);
    }
    if(caps & WL_SEAT_CAPABILITY_KEYBOARD)
    {
        window->wayland.keyboard = wl_seat_get_keyboard(seat);
        wl_keyboard_add_listener(window->wayland.keyboard, &__kd_wl_keyboard_listener, window);
    }
}
static const struct wl_seat_listener __kd_wl_seat_listener = {
    __kdWaylandSeatHandleCapabilities,
    KD_NULL};

static void __kdWaylandRegistryAddObject(void *data, struct wl_registry *registry, KDuint32 name, const KDchar *interface, KD_UNUSED KDuint32 version)
{
    struct KDWindow *window = data;
    if(!kdStrcmp(interface, "wl_compositor"))
    {
        window->wayland.compositor = wl_registry_bind(registry, name, &wl_compositor_interface, 1);
        window->wayland.surface = wl_compositor_create_surface(window->wayland.compositor);
    }
    else if(!kdStrcmp(interface, "wl_shell"))
    {
        window->wayland.shell = wl_registry_bind(registry, name, &wl_shell_interface, 1);
        window->wayland.shell_surface = wl_shell_get_shell_surface(window->wayland.shell, window->wayland.surface);
    }
    else if(!kdStrcmp(interface, "wl_seat"))
    {
        window->wayland.seat = wl_registry_bind(registry, name, &wl_seat_interface, 1);
        wl_seat_add_listener(window->wayland.seat, &__kd_wl_seat_listener, window);
    }
}
static const struct wl_registry_listener __kd_wl_registry_listener = {
    __kdWaylandRegistryAddObject,
    KD_NULL};

static void __kdWaylandShellSurfacePing(KD_UNUSED void *data, struct wl_shell_surface *shell_surface, KDuint32 serial)
{
    wl_shell_surface_pong(shell_surface, serial);
}
static void __kdWaylandShellSurfaceConfigure(KD_UNUSED void *data, KD_UNUSED struct wl_shell_surface *shell_surface, KD_UNUSED KDuint32 edges, KD_UNUSED KDint32 width, KD_UNUSED KDint32 height) {}
static void __kdWaylandShellSurfacePopupDone(KD_UNUSED void *data, KD_UNUSED struct wl_shell_surface *shell_surface) {}
static const struct wl_shell_surface_listener __kd_wl_shell_surface_listener = {
    __kdWaylandShellSurfacePing,
    __kdWaylandShellSurfaceConfigure,
    __kdWaylandShellSurfacePopupDone};
#endif

KD_API NativeDisplayType KD_APIENTRY kdGetDisplayVEN(void)
{
#if defined(KD_WINDOW_WAYLAND)
    KDchar *sessiontype = kdGetEnvVEN("XDG_SESSION_TYPE");
    if(kdStrstrVEN(sessiontype, "wayland"))
    {
        __kd_wl_display = wl_display_connect(KD_NULL);
        return (NativeDisplayType)__kd_wl_display;
    }
#endif
    return (NativeDisplayType)EGL_DEFAULT_DISPLAY;
}

KD_API KDWindow *KD_APIENTRY kdCreateWindow(KD_UNUSED EGLDisplay display, KD_UNUSED EGLConfig config, void *eventuserptr)
{
    if(__kd_window)
    {
        /* One window only */
        kdSetError(KD_EPERM);
        return KD_NULL;
    }

    KDWindow *window = (KDWindow *)kdMalloc(sizeof(KDWindow));
    if(window == KD_NULL)
    {
        kdSetError(KD_ENOMEM);
        return KD_NULL;
    }
    window->nativewindow = KD_NULL;
    window->nativedisplay = KD_NULL;
    window->platform = 0;
    if(eventuserptr == KD_NULL)
    {
        window->eventuserptr = window;
    }
    else
    {
        window->eventuserptr = eventuserptr;
    }
    window->originthr = kdThreadSelf();

    kdMemset(&window->properties, 0, sizeof(window->properties));
    kdMemset(&window->states.pointer, 0, sizeof(window->states.pointer));
    kdMemset(&window->states.keyboard, 0, sizeof(window->states.keyboard));

    /* If we have a window assume basic input support. */
    window->states.pointer.availability = 7;
    window->states.keyboard.availability = 15;
    window->states.dpad.availability = 31;
    window->states.gamekeys.availability = 31;

    window->properties.width = 800;
    window->properties.height = 600;

    const KDchar *caption = "OpenKODE";
    kdMemcpy(window->properties.caption, caption, 8);

#if defined(KD_WINDOW_ANDROID)
    eglGetConfigAttrib(display, config, EGL_NATIVE_VISUAL_ID, &window->format);
#elif defined(KD_WINDOW_EMSCRIPTEN)
    KDfloat64KHR width = 0.0;
    KDfloat64KHR height = 0.0;
    emscripten_get_element_css_size("#canvas", &width, (KDfloat64KHR *)&height);
    window->properties.width = (KDint32)width;
    window->properties.height = (KDint32)height;
    emscripten_set_canvas_element_size("#canvas", window->properties.width, window->properties.height);
#elif defined(KD_WINDOW_WIN32)
    WNDCLASS windowclass = {0};
    HINSTANCE instance = GetModuleHandle(KD_NULL);
    GetClassInfo(instance, "", &windowclass);
    windowclass.lpszClassName = window->properties.caption;
    windowclass.lpfnWndProc = __kdWindowsWindowCallback;
    windowclass.hInstance = instance;
    windowclass.hbrBackground = (HBRUSH)COLOR_BACKGROUND;
    RegisterClass(&windowclass);
    window->nativewindow = CreateWindow(window->properties.caption, window->properties.caption, WS_POPUP | WS_VISIBLE, 0, 0,
        window->properties.width, window->properties.height,
        KD_NULL, KD_NULL, instance, KD_NULL);

    window->nativedisplay = GetDC(window->nativewindow);
    /* Activate raw input */
    RAWINPUTDEVICE device[2];
    /* Mouse */
    device[0].usUsagePage = 1;
    device[0].usUsage = 2;
    device[0].dwFlags = RIDEV_NOLEGACY;
    device[0].hwndTarget = window->nativewindow;
    /* Keyboard */
    device[1].usUsagePage = 1;
    device[1].usUsage = 6;
    device[1].dwFlags = RIDEV_NOLEGACY;
    device[1].hwndTarget = window->nativewindow;
    RegisterRawInputDevices(device, 2, sizeof(RAWINPUTDEVICE));
#elif defined(KD_WINDOW_WAYLAND) || defined(KD_WINDOW_X11)
    window->xkb.context = xkb_context_new(XKB_CONTEXT_NO_FLAGS);

#if defined(KD_WINDOW_WAYLAND)
    if(__kd_wl_display)
    {
        window->platform = EGL_PLATFORM_WAYLAND_KHR;
    }
    if(window->platform == EGL_PLATFORM_WAYLAND_KHR)
    {
        window->nativedisplay = __kd_wl_display;
        window->wayland.registry = wl_display_get_registry(window->nativedisplay);
        wl_registry_add_listener(window->wayland.registry, &__kd_wl_registry_listener, window);
        wl_display_roundtrip(window->nativedisplay);
        wl_shell_surface_add_listener(window->wayland.shell_surface, &__kd_wl_shell_surface_listener, window);
    }
#endif

#if defined(KD_WINDOW_X11)
    if(!window->platform)
    {
        /* Fallback to X11*/
        window->platform = EGL_PLATFORM_X11_KHR;
    }
    if(window->platform == EGL_PLATFORM_X11_KHR)
    {
        if(!window->nativedisplay)
        {
            window->nativedisplay = xcb_connect(KD_NULL, KD_NULL);
        }
        xcb_screen_t *screen = xcb_setup_roots_iterator(xcb_get_setup(window->nativedisplay)).data;
        window->nativewindow = (void *)(KDuintptr)xcb_generate_id(window->nativedisplay);

        KDuint32 mask = XCB_CW_EVENT_MASK;
        KDuint32 values[1] = {XCB_EVENT_MASK_POINTER_MOTION |
            XCB_EVENT_MASK_BUTTON_PRESS | XCB_EVENT_MASK_BUTTON_RELEASE |
            XCB_EVENT_MASK_KEY_PRESS | XCB_EVENT_MASK_KEY_RELEASE |
            XCB_EVENT_MASK_ENTER_WINDOW | XCB_EVENT_MASK_LEAVE_WINDOW};

        xcb_create_window(window->nativedisplay, screen->root_depth, (KDuint)(KDuintptr)window->nativewindow,
            screen->root, 0, 0, (KDuint16)window->properties.width, (KDuint16)window->properties.height, 0, XCB_WINDOW_CLASS_INPUT_OUTPUT, screen->root_visual, mask, values);

        xcb_intern_atom_cookie_t *ewmhcookie = xcb_ewmh_init_atoms(window->nativedisplay, &window->xcb.ewmh);
        xcb_ewmh_init_atoms_replies(&window->xcb.ewmh, ewmhcookie, KD_NULL);

        xcb_intern_atom_cookie_t protcookie = xcb_intern_atom(window->nativedisplay, 1, 12, "WM_PROTOCOLS");
        xcb_intern_atom_reply_t *protreply = xcb_intern_atom_reply(window->nativedisplay, protcookie, 0);
        xcb_intern_atom_cookie_t delcookie = xcb_intern_atom(window->nativedisplay, 0, 16, "WM_DELETE_WINDOW");
        xcb_intern_atom_reply_t *delreply = xcb_intern_atom_reply(window->nativedisplay, delcookie, 0);
        xcb_change_property(window->nativedisplay, XCB_PROP_MODE_REPLACE, (KDuint)(KDuintptr)window->nativewindow, (*protreply).atom, 4, 32, 1, &(*delreply).atom);

        xkb_x11_setup_xkb_extension(window->nativedisplay, XKB_X11_MIN_MAJOR_XKB_VERSION, XKB_X11_MIN_MINOR_XKB_VERSION,
            XKB_X11_SETUP_XKB_EXTENSION_NO_FLAGS, KD_NULL, KD_NULL, &window->xkb.firstevent, KD_NULL);
        KDint32 device = xkb_x11_get_core_keyboard_device_id(window->nativedisplay);
        window->xkb.keymap = xkb_x11_keymap_new_from_device(window->xkb.context, window->nativedisplay, device, XKB_KEYMAP_COMPILE_NO_FLAGS);
        window->xkb.state = xkb_x11_state_new_from_device(window->xkb.keymap, window->nativedisplay, device);

        enum {
            required_events =
                (XCB_XKB_EVENT_TYPE_NEW_KEYBOARD_NOTIFY |
                    XCB_XKB_EVENT_TYPE_MAP_NOTIFY |
                    XCB_XKB_EVENT_TYPE_STATE_NOTIFY),

            required_map_parts =
                (XCB_XKB_MAP_PART_KEY_TYPES |
                    XCB_XKB_MAP_PART_KEY_SYMS |
                    XCB_XKB_MAP_PART_MODIFIER_MAP |
                    XCB_XKB_MAP_PART_EXPLICIT_COMPONENTS |
                    XCB_XKB_MAP_PART_KEY_ACTIONS |
                    XCB_XKB_MAP_PART_VIRTUAL_MODS |
                    XCB_XKB_MAP_PART_VIRTUAL_MOD_MAP),

            required_state_details =
                (XCB_XKB_STATE_PART_MODIFIER_BASE |
                    XCB_XKB_STATE_PART_MODIFIER_LATCH |
                    XCB_XKB_STATE_PART_MODIFIER_LOCK |
                    XCB_XKB_STATE_PART_GROUP_BASE |
                    XCB_XKB_STATE_PART_GROUP_LATCH |
                    XCB_XKB_STATE_PART_GROUP_LOCK),
        };

        static const xcb_xkb_select_events_details_t details = {
            .affectState = required_state_details,
            .stateDetails = required_state_details,
        };

        xcb_xkb_select_events_aux(window->nativedisplay, (KDuint16)device, required_events, 0, 0,
            required_map_parts, required_map_parts, &details);
    }
#endif
#endif
    __kd_window = window;
    return window;
}

/* kdDestroyWindow: Destroy a window. */
KD_API KDint KD_APIENTRY kdDestroyWindow(KDWindow *window)
{
    if(window->originthr != kdThreadSelf())
    {
        kdSetError(KD_EINVAL);
        return -1;
    }
#if defined(KD_WINDOW_WIN32)
    ReleaseDC(window->nativewindow, window->nativedisplay);
    DestroyWindow(window->nativewindow);
#elif defined(KD_WINDOW_WAYLAND) || defined(KD_WINDOW_X11)
    xkb_state_unref(window->xkb.state);
    xkb_keymap_unref(window->xkb.keymap);
    xkb_context_unref(window->xkb.context);

#if defined(KD_WINDOW_WAYLAND)
    if(window->platform == EGL_PLATFORM_WAYLAND_KHR)
    {
        if(window->wayland.keyboard)
        {
            wl_keyboard_destroy(window->wayland.keyboard);
        }
        if(window->wayland.pointer)
        {
            wl_pointer_destroy(window->wayland.pointer);
        }
        if(window->wayland.seat)
        {
            wl_seat_destroy(window->wayland.seat);
        }
        if(window->wayland.shell)
        {
            wl_shell_destroy(window->wayland.shell);
        }
        if(window->wayland.compositor)
        {
            wl_compositor_destroy(window->wayland.compositor);
        }
        wl_egl_window_destroy(window->nativewindow);
        wl_shell_surface_destroy(window->wayland.shell_surface);
        wl_surface_destroy(window->wayland.surface);
        wl_registry_destroy(window->wayland.registry);
        wl_display_disconnect(window->nativedisplay);
    }
#endif
#if defined(KD_WINDOW_X11)
    if(window->platform == EGL_PLATFORM_X11_KHR)
    {
        xcb_disconnect(window->nativedisplay);
    }
#endif
#endif
    kdFree(window);
    __kd_window = KD_NULL;
    return 0;
}

/* kdSetWindowPropertybv, kdSetWindowPropertyiv, kdSetWindowPropertycv: Set a window property to request a change in the on-screen representation of the window. */
KD_API KDint KD_APIENTRY kdSetWindowPropertybv(KDWindow *window, KDint pname, KD_UNUSED const KDboolean *param)
{
    if(pname == KD_WINDOWPROPERTY_FULLSCREEN_NV)
    {
#if defined(KD_WINDOW_EMSCRIPTEN)
        if(param[0])
        {
            EmscriptenFullscreenStrategy strategy;
            kdMemset(&strategy, 0, sizeof(strategy));
            strategy.scaleMode = EMSCRIPTEN_FULLSCREEN_SCALE_DEFAULT;
            strategy.canvasResolutionScaleMode = EMSCRIPTEN_FULLSCREEN_CANVAS_SCALE_NONE;
            strategy.filteringMode = EMSCRIPTEN_FULLSCREEN_FILTERING_DEFAULT;
            emscripten_enter_soft_fullscreen(0, &strategy);
        }
        else
        {
            emscripten_exit_soft_fullscreen();
        }
#elif defined(KD_WINDOW_WAYLAND) || defined(KD_WINDOW_X11)
#if defined(KD_WINDOW_WAYLAND)
        if(window->platform == EGL_PLATFORM_WAYLAND_KHR)
        {
            if(param[0])
            {
                wl_shell_surface_set_fullscreen(window->wayland.shell_surface, WL_SHELL_SURFACE_FULLSCREEN_METHOD_DEFAULT, 0, KD_NULL);
            }
            else
            {
                wl_shell_surface_set_toplevel(window->wayland.shell_surface);
            }
        }
#endif
#if defined(KD_WINDOW_X11)
        if(window->platform == EGL_PLATFORM_X11_KHR)
        {
            if(param[0])
            {
                xcb_change_property(window->nativedisplay, XCB_PROP_MODE_REPLACE, (KDuint)(KDuintptr)window->nativewindow,
                    window->xcb.ewmh._NET_WM_STATE, XCB_ATOM_ATOM, 32, 1, &(window->xcb.ewmh._NET_WM_STATE_FULLSCREEN));
                xcb_flush(window->nativedisplay);
            }
        }
#endif
#endif
        window->properties.fullscreen = param[0];

        KDEvent *event = kdCreateEvent();
        event->type = KD_EVENT_WINDOWPROPERTY_CHANGE;
        event->data.windowproperty.pname = KD_WINDOWPROPERTY_FULLSCREEN_NV;
        kdPostEvent(event);
        return 0;
    }
    kdSetError(KD_EINVAL);
    return -1;
}

KD_API KDint KD_APIENTRY kdSetWindowPropertyiv(KDWindow *window, KDint pname, const KDint32 *param)
{
    if(pname == KD_WINDOWPROPERTY_SIZE)
    {
#if defined(KD_WINDOW_WIN32)
        SetWindowPos(window->nativewindow, 0, 0, 0, param[0], param[1], 0);
#elif defined(KD_WINDOW_EMSCRIPTEN)
        emscripten_set_canvas_element_size("#canvas", param[0], param[1]);
#elif defined(KD_WINDOW_WAYLAND) || defined(KD_WINDOW_X11)
#if defined(KD_WINDOW_WAYLAND)
        if(window->platform == EGL_PLATFORM_WAYLAND_KHR)
        {
            if(window->nativewindow)
            {
                wl_egl_window_resize(window->nativewindow, param[0], param[1], 0, 0);
            }
        }
#endif
#if defined(KD_WINDOW_X11)
        if(window->platform == EGL_PLATFORM_X11_KHR)
        {
            xcb_size_hints_t hints;
            kdMemset(&hints, 0, sizeof(hints));
            xcb_icccm_size_hints_set_min_size(&hints, param[0], param[1]);
            xcb_icccm_size_hints_set_base_size(&hints, param[0], param[1]);
            xcb_icccm_size_hints_set_max_size(&hints, param[0], param[1]);
            xcb_icccm_size_hints_set_size(&hints, 0, param[0], param[1]);
            xcb_icccm_size_hints_set_position(&hints, 0, 0, 0);
            xcb_icccm_set_wm_size_hints(window->nativedisplay, (KDuint)(KDuintptr)window->nativewindow, XCB_ATOM_WM_NORMAL_HINTS, &hints);
            xcb_flush(window->nativedisplay);
        }
#endif
#endif
        window->properties.width = param[0];
        window->properties.height = param[1];

        KDEvent *event = kdCreateEvent();
        event->type = KD_EVENT_WINDOWPROPERTY_CHANGE;
        event->data.windowproperty.pname = KD_WINDOWPROPERTY_SIZE;
        kdPostEvent(event);
        return 0;
    }
    kdSetError(KD_EINVAL);
    return -1;
}

KD_API KDint KD_APIENTRY kdSetWindowPropertycv(KDWindow *window, KDint pname, const KDchar *param)
{
    if(pname == KD_WINDOWPROPERTY_CAPTION)
    {
#if defined(KD_WINDOW_WIN32)
        SetWindowTextA(window->nativewindow, param);
#elif defined(KD_WINDOW_WAYLAND) || defined(KD_WINDOW_X11)
#if defined(KD_WINDOW_WAYLAND)
        if(window->platform == EGL_PLATFORM_WAYLAND_KHR)
        {
            wl_shell_surface_set_title(window->wayland.shell_surface, param);
        }
#endif
#if defined(KD_WINDOW_X11)
        if(window->platform == EGL_PLATFORM_X11_KHR)
        {
            xcb_icccm_set_wm_name(window->nativedisplay, (KDuint)(KDuintptr)window->nativewindow, XCB_ATOM_STRING, 8, (KDuint32)kdStrlen(window->properties.caption), window->properties.caption);
            xcb_flush(window->nativedisplay);
        }
#endif
#endif
        kdMemcpy(window->properties.caption, param, kdStrlen(param));

        KDEvent *event = kdCreateEvent();
        event->type = KD_EVENT_WINDOWPROPERTY_CHANGE;
        event->data.windowproperty.pname = KD_WINDOWPROPERTY_CAPTION;
        kdPostEvent(event);
        return 0;
    }
    kdSetError(KD_EINVAL);
    return -1;
}

/* kdGetWindowPropertybv, kdGetWindowPropertyiv, kdGetWindowPropertycv: Get the current value of a window property. */
KD_API KDint KD_APIENTRY kdGetWindowPropertybv(KDWindow *window, KDint pname, KDboolean *param)
{
    if(pname == KD_WINDOWPROPERTY_FOCUS)
    {
        param[0] = window->properties.focused;
    }
    else if(pname == KD_WINDOWPROPERTY_VISIBILITY)
    {
        param[0] = window->properties.visible;
    }
    else if(pname == KD_WINDOWPROPERTY_FULLSCREEN_NV)
    {
        param[0] = window->properties.fullscreen;
    }
    kdSetError(KD_EINVAL);
    return -1;
}
KD_API KDint KD_APIENTRY kdGetWindowPropertyiv(KDWindow *window, KDint pname, KDint32 *param)
{
    if(pname == KD_WINDOWPROPERTY_SIZE)
    {
        param[0] = window->properties.width;
        param[1] = window->properties.height;
    }
    kdSetError(KD_EINVAL);
    return -1;
}
KD_API KDint KD_APIENTRY kdGetWindowPropertycv(KDWindow *window, KDint pname, KDchar *param, KDsize *size)
{
    if(pname == KD_WINDOWPROPERTY_CAPTION)
    {
        *size = kdStrlen(window->properties.caption);
        kdMemcpy(param, window->properties.caption, *size);
    }
    kdSetError(KD_EINVAL);
    return -1;
}

/* kdRealizeWindow: Realize the window as a displayable entity and get the native window handle for passing to EGL. */
KD_API KDint KD_APIENTRY kdRealizeWindow(KDWindow *window, EGLNativeWindowType *nativewindow)
{
    KDint32 windowsize[2];
    windowsize[0] = window->properties.width;
    windowsize[1] = window->properties.height;
    kdSetWindowPropertyiv(window, KD_WINDOWPROPERTY_SIZE, windowsize);
    kdSetWindowPropertycv(window, KD_WINDOWPROPERTY_CAPTION, window->properties.caption);
    window->properties.focused = 1;
    window->properties.visible = 1;

#if defined(KD_WINDOW_ANDROID)
    for(;;)
    {
        kdThreadMutexLock(__kd_androidwindow_mutex);
        if(__kd_androidwindow)
        {
            window->nativewindow = __kd_androidwindow;
            kdThreadMutexUnlock(__kd_androidwindow_mutex);
            break;
        }
        kdThreadMutexUnlock(__kd_androidwindow_mutex);
    }
    ANativeWindow_setBuffersGeometry(window->nativewindow, 0, 0, window->format);
#elif defined(KD_WINDOW_EMSCRIPTEN)
    emscripten_set_mousedown_callback(0, 0, 1, __kd_EmscriptenMouseCallback);
    emscripten_set_mouseup_callback(0, 0, 1, __kd_EmscriptenMouseCallback);
    emscripten_set_mousemove_callback(0, 0, 1, __kd_EmscriptenMouseCallback);
    emscripten_set_keydown_callback(0, 0, 1, __kd_EmscriptenKeyboardCallback);
    emscripten_set_keyup_callback(0, 0, 1, __kd_EmscriptenKeyboardCallback);
    emscripten_set_focusin_callback(0, 0, 1, __kd_EmscriptenFocusCallback);
    emscripten_set_focusout_callback(0, 0, 1, __kd_EmscriptenFocusCallback);
    emscripten_set_visibilitychange_callback(0, 1, __kd_EmscriptenVisibilityCallback);
    emscripten_set_resize_callback(0, 0, 1, __kd_EmscriptenResizeCallback);
#elif defined(KD_WINDOW_WAYLAND) || defined(KD_WINDOW_X11)
#if defined(KD_WINDOW_WAYLAND)
    if(window->platform == EGL_PLATFORM_WAYLAND_KHR)
    {
        window->nativewindow = wl_egl_window_create(window->wayland.surface, window->properties.width, window->properties.height);
    }
#endif
#if defined(KD_WINDOW_X11)
    if(window->platform == EGL_PLATFORM_X11_KHR)
    {
        xcb_map_window(window->nativedisplay, (KDuint)(KDuintptr)window->nativewindow);
        xcb_flush(window->nativedisplay);
    }
#endif
#endif

    KDEvent *kdevent = kdCreateEvent();
    kdevent->userptr = window->eventuserptr;
    kdevent->type = KD_EVENT_WINDOWPROPERTY_CHANGE;
    kdevent->data.windowproperty.pname = KD_EVENT_WINDOW_REDRAW;

    if(!__kdExecCallback(kdevent))
    {
        kdPostEvent(kdevent);
    }

    if(nativewindow)
    {
        *nativewindow = (EGLNativeWindowType)window->nativewindow;
    }
    return 0;
}

#endif
