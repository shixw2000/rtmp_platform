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
    
    Int32 (*dealCtrl)(RtmpNode* _this, Cache* cache);

    Int32 (*onRecv)(RtmpNode* _this, Int32 msg_type, Cache* cache);

    Int32 (*dispatch)(RtmpNode* _this, CacheHdr* hdr);
    
    /* add ref of the cache */
    Int32 (*handshake)(RtmpNode* _this, Cache* cache);
    
    Int32 (*sendPayload)(RtmpNode* _this, Uint32 epoch, Uint32 stream_id,
        Uint32 rtmp_type, const Chunk* chunk);
    
    Int32 (*sendPkg)(RtmpNode* _this, Uint32 stream_id, Cache* cache); 
};

extern ChnInput* creatChnIn(Uint32 cid);
extern Void freeChnIn(ChnInput* chn);

extern Uint32 toMsgType(Uint32 rtmp_type);
extern Uint32 toCid(Uint32 type, Uint32 stream_id);

extern Cache* creatPkg(Uint32 epoch, Uint32 stream_id, 
    Uint32 rtmp_type, Uint32 payload_size);

extern RtmpUint* creatUnit(Uint32 userId, RtmpNode* node);
extern Void freeUnit(RtmpUint* unit);

RtmpNode* creatRtmpNode(Int32 fd, Director* director);

#endif

