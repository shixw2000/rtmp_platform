#ifndef __RTSPNODE_H__
#define __RTSPNODE_H__
#include"datatype.h"


struct CacheHdr;
struct ListenerNode;
struct RtpNode;
struct Rtp;
class Director;

struct RtspNode {
    NodeBase m_base;

    Int32 (*onAccept)(RtspNode* _this);
    Int32 (*onConnOk)(RtspNode* _this);
    Void (*onConnFail)(RtspNode* _this); 

    Int32 (*recv)(RtspNode* _this, CacheHdr* hdr);
    
    Int32 (*sendData)(RtspNode* _this, Char* data1, Int32 data1Len,
        Char* data2, Int32 data2Len, Cache* cache);
};

RtspNode* creatRtspNode(Int32 fd, Rtp* rtp, Director* director); 
ListenerNode* creatRtspListener(Int32 fd, Director* director);


#endif

