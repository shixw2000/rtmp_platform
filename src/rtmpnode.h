#ifndef __RTMPNODE_H__
#define __RTMPNODE_H__
#include"datatype.h"
#include"socknode.h"


struct CacheHdr;
struct Cache;
class Director;

struct RtmpNode {
    NodeBase m_base;

    Int32 (*parsePkg)(RtmpNode* _this, Byte* data, Int32 len);
    Int32 (*onAccept)(RtmpNode* _this);
    Int32 (*onConnOk)(RtmpNode* _this);
    Void (*onConnFail)(RtmpNode* _this); 

    /* begin receive functions */
    Int32 (*recv)(RtmpNode* _this, Int32 msg_type, Uint32 sid, Cache* cache);

    Int32 (*recvCmd)(RtmpNode* _this, Int32 msg_type, Int64 val);

    /* end receive functions */
    
    
    /* begin send functions */
    Int32 (*sendHandshake)(RtmpNode* _this, Cache* cache);
    
    Int32 (*sendPayload)(RtmpNode* _this, Uint32 sid,
        Uint32 rtmp_type, const Chunk* chunk);
    
    Int32 (*sendPkg)(RtmpNode* _this, Uint32 sid, Uint32 delta_ts, Cache* payload); 

    /* end send functions */
};

extern Bool chkRtmpVer(Byte ver);

extern ChnInput* creatChnIn(Uint32 cid);
extern Void freeChnIn(ChnInput* chn);

extern Uint32 toMsgType(Uint32 rtmp_type);
extern Uint32 toCid(Uint32 msg_type, Uint32 sid);
extern Bool chkFlvData(Uint32 msg_type);

extern Cache* creatPkg(Uint32 rtmp_type, Uint32 payload_size);

RtmpNode* creatRtmpNode(Int32 fd, Director* director);

#endif

