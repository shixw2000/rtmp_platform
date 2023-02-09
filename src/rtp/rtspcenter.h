#ifndef __RTSPCENTER_H__
#define __RTSPCENTER_H__
#include<string>
#include<map>
#include<vector>
#include"rtspmsg.h"


struct CacheHdr;
struct Cache;
class RtspDec;

typedef std::string typeString;
typedef std::map<typeString, typeString> typeFields;
typedef typeFields::iterator typeItr;
typedef typeFields::const_iterator typeItrConst;
typedef std::size_t typeSize;
typedef std::vector<typeString> typeVec;

static const typeSize DEF_STR_END = typeString::npos;
static const typeString DEF_RTSP_EMPTY_STR = "";
static const typeString DEF_BLANK_TOKEN = " \t";
static const typeString DEF_QUATION_TOKEN = "=";

class StdTool { 
public:
    static Void split(const typeString& txt, const Char* delim, typeVec& vecs);

    static Void strip(typeString& txt);

    static const typeString& getVal(const typeFields& fields, 
        const typeString& key);

    static Void setVal(typeFields& fields, 
        const typeString& key, const typeString& val);
};

struct HtmlCtx {
    RTSP_HTML_TYPE m_html_type;
    const Char* m_body;
    Int32 m_body_size;
    typeString m_ver;
    typeString m_method;
    typeString m_prefix;
    typeString m_path;
    typeString m_query;
    typeString m_rsp_code;
    typeString m_rsp_detail;
        
    typeFields m_fields;
};

class RtspHandler {
public:
    typedef Int32 (RtspHandler::*PFunc)(Rtsp* rtsp, 
        HtmlCtx* oRsp, const HtmlCtx* ctx);

    struct Callback {
        const Char* m_name;
        PFunc m_func;
    };

private:
    RtspHandler();

    Int32 init();
    Void finish();

public:
    static RtspHandler* creat();
    static Void freeHd(RtspHandler* hd);

    static Void dspHtml(const Char* promt, CacheHdr* hdr);
    
    static CacheHdr* creatRtspMsg();
    static Void freeRtspMsg(CacheHdr* hdr);

    static Cache* creatRtspPkg(Int32 len);

    static Bool isVer(const char* txt);
    static Bool hasError(const char* txt);

    Void resetCtx(HtmlCtx* ctx);
    
    Int32 sendCtx(Rtsp* rtsp, HtmlCtx* ctx);
    
    Cache* genRtspHtml(HtmlCtx* ctx);

    Void dupKey(HtmlCtx* dst, const HtmlCtx* src, 
        const Char* key, const Char* defVal = DEF_RTSP_EMPTY_TXT);

    const typeString& getField(const HtmlCtx* ctx, const Char* key);
    
    Void setField(HtmlCtx* ctx, const Char* key, const Char* val);
    Void setDateField(HtmlCtx* ctx);
    Void setSessionField(HtmlCtx* dst, const HtmlCtx* req);
    Void setContentBase(Rtsp* rtsp, HtmlCtx* ctx, const HtmlCtx* req);
    Void setTransport(Int32 srvPort, const typeVec& vecProto, 
        const typeVec& vecPort, HtmlCtx* ctx);
    
    Int32 parseRtsp(Rtsp* rtsp, Byte* data, Int32 len);

    Int32 dealRtsp(Rtsp* rtsp, CacheHdr* hdr);

    Void dspHtmlCtx(Int32 ret, const HtmlCtx* ctx);

    Int32 dealOption(Rtsp* rtsp, HtmlCtx* oRsp, const HtmlCtx* ctx);
    Int32 dealDescribe(Rtsp* rtsp, HtmlCtx* oRsp, const HtmlCtx* ctx);
    Int32 dealAnnounce(Rtsp* rtsp, HtmlCtx* oRsp, const HtmlCtx* ctx);
    Int32 dealSetup(Rtsp* rtsp, HtmlCtx* oRsp, const HtmlCtx* ctx);
    Int32 dealPlay(Rtsp* rtsp, HtmlCtx* oRsp, const HtmlCtx* ctx);
    Int32 dealRecord(Rtsp* rtsp, HtmlCtx* oRsp, const HtmlCtx* ctx);
    Int32 dealTeardown(Rtsp* rtsp, HtmlCtx* oRsp, const HtmlCtx* ctx);

    Int32 parseTransport(const HtmlCtx* ctx, typeVec& vecProto, 
        typeVec& vecPort, typeFields& fields);

    Int32 prepareRtp(Rtsp* rtsp, Int32 base_id, Int32* srvPort);

    Void setPortPairMap(Int32 port_base, Bool bUsed);
    
private:
    Int32 dealRsp(Rtsp* rtsp, HtmlCtx* ctx);
    Int32 dealReq(Rtsp* rtsp, const HtmlCtx* ctx); 
    
    Int32 parseCtx(HtmlMsg* msg, HtmlCtx* ctx);

    Int32 parseMethod(HtmlCtx* ctx, Char* data, Int32 len);
    Int32 parseField(HtmlCtx* ctx, Char* data, Int32 len);
    Int32 parsePath(HtmlCtx* ctx, Char* txt); 

    const AVal* getDesc();

    Int32 getNextPortPair();

private: 
    static const Callback* const m_cmds;
    RtspDec* m_decoder; 
    Int32 m_port_base;
    Bool m_port_used_map[MAX_RTSP_TRANSPORT_SIZE];
    Char m_buffer[MAX_RTSP_BUFFER_SIZE];
}; 


#endif

