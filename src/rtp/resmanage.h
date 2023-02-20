#ifndef __RESMANAGE_H__
#define __RESMANAGE_H__
#include"stdtype.h"
#include"sysoper.h"


enum EnumSDPCateNum {
    ENUM_SDP_ALL = 0,
        
    ENUM_SDP_CATE_VER,
    ENUM_SDP_CATE_CREATOR,
    ENUM_SDP_CATE_SESS_NAME,
    ENUM_SDP_CATE_SESS_DETAIL,
    ENUM_SDP_CATE_URI,
    ENUM_SDP_CATE_EMAIL,
    ENUM_SDP_CATE_PHONE,
    ENUM_SDP_CATE_CONNECTION,
    ENUM_SDP_CATE_BANDWITH,

    ENUM_SDP_END
};

enum EnumMediaType { 
    ENUM_MEDIA_TYPE_VIDEO,
    ENUM_MEDIA_TYPE_AUDIO,
    ENUM_MEDIA_TYPE_DATA, 

    ENUM_MEDIA_TYPE_END,
};

static const Int32 MAX_PRESENT_MADIA_CNT = 8;
static const Int32 MAX_FMTP_LEN = 256;
static const Int32 MAX_SDP_LEN = 1024;
static const Int32 MAX_CONN_LEN = 64;

struct NodeBase;

struct MediaInfo {
    Int32 m_index;
    Int32 m_codec_type;
    Int32 m_sample_rate;
    Int32 m_channel_num;
    Int32 m_sdp_pt;
    Char m_enc_name[MAX_NAME_LEN];
    Char m_fmtp[MAX_FMTP_LEN];
    Char m_uri[MAX_URL_LEN];
};

/* this is the session level part of presentation */
struct Presentation { 
    Int32 m_media_size;
    MediaInfo m_media[MAX_PRESENT_MADIA_CNT];
    Char m_title[MAX_NAME_LEN];
    Char m_detail[MAX_NAME_LEN];
    Int32 m_ctx_size;
    Char m_ctx[MAX_SDP_LEN];
    Char m_uri[MAX_URL_LEN];
};

struct RtspStream {
    const Char* m_path;
    MediaInfo* m_media;

    /* recorders */
    NodeBase* m_publisher[100];

    /* players */
    NodeBase* m_player[100];
    Int32 m_publisher_cnt;
    Int32 m_player_cnt;
};

struct Aggregate {
    const Char* m_path;
    Presentation* m_res;
    Int32 m_stream_cnt;
    RtspStream m_stream[MAX_PRESENT_MADIA_CNT];
};

class ResTool {
public:
    static Bool parseSDP(const Token* t, Presentation* sdp);
    static Bool encCtx(Presentation* sdp);
    static Bool encMedia(MediaInfo* media, Token* orig);
    static Bool parseConn(const Token* orig, Presentation* sdp);
    static Bool parseMedia(const Token* orig, Presentation* sdp);
    static Bool parseAttr(const Token* orig, Presentation* sdp);
    static Bool parseControl(const Token* orig, Presentation* sdp);
    static Bool parseRtpmap(const Token* orig, Presentation* sdp);
    static Bool parseFmtp(const Token* orig, Presentation* sdp);

    static Presentation* creatPresent();
    static Void freePresent(Presentation* present);

    static Void resetPresent(Presentation* present);
    static Void resetMedia(MediaInfo* media);

    static MediaInfo* findMedia(Presentation* res, const Char* path);
    static RtspStream* findStream(Aggregate* aggr, const Char* path);

    static Aggregate* genAggr(Presentation* present);
};

class ResCenter {
public: 
    Presentation* findRes(const Char* path);
    Void addRes(Presentation* aggr);
    Void delRes(const Char* path);
    
    Aggregate* findAggr(const Char* path);
    Void delAggr(const Char* path);
    Void addGroup(Aggregate* aggr);
    Void delGroup(Aggregate* aggr);
    
private:
    typeMapRes m_mapRes;
    typeMapAggr m_mapAggr;
};

#endif

