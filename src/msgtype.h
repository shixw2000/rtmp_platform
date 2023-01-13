#ifndef __MSGTYPE_H__
#define __MSGTYPE_H__
#include"sysoper.h"


#pragma pack(push, 1)



#pragma pack(pop)


/******* begin: used by self system ********/
struct MsgSend {
    Byte* m_body;
    Int32 m_hdr_upto;
    Int32 m_hdr_size;
    Int32 m_body_upto;
    Int32 m_body_size;
    Byte m_head[0];
};

struct MsgComm {
    Int32 m_size;
    Byte  m_data[0];
};

/******* end ********/

enum EnumMsgType {
    ENUM_MSG_TYPE_CLOSE,
    ENUM_MSG_TYPE_STOP,
    ENUM_MSG_ADD_FD,
    ENUM_MSG_DEL_FD,
    ENUM_MSG_REGIST_TASK,
    ENUM_MSG_TIMER_TICK,

    ENUM_MSG_TYPE_SYSTEM_MAX = 100, // separate msgs

    ENUM_MSG_RTMP_HANDSHAKE_C0C1,
    ENUM_MSG_RTMP_HANDSHAKE_C2,
    ENUM_MSG_RTMP_TYPE_CTRL,
    ENUM_MSG_RTMP_TYPE_AUDIO,
    ENUM_MSG_RTMP_TYPE_VIDEO,
    ENUM_MSG_RTMP_TYPE_META_INFO,
    ENUM_MSG_RTMP_TYPE_INVOKE,
    
    ENUM_MSG_RTMP_TYPE_APP,
    
    ENUM_MSG_CUSTOMER_NOTIFY_END_STREAM,
    

    ENUM_MSG_TYPE_COMMON,
    ENUM_MSG_TYPE_SEND,

    ENUM_MSG_TYPE_END
};

#endif
