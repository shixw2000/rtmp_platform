#ifndef __RTMPPROTO_H__
#define __RTMPPROTO_H__
#include"globaltype.h"
#include"datatype.h"
#include"payload.h"


struct Cache;
class AmfPayload;

class RtmpProto {
public:
    explicit RtmpProto(AmfPayload* amf) : m_amf(amf) {}
    
    Cache* genStatus(const Chunk* status, const Chunk* path,
        const Char* desc);

    Cache* genResult(const double* txn, const AMFObject* info,
        AMFDataType arg_type, const Void* arg); 

    Int32 sendStatus(Rtmp* rtmp, Uint32 sid, const Chunk* status, 
        const Chunk* path, const Char* desc);

    Int32 sendResult(Rtmp* rtmp, Uint32 sid, const double* txn, 
        const AMFObject* info = NULL,
        AMFDataType arg_type = AMF_INVALID, const Void* arg = NULL);

    Int32 sendCall(Rtmp* rtmp, Uint32 sid, 
        const Chunk* call, const double* txn, 
        const AMFObject* info = NULL,
        AMFDataType arg_type = AMF_INVALID, const Void* arg = NULL);

    /* the rtmp receive the internal msg and then start to deal it */
    Int32 recvCmd(Rtmp* rtmp, Int32 msg_type, Int64 val);

    /* set sid of hdr as the same of unit, not sid of the original pkg */
    Int32 recv(RtmpUint* unit, Int32 msg_type, Cache* cache);

    /* set sid of hdr as the same of unit, not sid of the original pkg */
    Int32 send(RtmpUint* unit, Uint32 delta_ts, Cache* cache);

    Int32 sendChunkSize(Rtmp* rtmp, Uint32 chunkSize);
    Int32 sendBytesReport(Rtmp* rtmp, Uint32 seqBytes);
    Int32 sendServBw(Rtmp* rtmp, Uint32 bw);
    Int32 sendCliBw(Rtmp* rtmp, Uint32 bw, Int32 limit);
    Int32 sendUsrCtrl(Rtmp* rtmp, Uint16 ev, Uint32 p1, Uint32 p2 = 0);

    Void notifyStart(Rtmp* rtmp, Uint32 sid, const Chunk* code, 
        const Char* desc);
    Void notifyStop(Rtmp* rtmp, Uint32 sid, const Chunk* code, 
        const Char* desc);

private:
    Int32 sendCtrl(Rtmp* rtmp, Uint32 rtmp_type, const Chunk* chunk);
    
    Cache* genCall(const Chunk* call, const double* txn, 
        const AMFObject* info, AMFDataType arg_type, 
        const Void* arg);

private:
    AmfPayload* m_amf;    
};

#endif

