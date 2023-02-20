#ifndef __RTSPCENTER_H__
#define __RTSPCENTER_H__
#include"rtspmsg.h"
#include"stdtype.h"


struct CacheHdr;
struct Cache;
struct RtspCtx;
class RtspDec;
class SessCenter;
class ResCenter;

class StdTool { 
public:
    static Void split(const typeString& txt, const Char* delim, typeVec& vecs);

    static Void strip(typeString& txt);

    static const typeString& getVal(const typeFields& fields, 
        const typeString& key);

    static Void setVal(typeFields& fields, 
        const typeString& key, const typeString& val);
};

class RtspHandler {
public:
    typedef Int32 (RtspHandler::*PFunc)(Rtsp* rtsp, 
        const RtspCtx* req, RtspCtx* rsp);

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

    static Bool isVer(const char* txt);
    static Bool hasError(const char* txt);

    Void dspRtspCtx(const RtspCtx* ctx);

    Int32 sendRtspMsg(Rtsp* rtsp, const RtspCtx* ctx);
    Int32 sendErrorReply(Rtsp* rtsp, Int32 code); 
    
    Int32 parseRtsp(Rtsp* rtsp, Byte* data, Int32 len);

    Int32 dealRtsp(Rtsp* rtsp, CacheHdr* hdr);

    Bool hasSession(const RtspCtx* ctx);
    Bool chkTransport(const RtspCtx* ctx);

    Int32 dealOption(Rtsp* rtsp, const RtspCtx* req, RtspCtx* rsp);
    Int32 dealDescribe(Rtsp* rtsp, const RtspCtx* req, RtspCtx* rsp);
    Int32 dealAnnounce(Rtsp* rtsp, const RtspCtx* req, RtspCtx* rsp);
    Int32 dealSetup(Rtsp* rtsp, const RtspCtx* req, RtspCtx* rsp);
    Int32 dealPlay(Rtsp* rtsp, const RtspCtx* req, RtspCtx* rsp);
    Int32 dealRecord(Rtsp* rtsp, const RtspCtx* req, RtspCtx* rsp);
    Int32 dealTeardown(Rtsp* rtsp, const RtspCtx* req, RtspCtx* rsp);

    Int32 prepareRtp(Rtsp* rtsp, Int32* srvPort);

    Int32 parseSDP(const RtspCtx* req);
    
private:
    Int32 dealRsp(Rtsp* rtsp, const RtspCtx* ctx);
    Int32 dealReq(Rtsp* rtsp, const RtspCtx* ctx); 
  
    const Token* getDesc();

    Int32 getNextPortPair();

private: 
    static const Callback* const m_cmds;
    ResCenter* m_res_center;
    SessCenter* m_sess_center;
    RtspDec* m_decoder; 
    Uint32 m_seq;
    Char m_buffer[MAX_RTSP_BUFFER_SIZE];
}; 


#endif

