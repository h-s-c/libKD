#ifndef KD_QNX_INPUT_H_
#define KD_QNX_INPUT_H_
#include <KD/kd.h>

#ifndef KD_EVENT_UNDEFINED_QNX
#define KD_EVENT_UNDEFINED_QNX 0x30000000
#endif

#ifdef __cplusplus
extern "C" {
#endif


#define KD_IOGROUP_ASSIGN_GROUP_QNX             (KD_IO_UNDEFINED + 0)
#define KD_STATE_ASSIGN_GROUP_AVAILABILITY_QNX  (KD_IOGROUP_ASSIGN_GROUP_QNX + 0)
#define KD_OUTPUT_ASSIGN_POINTER_QNX            (KD_IOGROUP_ASSIGN_GROUP_QNX + 1)
#define KD_OUTPUT_ASSIGN_JOGDIAL_QNX            (KD_IOGROUP_ASSIGN_GROUP_QNX + 2)
#define KD_OUTPUT_ASSIGN_KEYBOARD_QNX           (KD_IOGROUP_ASSIGN_GROUP_QNX + 3)
#define KD_OUTPUT_ASSIGN_MOUSE_QNX              (KD_IOGROUP_ASSIGN_GROUP_QNX + 4)
#define KD_OUTPUT_ASSIGN_TOUCH_QNX              (KD_IOGROUP_ASSIGN_GROUP_QNX + 5)

#define KD_IOGROUP_DISP_GROUP_QNX               (KD_IO_UNDEFINED + 0x500)
#define KD_STATE_DISP_GROUP_AVAILABILITY_QNX    (KD_IOGROUP_DISP_GROUP_QNX + 0)
#define KD_OUTPUT_DISP_POINTER_QNX              (KD_IOGROUP_DISP_GROUP_QNX + 1)
#define KD_OUTPUT_DISP_JOGDIAL_QNX              (KD_IOGROUP_DISP_GROUP_QNX + 2)
#define KD_OUTPUT_DISP_KEYBOARD_QNX             (KD_IOGROUP_DISP_GROUP_QNX + 3)
#define KD_OUTPUT_DISP_MOUSE_QNX                (KD_IOGROUP_DISP_GROUP_QNX + 4)
#define KD_OUTPUT_DISP_TOUCH_QNX                (KD_IOGROUP_DISP_GROUP_QNX + 5)

#define KD_EVENT_KEYBOARD_QNX                   (KD_EVENT_UNDEFINED_QNX + 1000)

typedef struct KDEventKeyboardQNX {
    KDint32     index;
    KDuint32    flags;
    KDuint32    modifiers;
    KDuint32    key_cap;
    KDuint32    key_scan;
    KDuint32    key_sym;
} KDEventKeyboardQNX;

#define KD_IOGROUP_KEYBOARD_QNX                 (KD_IO_UNDEFINED + 0x1000)
#define KD_STATE_KEYBOARD_AVAILABILITY_QNX      (KD_IOGROUP_KEYBOARD_QNX + 0)
#define KD_INPUT_KEYBOARD_FLAGS_QNX             (KD_IOGROUP_KEYBOARD_QNX + 1)
#define KD_INPUT_KEYBOARD_MODIFIERS_QNX         (KD_IOGROUP_KEYBOARD_QNX + 2)
#define KD_INPUT_KEYBOARD_KEY_CAP_QNX           (KD_IOGROUP_KEYBOARD_QNX + 3)
#define KD_INPUT_KEYBOARD_KEY_SCAN_QNX          (KD_IOGROUP_KEYBOARD_QNX + 4)
#define KD_INPUT_KEYBOARD_KEY_SYM_QNX           (KD_IOGROUP_KEYBOARD_QNX + 5)

#define KD_IOGROUP_MOUSE_QNX                    (KD_IO_UNDEFINED + 0x1500)

#define KD_IOGROUP_TOUCH_QNX                    (KD_IO_UNDEFINED + 0x2000)

KD_API KDint32 KD_APIENTRY kdOutputSetpQNX(KDint32 startidx, KDuint32 numidxs, const void *buffer);

#ifdef __cplusplus
}
#endif

#endif /* KD_QNX_INPUT_H_ */
