#ifndef __RTSPMSG_H__
#define __RTSPMSG_H__
#include"globaltype.h"
#include"sock/commsock.h"
#include"encoding/prototype.h"


static const Int32 MIN_RTSP_TRANSPORT_PORT = 33330;
static const Int32 MAX_RTSP_TRANSPORT_PORT = 40000;
static const Int32 MAX_RTSP_BUFFER_SIZE = 0x10000;
static const Int32 MAX_HTML_HEADER_SIZE = 0x400;
static const Int32 MAX_HEADER_FIELDS_NUM = 16;
static const Int32 DEF_TMP_BUFFER_SIZE = 256;
static const Int32 DEF_RTSP_CR_LF_SIZE = 2;
static const Char BYTE_RTSP_CR = '\r';
static const Char BYTE_RTSP_LF = '\n';
static const Char BYTE_RTSP_NULL = '\0'; 
static const Char DEF_PATH_SEPARATOR = '/';
static const Char DEF_QUERY_SEPARATOR = '?';

static const Char DEF_PROTOCOL_UDP[] = "UDP";
static const Char DEF_PROTOCOL_TCP[] = "TCP";
static const Char DEF_RTSP_EMPTY_TXT[] = "";
static const Char DEF_URL_SCHEMA_MARK[] = "://";
static const Int32 DEF_URL_SCHEMA_MARK_SIZE = sizeof(DEF_URL_SCHEMA_MARK)-1;
static const Char DEF_RTSP_SCHEMA_MARK[] = "rtsp://";
static const Int32 DEF_RTSP_SCHEMA_MARK_SIZE = sizeof(DEF_RTSP_SCHEMA_MARK)-1;
static const Char DEF_RTSP_RESP_PREFIX[] = "RTSP/1.";
static const Int32 DEF_RTSP_RESP_PREFIX_SIZE = sizeof(DEF_RTSP_RESP_PREFIX)-1;
static const Char PREFIX_CONTENT_LENGTH[] = "Content-Length: ";
static const Int32 PREFIX_CONTENT_LENGTH_LEN = sizeof(PREFIX_CONTENT_LENGTH)-1;

static const Char DEF_RTSP_FIELD_PUBLIC_VAL[] =
    "OPTIONS, DESCRIBE, ANNOUNCE, SETUP, TEARDOWN, PLAY, RECORD";
static const Char DEF_RTSP_FIELD_CONTENT_TYPE_VAL[] = "application/sdp";

enum EnumRtspErrCode {
    ENUM_RTSP_ERROR = -1,
    ENUM_RTSP_ERR_OK = 0,

    ENUM_RTSP_ERR_BEGIN = 1000,
    ENUM_RTSP_ERR_MSG_TYPE_INVAL,
    ENUM_RTSP_ERR_UNKNOWN_METHOD,
    ENUM_RTSP_ERR_METHOD_INVAL,
    ENUM_RTSP_ERR_PATH_INVAL,
    ENUM_RTSP_ERR_FIELD_INVAL,
    ENUM_RTSP_ERR_BODY_LENGTH_INVAL,

    ENUM_RTSP_ERR_HEADER_EXCEED_SIZE,
    ENUM_RTSP_ERR_HEADER_EXCEED_LINE,
    ENUM_RTSP_ERR_HEADER_CHAR_INVAL,
    ENUM_RTSP_ERR_CONTENT_LENGTH_INVAL,

    ENUM_RTSP_ERR_END
};

enum EnumRtspMsgType {
    ENUM_MSG_RTSP_HTML = 1000,

    ENUM_MSG_RTP_DATA_ORIGIN,
    ENUM_MSG_RTP_UDP_SEND,
};

enum EnumRtspDecState {
    ENUM_RTSP_DEC_INIT = 0,

    ENUM_READ_HEADER,
    ENUM_READ_BODY,

    ENUM_RTSP_DEC_END
}; 


/* the length of span is (high - low), same as [low, high) */
struct Span {
    Int32 m_low:16,
        m_high:16;
};

struct CacheHdr;
struct RtspNode;
struct RtpNode;
struct NodeBase;
struct Session;

struct RtspInput {
    Char* m_txt;   // used as temporary buffer
    Span m_fields[MAX_HEADER_FIELDS_NUM];
    Int32 m_field_num;  // current field number
    Int32 m_txt_cap;    // capacity of buffer
    Int32 m_head_size;
    Int32 m_body_size;
    Int32 m_upto;   // current length in buffer
    Int32 m_total;  // the whole html msg length
    Int32 m_line_start;       // current position
    Int32 m_line_size;

    EnumRtspDecState m_rd_stat;
};

struct HtmlMsg {
    Char* m_txt;
    Span m_fields[MAX_HEADER_FIELDS_NUM];  // each of html fields 
    Int32 m_field_num; // html field actual size in a package 
    Int32 m_head_size;   // html head size
    Int32 m_body_size;   // html body size 
    Int32 m_txt_cap;    // cap of txt 
};

enum EnumRtspStreamType {
    ENUM_RTSP_STREAM_RTP = 0,
    ENUM_RTSP_STREAM_RTCP = 1,
    ENUM_RTSP_STREAM_END
};

struct Rtp {
    RtpNode* m_rtp_node;
    RtpNode* m_rtcp_node;
    Int32 m_port_base;
    Char m_ip[MAX_IP_SIZE];
};

struct Rtsp {
    RtspNode* m_entity;
    Session* m_sess;
    Rtp* m_rtp;
    RtspInput m_input; 
    Int32 m_fd;
    IpInfo m_my_ip;
    IpInfo m_peer_ip; 
};

struct RtspPkg {
    Int32 m_len;
    Char m_data[0];
};

#endif

