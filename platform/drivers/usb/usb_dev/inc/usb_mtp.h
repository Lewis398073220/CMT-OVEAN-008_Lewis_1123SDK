/***************************************************************************
 * Copyright 2015-2022 BES.
 * All rights reserved. All unpublished rights reserved.
 *
 * No part of this work may be used or reproduced in any form or by any
 * means, or stored in a database or retrieval system, without prior written
 * permission of BES.
 *
 * Use of this work is governed by a license granted by BES.
 * This work contains confidential and proprietary information of
 * BES. which is protected by copyright, trade secret,
 * trademark and other intellectual property rights.
 ***************************************************************************/
#ifndef __USB_MTP_H__
#define __USB_MTP_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "plat_types.h"
#include "list.h"

#define MTP_MAX_PATH_LEN  256
#define MTP_MAX_NAME_LEN  64
#define MAX_DIR_ELEMENT   50
#define MAX_EPBULK_SEND_SIZE (8192)
#define MAX_EPBULK_RECV_SIZE (8192)

enum USB_MTP_API_MODE {
    USB_MTP_API_NONBLOCKING = 0,
    USB_MTP_API_BLOCKING,
};

// MTP DEV DATA STATE
enum DEV_STATE {
    /* initial state, disconnected */
    STATE_OFFLINE,
    /* ready for userspace calls */
    STATE_READY,
    /* processing userspace calls */
    STATE_BUSY,
    /* transaction canceled by host */
    STATE_CANCELED,
    /* error from completion routine */
    STATE_ERROR,
};

// MTP Bulk Stage
enum Stage {
    /* wait a command or data */
    READ_DATA,
    /* process a command or data */
    PROCESS_DATA,
    /* response a command or data */
    RESPONSE_DATA,
};

typedef struct{
    /* length of packet, including this header */
    uint32_t length;
    /* container type (2 for data packet) */
    uint16_t type;
    /* MTP command code */
    uint16_t command;
    /* MTP transaction ID */
    uint32_t transaction_id;
    /* MTP payload params */
    uint32_t param[0];
} PACKED MTP_DATA_PACKET;

typedef struct
{
    uint32_t storage_id;
    uint32_t object_format;
    size_t   object_size;
    uint32_t parent_handle;
    uint32_t self_handle;
    char     display_name[MTP_MAX_NAME_LEN+1];
    uint64_t last_modified;
    bool     scanned;
} mtp_db_entry_t;

typedef struct {
    uint32_t handle;
    uint32_t format;
    mtp_db_entry_t *dbentry;
} mtp_obj_handle_t;

typedef struct
{
    uint32_t m_storage_id;
    char     m_description[MTP_MAX_NAME_LEN+1];
    uint64_t m_max_capacity;
    uint64_t m_max_file_size;
    uint64_t m_free_space;
    bool     m_removable;
    uint64_t m_reserve_space;
    int32_t  m_access_capability;
} mtp_storage_t;

typedef struct
{
    int32_t  m_handle;
    uint32_t m_storage_id;
    uint16_t m_format;
    uint32_t m_parent;
    uint32_t m_compressed_size;
    char     m_name[MTP_MAX_NAME_LEN+1];
    int64_t  m_date_created;
    int64_t  m_date_modified;
#if 0
    uint16_t m_protection_status;
    uint16_t m_thumb_format;
    uint32_t m_thumb_compressed_size;
    uint32_t m_thumb_pix_width;
    uint32_t m_thumb_pix_height;
    uint32_t m_image_pix_width;
    uint32_t m_image_pix_height;
    uint32_t m_image_pix_depth;
    uint16_t m_association_type;
    uint32_t m_association_desc;
    uint32_t m_sequence_number;
    char     m_key_words[MTP_MAX_NAME_LEN+1];
#endif
}mtp_obj_info_t;

typedef struct
{
    bool         mutex_enabled;
    void        *mutex;
    list_t      *list;
    list_node_t *node;
} mtp_obj_handle_list_t;

#define MTP_GET_DEVICE_STATUS   0X67
#define MTP_CANCEL_REQUEST      0X64

/* MTP DATA TYPE */
#define COMMADN                 1
#define DATA                    2
#define RESPONSE                3
#define EVENT                   4

/* MTP COMMAND */
#define GetDeviceInfo           0x1001
#define OpenSession             0x1002
#define CloseSession            0x1003
#define GetStorageIDs           0x1004
#define GetStorageInfo          0x1005
#define GetNumObjects           0x1006
#define GetObjectHandles        0x1007
#define GetObjectInfo           0x1008
#define GetObject               0x1009
#define GetThumb                0x100A
#define DeleteObject            0x100B
#define SendObjectInfo          0x100C
#define SendObject              0x100D
#define GetDevicePropDesc       0x1014
#define GetDevicePropValue      0x1015
#define SetDevicePropValue      0x1016
#define ResetDevicePropValue    0x1017
#define GetPartialObject        0x101B
#define GetObjectPropsSupported 0x9801
#define GetObjectPropDesc       0x9802
#define GetObjectPropValue      0x9803
#define SetObjectPropValue      0x9804
#define GetObjectPropList       0x9805
#define GetObjectReferences     0x9810
#define SetObjectReferences     0x9811
#define Vendor1                 0x95C1
#define Vendor2                 0x95C2
#define Vendor3                 0x95C3
#define Vendor4                 0x95C4
#define Vendor5                 0x95C5

/* MTP EVENT */
#define ObjectInfoChanged       0x4007
#define ObjectAdded             0x4002
#define ObjectRemoved           0x4003

/* MTP Device Properties Supported */
#define SynchronizationPartner  0xD401
#define DeviceFriendlyName      0xD402
#define ImageSize               0x5003

/* MTP Image formats Supported */
#define Undefined               0x3000
#define Association             0x3001
#define Text                    0x3004
#define HTML                    0x3005
#define WAV                     0x3008
#define MP3                     0x3009
#define MPEG                    0x300B
#define JPEG                    0x3801
#define EP                      0x3802
#define BMP                     0x3804
#define GIF                     0x3807
#define JFIF                    0x3808
#define PNG                     0x380B
#define TIFF                    0x380D
#define WMA                     0xB901
#define OGG                     0xB902
#define AAC                     0xB903
#define MP4                     0xB982
#define MP2                     0xB983
#define _3GP                    0xB984
#define Audio_Video             0xBA05
#define WPL                     0xBA10
#define M3U                     0xBA11
#define PLS                     0xBA14
#define XML                     0xBA82
#define FLAC                    0xB906

/* MTP Object Prop Code */
#define StorageID               0xDC01
#define ObjectFormat            0xDC02
#define ProtectionStatus        0xDC03
#define ObjectSize              0xDC04
#define ObjectFileName          0xDC07
#define DataModified            0xDC09
#define PersisUniqObjIndent     0xDC41
#define ParentObject            0xDC0B
#define Name                    0xDC44
#define Description             0xDC48
#define DisplayName             0xDCE0
#define DateAdded               0xDC4E
#define Artist                  0xDC46
#define AlbumName               0xDC9A
#define AlbumArtist             0xDC9B
#define Track                   0xDC8B
#define OriginalRelesDate       0xDC99
#define Duration                0xDC89
#define Genre                   0xDC8C
#define Composer                0xDC96
#define AudioWAVECodec          0xDE99
#define BitrateType             0xDE92
#define AudioBitRate            0xDE9A
#define NumofChannels           0xDE94
#define SampleRate              0xDE93
#define Description             0xDC48
#define DevFriendlyName         0xD402

/* MTP RESPONSE */
#define OK                      0x2001
#define General_Error           0x2002
#define OperationNotSupported   0x2005
#define Device_busy             0x2019
#define InvalidDevPropValue     0x201C

/* MTP DEVICE INFO */
#define StandardVersion         0x0064
#define VendExtID               0x00000006
#define VendExtVersion          0x0064
#define FuncMode                0x0000
#define OpsSuppCount            0x0000001E
#define EventSuppCount          0x00000000
#define DPSSuppCount            0x00000003
#define CaptureSuppCount        0x00000000
#define ImageSuppCount          0x0000001A

/* MTP DEVICE Prop Desc */
/* Date Type */
#define UINT16                  0x0004
#define UINT32                  0x0006
#define UINT64                  0x0008
#define UINT128                 0x000A
#define STR                     0xFFFF

/* MTP Storage Type*/
#define FixedROM                0x0001
#define RemovableROM            0x0002  //1 or 2 is read only
#define FixedRAM                0x0003
#define RemovableRAM            0x0004

/* MTP Filesystem Type*/
#define GenericFlat             0x0001
#define GenericHierarchical     0x0002
#define DCF                     0x0003

extern list_node_t *mtp_list_begin(mtp_obj_handle_list_t *listinfo);
extern list_node_t *mtp_list_next(mtp_obj_handle_list_t *listinfo);

typedef int32_t (*USB_MTP_STORAGEID_GET_CALLBACK)(void);
typedef int32_t (*USB_MTP_SESSION_CALLBACK)(uint32_t storage);
typedef int32_t (*USB_MTP_HANDLES_NUMGET_CALLBACK)(uint32_t storage, uint32_t parent, uint16_t format);
typedef int32_t (*USB_MTP_HANDLES_GET_CALLBACK)(uint32_t storage, uint32_t parent, uint16_t format, mtp_obj_handle_list_t *listinfo);
typedef void    (*USB_MTP_HANDLES_DESTROY_CALLBACK)(mtp_obj_handle_list_t *listinfo);
typedef int32_t (*USB_MTP_STORAGEINFO_CALLBACK)(uint32_t storage, mtp_storage_t **mstorage);
typedef int32_t (*USB_MTP_OBJINFO_RECEIVE_CALLBACK)(mtp_obj_info_t *objinfo);
typedef int32_t (*USB_MTP_OBJINFO_READ_CALLBACK)(uint32_t storage, uint32_t handle, mtp_obj_info_t *objinfo);
typedef int32_t (*USB_MTP_XFER_CALLBACK)(uint32_t storage, uint32_t handle, uint32_t offset, char *buff, uint32_t length);
typedef int32_t (*USB_MTP_OBJ_OPERATION_CALLBACK)(uint32_t storage, uint32_t handle);
typedef int32_t (*USB_MTP_OBJ_RENAME_CALLBACK)(uint32_t storage, uint32_t handle, char *name);
typedef int32_t (*USB_MTP_BIND_CALLBACK)(char *mount_name, uint32_t max_file_size);
typedef int32_t (*USB_MTP_XFER_CANCEL_CALLBACK)(uint32_t storage, uint32_t handle);

struct USB_MTP_CALLBACKS {
    USB_MTP_BIND_CALLBACK bind;

    USB_MTP_SESSION_CALLBACK open_session;
    USB_MTP_SESSION_CALLBACK close_session;

    USB_MTP_HANDLES_NUMGET_CALLBACK get_handlesnum;
    USB_MTP_HANDLES_GET_CALLBACK get_handles;
    USB_MTP_HANDLES_DESTROY_CALLBACK destroy_handles;

    USB_MTP_STORAGEINFO_CALLBACK get_storageinfo;
    USB_MTP_OBJINFO_RECEIVE_CALLBACK receive_objinfo;
    USB_MTP_OBJINFO_READ_CALLBACK read_objinfo;
    USB_MTP_XFER_CALLBACK read_obj;
    USB_MTP_XFER_CANCEL_CALLBACK read_cancel;
    USB_MTP_XFER_CANCEL_CALLBACK write_cancel;

    USB_MTP_XFER_CALLBACK write_obj;
    USB_MTP_OBJ_OPERATION_CALLBACK write_end;

    USB_MTP_OBJ_OPERATION_CALLBACK removeobj;
    USB_MTP_OBJ_RENAME_CALLBACK rename_obj;
};

int usb_mtp_callback_register(const struct USB_MTP_CALLBACKS *c);
int usb_mtp_open(enum USB_MTP_API_MODE m);
int usb_mtp_work_start(void);
int usb_mtp_fs_bind(char *fs_mount_name, uint32_t max_file_size);

#ifdef __cplusplus
}
#endif

#endif
