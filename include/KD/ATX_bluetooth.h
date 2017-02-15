
/*******************************************************
 * OpenKODE Core extension: KD_ATX_bluetooth
 *******************************************************/
/* Sample KD/ATX_bluetooth.h for OpenKODE Core */
#ifndef __kd_ATX_bluetooth_h_
#define __kd_ATX_bluetooth_h_
#include <KD/kd.h>

#ifdef __cplusplus
extern "C" {
#endif


typedef struct KDUuidATX {
    KDuint32 i1, i2, i3, i4;
} KDUuidATX;
#define KD_SOCK_RFCOMM_ATX 169

#define KD_AF_BLUETOOTH_ATX 170

/* This struct declaration needs to be put into
* KD/kd.h.

typedef struct KDBdAddrATX {
    KDuint8 b[6];
} KDBdAddrATX;

*/

/* This struct declaration needs to be put into
* KD/kd.h, and a member of this type called sbtrcATX needs to be put into
* KDSockaddr's data union in KD/kd.h.

typedef struct KDSockaddrAfBtrcATX {
    KDBdAddrATX bdaddr;
    KDuint8 channel;
} KDSockaddrAfBtrcATX;

*/


/* kdBtInquireDevicesATX: Start an inquiry for remote Bluetooth devices. */
typedef struct KDBtLocalDeviceATX KDBtLocalDeviceATX;

KD_API KDint KD_APIENTRY kdBtInquireDevicesATX(KDBtLocalDeviceATX *localdevice, KDint access, void *eventuserptr);
#define KD_BT_GIAC_ATX 0x9e8b33

/* kdBtCancelInquireDevicesATX: Selectively cancel ongoing kdBtInquireDevicesATX operations. */
KD_API void KD_APIENTRY kdBtCancelInquireDevicesATX(void *eventuserptr);

/* kdBtGetFriendlyNameATX: Get the user-friendly name of a Bluetooth device. */
KD_API KDint KD_APIENTRY kdBtGetFriendlyNameATX(KDBtLocalDeviceATX *localdevice, const KDBdAddrATX *bdaddr, void *eventuserptr);

/* kdBtCancelGetFriendlyNameATX: Selectively cancel ongoing kdBtGetFriendlyNameATX operations. */
KD_API void KD_APIENTRY kdBtCancelGetFriendlyNameATX(void *eventuserptr);

/* kdBtSearchServicesATX: Search services on a remote Bluetooth device. */
KD_API KDint KD_APIENTRY kdBtSearchServicesATX(KDBtLocalDeviceATX *localdevice, const KDBdAddrATX *bdaddr, const KDint32 *attrset, const KDUuidATX *uuidset, void *eventuserptr);

/* kdBtCancelSearchServicesATX: Cancel an outstanding Bluetooth service search. */
KD_API void KD_APIENTRY kdBtCancelSearchServicesATX(void *eventuserptr);

/* kdBtServiceRecordGetRfcommChannelATX: Get the RFCOMM channel from a Bluetooth service record. */
typedef struct KDBtServiceRecordATX KDBtServiceRecordATX;
KD_API KDint KD_APIENTRY kdBtServiceRecordGetRfcommChannelATX(KDBtServiceRecordATX *servicerecord);

/* kdBtServiceRecordFreeATX: Free a Bluetooth service record. */
KD_API void KD_APIENTRY kdBtServiceRecordFreeATX(KDBtServiceRecordATX *servicerecord);

/* kdBtServiceRecordCreateRfcommATX: Create a Bluetooth service record for an RFCOMM service. */
KD_API KDBtServiceRecordATX *KD_APIENTRY kdBtServiceRecordCreateRfcommATX(const KDUuidATX *uuid, KDint channel);

/* kdBtRegisterServiceATX: Register a Bluetooth service. */
KD_API KDint KD_APIENTRY kdBtRegisterServiceATX(KDBtLocalDeviceATX *localdevice, KDBtServiceRecordATX *servicerecord);

/* kdBtSocketSetFlagsATX: Set Bluetooth socket flags for authentication, encryption and/or authorization. */
KD_API KDint KD_APIENTRY kdBtSocketSetFlagsATX(KDSocket *socket, KDuint flags);
#define KD_BTFLAG_AUTHENTICATE 1
#define KD_BTFLAG_ENCRYPT 2
#define KD_BTFLAG_AUTHORIZE 4


/* kdBtSetDiscoverableATX: Set the discoverable mode of the device. */
KD_API KDint KD_APIENTRY kdBtSetDiscoverableATX(KDBtLocalDeviceATX *localdevice, KDuint mode);
#define KD_BT_NOT_DISCOVERABLE_ATX 0
            

/* KD_EVENT_BT_DEVICE_DISCOVERED_ATX: kdBtInquireDevicesATX has found a device or has completed event. */
#define KD_EVENT_BT_DEVICE_DISCOVERED_ATX 171
typedef struct KDBtRemoteDeviceInfoATX {
    KDBdAddrATX bdaddr;
    KDuint8 deviceclass[3];
} KDBtRemoteDeviceInfoATX;
/* This needs to be in KD/kd.h, with a new field of this
 * type called btdeviceATX added to the event data union:
typedef struct KDEventBtDeviceATX {
    KDint32 error;
    const struct KDBtRemoteDeviceInfoATX *result;
} KDEventBtDeviceATX;
 */

/* KD_EVENT_BT_NAME_ATX: kdBtGetFriendlyNameATX complete event. */
#define KD_EVENT_BT_NAME_ATX 172
/* This needs to be in KD/kd.h, with a new field of this
 * type called btnameATX added to the event data union:
typedef struct KDEventBtNameATX {
    KDint32 error;
    const KDchar *result;
} KDEventBtNameATX;
 */

/* KD_EVENT_BT_SERVICE_DISCOVERED_ATX: kdBtSearchServicesATX has found a service or has completed event. */
#define KD_EVENT_BT_SERVICE_DISCOVERED_ATX 173
/* This needs to be in KD/kd.h, with a new field of
 * this type called btserviceATX added to the event data union:
typedef struct KDEventBtServiceATX {
    KDint32 error;
    struct KDBtServiceRecordATX *result;
} KDEventBtServiceATX;
 */

#ifdef __cplusplus
}
#endif

#endif /* __kd_ATX_bluetooth_h_ */

