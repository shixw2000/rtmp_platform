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

class RtmpHandler {
    RtmpHandler();
    ~RtmpHandler();

    Int32 init();
    Void finish();

public:
    static RtmpHandler* creat();
    static Void free(RtmpHandler* hd);

    static Bool genStreamKey(const Chunk* app,
        const Chunk* name, Chunk* key);

    static Bool chkFlvData(Uint32 msg_type);

    Int32 dealCtrlMsg(RtmpNode* node, Rtmp* rtmp, Cache* cache);

    Void cacheAvc(RtmpStream* stream, Int32 msg_type, Cache* cache);

    Void closeRtmp(RtmpNode* node, Rtmp* rtmp);
    
    Int32 sendRtmpPkg(RtmpNode* node, Uint32 stream_id, Cache* cache);
    
    Int32 sendAmfObj(const Char* promt, RtmpNode* node, Uint32 epoch, 
        Uint32 stream_id, Uint32 rtmp_type, AMFObject* obj);
    
    Int32 sendStatus(RtmpNode* node, const Chunk* status, 
        const Chunk* detail);
    
    Int32 sendCall(RtmpNode* node, const Chunk* call,
        const double* txn, AMFObject* info, 
        AMFDataType arg_type, const Void* arg);

    Int32 sendChunkSize(RtmpNode* node, Rtmp* rtmp, Uint32 chunkSize);
    Int32 sendBytesReport(RtmpNode* node, Rtmp* rtmp, Uint32 seqBytes);
    Int32 sendServBw(RtmpNode* node, Rtmp* rtmp, Uint32 bw);
    Int32 sendCliBw(RtmpNode* node, Rtmp* rtmp, Uint32 bw, Int32 limit);
    
    Int32 sendUsrCtrl(RtmpNode* node, Uint16 ev, Uint32 p1, Uint32 p2);
  
    Int32 dealRtmp(RtmpNode* node, Rtmp* rtmp, CacheHdr* hdr);

    Int32 dealHandshakeC01(RtmpNode* node, Rtmp* rtmp, CacheHdr* hdr);

    Int32 dealHandshakeC2(RtmpNode* node, Rtmp* rtmp, CacheHdr* hdr);

    Int32 dealInvoke(RtmpNode* node, Rtmp* rtmp, CacheHdr* hdr);

    Int32 dealMetaData(RtmpNode* node, Rtmp* rtmp, CacheHdr* hdr);

    Int32 dealFlvData(RtmpNode* , Rtmp* , Int32 , CacheHdr* hdr);

    Int32 dealNotifyEndStream(RtmpNode* , Rtmp* rtmp, CacheHdr* hdr);

private:
    Int32 dealConn(RtmpNode* node, Rtmp* rtmp, AMFObject* obj);
    Int32 dealPlay(RtmpNode* node, Rtmp* rtmp, AMFObject* obj);
    Int32 dealPause(RtmpNode* node, Rtmp* rtmp, AMFObject* obj);
    Int32 dealPublish(RtmpNode* node, Rtmp* rtmp, AMFObject* obj);
    Int32 dealUnpublish(RtmpNode* node, Rtmp* rtmp, AMFObject* obj);
    Int32 dealDelStream(RtmpNode* node, Rtmp* rtmp, AMFObject* obj);
    Int32 dealCreatStream(RtmpNode* node, Rtmp* rtmp, AMFObject* obj); 
    
private:
    HandShake* m_handshake;
    AmfPayload* m_amf;
    StreamCenter* m_stream_center;
};


#endif

