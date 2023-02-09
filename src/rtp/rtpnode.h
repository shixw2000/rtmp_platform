#ifndef __RTPNODE_H__
#define __RTPNODE_H__
#include"datatype.h"


struct CacheHdr;
struct RtspNode;
struct IpInfo;
class Director;

struct RtpNode {
    NodeBase m_base;

    Int32 (*recv)(RtpNode* _this, CacheHdr* hdr);
};

RtpNode* creatRtpNode(Int32 fd, RtspNode* parent,
    Director* director); 


struct RtcpNode {
    NodeBase m_base;

    Int32 (*recv)(RtcpNode* _this, CacheHdr* hdr);
};

RtcpNode* creatRtcpNode(Int32 fd, RtspNode* parent,
    Director* director); 

#endif

