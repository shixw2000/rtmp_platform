#ifndef __HTTPDEC_H__
#define __HTTPDEC_H__
#include"sysoper.h"


static const Char DEF_RTP_STR[] = "RTP";
static const Char DEF_AVP_STR[] = "AVP";
static const Char DEF_UDP_STR[] = "UDP";
static const Char DEF_RTSP_VER_PREFIX[] = "RTSP/1.";

enum RTSPStatusCode {
RTSP_STATUS_CONTINUE             =100,
RTSP_STATUS_OK                   =200,
RTSP_STATUS_CREATED              =201,
RTSP_STATUS_LOW_ON_STORAGE_SPACE =250,
RTSP_STATUS_MULTIPLE_CHOICES     =300,
RTSP_STATUS_MOVED_PERMANENTLY    =301,
RTSP_STATUS_MOVED_TEMPORARILY    =302,
RTSP_STATUS_SEE_OTHER            =303,
RTSP_STATUS_NOT_MODIFIED         =304,
RTSP_STATUS_USE_PROXY            =305,
RTSP_STATUS_BAD_REQUEST          =400,
RTSP_STATUS_UNAUTHORIZED         =401,
RTSP_STATUS_PAYMENT_REQUIRED     =402,
RTSP_STATUS_FORBIDDEN            =403,
RTSP_STATUS_NOT_FOUND            =404,
RTSP_STATUS_METHOD               =405,
RTSP_STATUS_NOT_ACCEPTABLE       =406,
RTSP_STATUS_PROXY_AUTH_REQUIRED  =407,
RTSP_STATUS_REQ_TIME_OUT         =408,
RTSP_STATUS_GONE                 =410,
RTSP_STATUS_LENGTH_REQUIRED      =411,
RTSP_STATUS_PRECONDITION_FAILED  =412,
RTSP_STATUS_REQ_ENTITY_2LARGE    =413,
RTSP_STATUS_REQ_URI_2LARGE       =414,
RTSP_STATUS_UNSUPPORTED_MTYPE    =415,
RTSP_STATUS_PARAM_NOT_UNDERSTOOD =451,
RTSP_STATUS_CONFERENCE_NOT_FOUND =452,
RTSP_STATUS_BANDWIDTH            =453,
RTSP_STATUS_SESSION              =454,
RTSP_STATUS_STATE                =455,
RTSP_STATUS_INVALID_HEADER_FIELD =456,
RTSP_STATUS_INVALID_RANGE        =457,
RTSP_STATUS_RONLY_PARAMETER      =458,
RTSP_STATUS_AGGREGATE            =459,
RTSP_STATUS_ONLY_AGGREGATE       =460,
RTSP_STATUS_TRANSPORT            =461,
RTSP_STATUS_UNREACHABLE          =462,
RTSP_STATUS_INTERNAL             =500,
RTSP_STATUS_NOT_IMPLEMENTED      =501,
RTSP_STATUS_BAD_GATEWAY          =502,
RTSP_STATUS_SERVICE              =503,
RTSP_STATUS_GATEWAY_TIME_OUT     =504,
RTSP_STATUS_VERSION              =505,
RTSP_STATUS_UNSUPPORTED_OPTION   =551,
};

enum EnumHttpFieldType { 
    ENUM_HTTP_CONTENT_LENGTH = 0,
    ENUM_HTTP_SESSION, 
    ENUM_HTTP_CONTENT_TYPE,
    ENUM_HTTP_CONTENT_BASE, 
    ENUM_HTTP_LOCATION,
    
    ENUM_HTTP_SERVER, 
    ENUM_HTTP_SEQ, 
    ENUM_HTTP_DATE,
    ENUM_HTTP_TRANSPORT,
    ENUM_HTTP_PUBLIC,

    ENUM_HTTP_WWW_AUTH, 
    
    ENUM_HTTP_FIELD_END
};

struct RtspTransport {
    Bool m_valid;
    Int32 m_port[2];
    Int32 m_client_port[2];
    Int32 m_server_port[2];
    Int32 m_ttl;
    Bool m_is_rtp_udp;
    Bool m_is_unicast;
    Int32 m_mode_record; // default play
    Token m_dest;
    Token m_source;
};

struct RtspAuthInfo {
    Int32 m_auth_type;
    Token m_realm;
};

struct RtspCtx {
    Bool m_is_req;
    Uint32 m_seq;
    Int32 m_timeout; 
    Int32 m_status_code;
    
    Token m_ver; 
    Token m_reason;
    Token m_url;
    Token m_method; 
    Token m_public;
    Token m_control_uri; 
    Token m_content_type;
    Token m_body; 
    
    RtspTransport m_transport;
    RtspAuthInfo m_auth_info;
    Char m_sid[MAX_SESS_ID_LEN];
};

struct HtmlMsg;

class HttpTool {
public: 
    static Void splitUri(const Token* uri, Token* schema, Token* auth,
        Token* ip, Int32* port, Token* path);

    static Int32 parseCmdLine(const Token* orig, RtspCtx* ctx);

    static Int32 parseHdrLine(const Token* orig, RtspCtx* ctx);

    static Int32 parseTransport(const Token* orig, RtspTransport* param);

    static Int32 parseTransSpec(const Token* orig, RtspTransport* param);

    static Int32 parseTransParam(const Token* orig, RtspTransport* param);

    static Int32 parseRange(const Token* orig, Int32 fd[]);

    static Int32 parseRtspCtx(const HtmlMsg* msg, RtspCtx* ctx);

    static Int32 parseAuthInfo(const Token* orig, RtspAuthInfo* param);

    static Int32 parseSess(const Token* orig, RtspCtx* ctx);

    static Int32 parseContentBase(const Token* orig, RtspCtx* ctx);
    
    static Bool isRtspVer(const Token* t);

    static Int32 genTransport(const RtspTransport* trans, Token* buffer); 

    static Bool genRtspMsg(const RtspCtx* ctx, Token* buffer);

    static Bool toStr(const RtspCtx* ctx, Token* t);

    static const Char* getHttpStatusStr(Int32 code); 

    static Int32 addDateField(Token* buffer);
    static Int32 addField(Token* buffer, const Char* key, const Char* val); 
    static Int32 addFieldToken(Token* buffer, const Char* key, const Token* val); 
    static Int32 addFieldUint(Token* buffer, const Char* key, Uint32 val); 
    static Int32 addFieldInt(Token* buffer, const Char* key, Int32 val); 
    static Int32 addBody(Token* buffer, const Token* body);

    static Void reset(RtspCtx* ctx);
};


#endif

