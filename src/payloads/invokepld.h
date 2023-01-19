#ifndef __INVOKEPLD_H__
#define __INVOKEPLD_H__
#include"globaltype.h"
#include"datatype.h"
#include"payload.h"


struct RtmpNode;
struct HeaderRtmp;
struct Rtmp;
struct RtmpPkg;
struct RtmpUint;
struct Cache;
struct RtmpStream;
class HandShake;
class AmfPayload;
class StreamCenter;
class RtmpProto;

class RtmpHandler {
    RtmpHandler();
    ~RtmpHandler();

    Int32 init();
    Void finish();

public:
    static RtmpHandler* creat();
    static Void free(RtmpHandler* hd);

    /* this function will allocat memory for key, and the user must release it later */
    static Bool genStreamKey(const Chunk* app,
        const Chunk* name, Chunk* key);

    static Bool isAvcSid(Rtmp* rtmp, Uint32 sid); 
    
    Int32 dealRtmp(Rtmp* rtmp, CacheHdr* hdr);

    Int32 dealCtrlMsg(Rtmp* rtmp, Cache* cache);
    
    Int32 sendBytesReport(Rtmp* rtmp, Uint32 seqBytes);
    
    Void closeRtmp(Rtmp* rtmp); 

    Int32 dealHandshakeC01(Rtmp* rtmp, CacheHdr* hdr);
    Int32 dealHandshakeC2(Rtmp* rtmp, CacheHdr* hdr);

    Int32 dealInvoke(Rtmp* rtmp, CacheHdr* hdr);

    Int32 dealMetaData(Rtmp* rtmp, CacheHdr* hdr);

    Int32 dealFlvData(Rtmp* rtmp, CacheHdr* hdr); 

private:
    Int32 dealConn(Rtmp* rtmp, AMFObject* obj);
    
    Int32 dealPlay(Rtmp* rtmp, Uint32 sid, Uint32 pkg_sid, AMFObject* obj);
    Int32 dealPause(Rtmp* rtmp, Uint32 sid, Uint32 pkg_sid, AMFObject* obj);
    Int32 dealPublish(Rtmp* rtmp, Uint32 sid, Uint32 pkg_sid, AMFObject* obj);
    
    Int32 dealFCPublish(Rtmp* rtmp, Uint32 sid, Uint32 pkg_sid, AMFObject* obj);
    Int32 dealFCUnpublish(Rtmp* rtmp, Uint32 sid, Uint32 pkg_sid, AMFObject* obj);
    
    Int32 dealDelStream(Rtmp* rtmp, Uint32 sid, Uint32 pkg_sid, AMFObject* obj);
    Int32 dealReleaseStream(Rtmp* rtmp, Uint32 sid, Uint32 pkg_sid, AMFObject* obj);
    
    Int32 dealCreatStream(Rtmp* rtmp, AMFObject* obj); 

    Int32 publish(RtmpUint* unit, Cache* cache);
    Int32 play(RtmpUint* unit, Cache* cache); 

    RtmpStream* findStream(Rtmp* rtmp, const Chunk* name);

private:
    Uint32 nextSid(Rtmp* rtmp);
    
private:
    Uint32 m_user_id;
    HandShake* m_handshake;
    AmfPayload* m_amf;
    StreamCenter* m_stream_center;
    RtmpProto* m_proto_center;
};


#endif

