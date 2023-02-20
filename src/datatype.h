#ifndef __DATATYPE_H__
#define __DATATYPE_H__
#include"globaltype.h"
#include"listnode.h"


struct CacheHdr;
struct Cache;
struct RtmpUint;
struct RtmpStream;
struct Rtmp;
struct RtmpNode;

enum EnumSockRet {
    ENUM_SOCK_MARK_FINISH = 0,

    ENUM_SOCK_MARK_PARTIAL,
    ENUM_SOCK_MARK_BLOCK, 

    ENUM_SOCK_MARK_ERR
};

enum EnumFdStatus {
    ENUM_FD_NORMAL,

    ENUM_FD_CLOSING,
    ENUM_FD_STOP_RCV,
    ENUM_FD_STOP_DEAL,
    ENUM_FD_STOP_SND,

    ENUM_FD_CLOSED
};

enum EnumNodeType {
    ENUM_NODE_BEG,

    ENUM_NODE_TIMER,
    ENUM_NODE_CTRL, 
    ENUM_NODE_LISTENER,
    ENUM_NODE_SOCKET,

    ENUM_NODE_RTMP,
    ENUM_NODE_RTSP,
    ENUM_NODE_RTP,
    ENUM_NODE_RTCP,

    ENUM_NODE_END
};

enum EnumErrCode {
    ENUM_ERR_FAIL = -1,
    ENUM_ERR_OK = 0,

    ENUM_ERR_SOCK_PEER_CLOSE,
    ENUM_ERR_SOCK_CHK_IO,
    ENUM_ERR_SOCK_DATA_RD,
    ENUM_ERR_SOCK_DATA_WR,
    ENUM_ERR_SOCK_DATA_DEAL,
    ENUM_ERR_SOCK_PARENT_STOP,

    ENUM_ERR_ALLOC_CACHE,
};

enum EnumCtrlType {
    CTRL_TYPE_RCV = 0,
    CTRL_TYPE_SND,
    CTRL_TYPE_DEAL,
    CTRL_TYPE_CMD,

    CTRL_TYPE_END
};

enum EventType {
    EVENT_TYPE_RD = 0x1,
    EVENT_TYPE_WR = 0x2,
    EVENT_TYPE_ALL = EVENT_TYPE_WR | EVENT_TYPE_RD,
};

enum EnumTimerType {
    ENUM_TIMER_NULL,
        
    ENUM_TIMER_CMD_SECOND,

    ENUM_TIMER_END
};


static const Int32 DEF_TIMER_TICK_MS = 100; // 100ms
static const Uint32 DEF_SECOND_TICK_CNT = 10;

static const Int32 MAX_PIN_PASSWD_SIZE = 64; 

struct Config { 
    Int32 m_log_level;
    Int32 m_log_stdin;
    Int32 m_chunk_size;
    Int32 m_window_size;
    Int32 m_prnt_flv;
    Int32 m_port;
    Char m_ip[32];
    Char m_passwd[MAX_PIN_PASSWD_SIZE];
};

struct TcpParam {
    Int32 m_port;
    Int32 m_addr_len;
    Char m_ip[DEF_IP_SIZE];
    Char m_addr[DEF_ADDR_SIZE];
};

struct Chunk {
    Byte* m_data;
    Int32 m_size;
};

struct NodeBase {
    Int32 (*getFd)(struct NodeBase* _this);
    
    EnumSockRet (*readNode)(struct NodeBase* _this);
    EnumSockRet (*writeNode)(struct NodeBase* _this);
    Int32 (*dealMsg)(struct NodeBase* _this, CacheHdr* hdr); 
    Void (*dealCmd)(struct NodeBase* _this, CacheHdr* hdr); 
    
    Void (*onClose)(struct NodeBase* _this);
    Void (*onChildExit)(struct NodeBase* _this, struct NodeBase* child);
    

    Void (*destroy)(struct NodeBase* _this);
  
    list_node m_rcv_node;
    list_node m_snd_node;
    list_node m_cmd_node;
    
    list_head m_snd_que_tmp;
    list_head m_deal_que_tmp;
    list_head m_cmd_que_tmp;
    list_head m_snd_que;
    list_head m_deal_que;
    list_head m_cmd_que;

    struct Task m_rcv_task;
    struct Task m_snd_task;
    struct Task m_deal_task;
    struct Task m_cmd_task;

    Int32 m_fd_status;
    Int32 m_node_type;
    Int32 m_reason;
    Bool m_can_rd;
    Bool m_can_wr;
    Bool m_stop_deal;
};


enum EnumRtmpRdStat {
    ENUM_RTMP_RD_INIT,

    ENUM_RTMP_RD_C0,
    ENUM_RTMP_RD_C1,
    ENUM_RTMP_RD_C2,
    
    ENUM_RTMP_RD_BASIC,
    ENUM_RTMP_RD_HEADER, 
    ENUM_RTMP_RD_EXT_TIME,
    ENUM_RTMP_RD_CHUNK,

    ENUM_RTMP_RD_END
}; 

enum EnumRtmpStatus {
    ENUM_RTMP_STATUS_INIT = 0,

    ENUM_RTMP_STATUS_HANDSHAKE,
    ENUM_RTMP_STATUS_CONNECTED, 
    ENUM_RTMP_STATUS_CLOSED,
    
    ENUM_RTMP_STATUS_ERROR
};

enum RTMPChannel {
    RTMP_CTRL_CID = 2,   ///< channel for network-related messages (bandwidth report, ping, etc)
    RTMP_INVOKE_CID = 3,     ///< channel for sending server control messages
    
    RTMP_SOURCE_CHANNEL  = 4,   ///< channel for a/v invokes
    RTMP_OTHER_CHANNEL = 5,

    RTMP_MAX_CHNN_ID = 64
};

enum RtmpUsrCtrlEvent {
    ENUM_USR_CTRL_STREAM_BEGIN = 0,
    ENUM_USR_CTRL_STREAM_END = 1,
};

static const Uint32 RTMP_INVALID_SID = 0;
static const Uint32 RTMP_MAX_SESS_CNT = (RTMP_MAX_CHNN_ID 
    - RTMP_OTHER_CHANNEL) >> 1;

static const Int32 RTMP_MAX_HEADER_SIZE = 16;
static const Int32 RTMP_MIN_CHUNK_SIZE = 128;
static const Int32 RTMP_MAX_CHUNK_SIZE = 0x4000;
static const Int32 RTMP_EXT_TIMESTAMP_SIZE = 4;
static const Uint32 RTMP_TIMESTAMP_EXT_MARK = 0XFFFFFF;

static const Uint32 RTMP_LARGE_FMT = 0;
static const Uint32 RTMP_MEDIUM_FMT = 1;
static const Uint32 RTMP_SMALL_FMT = 2;
static const Uint32 RTMP_MINIMUM_FMT = 3;
static const Int32 RTMP_HEADER_SIZE_ARR[] = { 11, 7, 3, 0 };

static const Int32 AMF_MAX_STRING_SIZE = 0x10000;
static const Uint32 AMF_END_OF_OBJ = 0x9;
static const Uint32 RTMP_DEF_WIND_SIZE = 2500000;

static const Int32 RTMP_MSG_TYPE_CHUNK_SIZE = 0x1;
static const Int32 RTMP_MSG_TYPE_ABORT_MSG = 0x2;
static const Int32 RTMP_MSG_TYPE_BYTES_READ_REPORT = 0x3;
static const Int32 RTMP_MSG_TYPE_USR_CTRL_MSG = 0x4;
static const Int32 RTMP_MSG_TYPE_SERVER_BW = 0x5;
static const Int32 RTMP_MSG_TYPE_CLIENT_BW = 0x6;

static const Int32 RTMP_MSG_TYPE_AUDIO = 0x8;
static const Int32 RTMP_MSG_TYPE_VIDEO = 0x9;
static const Int32 RTMP_MSG_TYPE_META_INFO = 0x12;

static const Int32 RTMP_MSG_TYPE_INVOKE = 0x14;

static const Int32 DEF_DIGEST_LENGTH = 32;
static const Int32 RTMP_SIG_C0_SIZE = 1;
static const Int32 RTMP_SIG_SIZE = 1536;
static const Byte  RTMP_DEF_HANDSHAKE_VER = 0x3;

enum EnumBWType {
    ENUM_BW_HARD = 0,
    ENUM_BW_SOFT,
    ENUM_BW_DYNAMIC
};

typedef struct HeaderRtmp {
    Uint32 m_epoch;     // the absolute time
    Uint32 m_timestamp;
    Uint32 m_sid;
    Uint32 m_pld_size;
    Uint32 m_fmt:2,
        m_cid:6,
        m_rtmp_type:8; 
} HeaderRtmp;

typedef struct RtmpHandshake {
    Uint32 m_peerTs;
    Uint32 m_peerMark;
    Byte m_peerDigest[DEF_DIGEST_LENGTH];
    Byte m_localDigest[DEF_DIGEST_LENGTH];
} RtmpHandshake;

typedef struct ChnInput {
    Cache* m_pkg;
    Int32 m_rd_bytes;
    Uint32 m_epoch;     // the absolute time
    Uint32 m_timestamp;
    Uint32 m_sid;
    Uint32 m_pld_size;
    Uint32 m_fmt:2,
        m_cid:6,
        m_rtmp_type:8;
        
} ChnInput;

typedef struct ChnOutput {
    Uint32 m_epoch;     // the absolute time
    Uint32 m_timestamp; 
    Uint32 m_sid;
    Uint32 m_pld_size;
    Uint32 m_cid:6,
        m_rtmp_type:8;
} ChnOutput;

enum EnumOperAuth {
    ENUM_OPER_NULL = 0,
    ENUM_OPER_PLAYER,
    ENUM_OPER_PUBLISHER,

    ENUM_OPER_END
};

struct RtmpUint {
    Rtmp* m_parent;
    RtmpStream* m_ctx;
    Chunk m_stream_name;
    Chunk m_stream_type;
    Chunk m_key; 
    
    Uint32 m_sid;
    Uint32 m_delta_out_ts;
    Uint32 m_max_out_epoch;
    Int32 m_oper_auth;
    Bool m_blocked;
    Bool m_is_pause; 
};

typedef struct RtmpChn { 
    Cache* m_cache; // used by handshake
    ChnInput* m_input;
    Byte* m_data;
    Int32 m_upto;
    Int32 m_size;
    EnumRtmpRdStat m_rd_stat;
    RtmpHandshake m_handshake; 
    Byte m_buff[RTMP_MAX_HEADER_SIZE];
} RtmpChn;

typedef struct Rtmp {
    list_head m_rpc_calls; 
    RtmpNode* m_entity;     // level of socket
    ChnInput* m_chnnIn[RTMP_MAX_CHNN_ID];   // chn input
    ChnOutput* m_chnnOut[RTMP_MAX_CHNN_ID]; // chn output, level of connection
    RtmpUint* m_unit[RTMP_MAX_SESS_CNT];   // level of stream
    RtmpChn m_chn;
    Chunk m_app;
    Chunk m_tcUrl;

    Uint32 m_user_id;
    Uint32 m_encoding;
    Int32 m_fd;
    Int32 m_chunkSizeIn;
    Int32 m_chunkSizeOut;
    Int32 m_bwSrv;
    Int32 m_bwCli;
    Uint32 m_rcv_total_bytes;
    Uint32 m_rcv_ack_bytes;
    Uint32 m_srv_window_size;
    Uint32 m_snd_total_bytes;
    Uint32 m_snd_ack_bytes;
    Uint32 m_cli_window_size;
    
    EnumRtmpStatus m_status;    // status of rtmp    
} Rtmp;

/*******begin: kinds of caches*********/
typedef struct RtmpPkg {
    Uint32 m_epoch; // absolute time
    Uint32 m_timestamp; // delta time
    Uint32 m_sid;
    Uint32 m_rtmp_type:8,   // rtmp type
        m_msg_type:8,       // customer type 
        m_has_striped:1;    // used by meta_data
    Int32 m_size;   // payload max size       
    Int32 m_skip;   // payload skip len
    Byte m_payload[0];
} RtmpPkg;

typedef struct CommPkg {
    Int32 m_size;
    Byte m_data[0];
} CommPkg;

/*******end of caches*********/

extern Config g_conf;

#endif

