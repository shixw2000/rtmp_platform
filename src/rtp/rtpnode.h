#ifndef __RTPNODE_H__
#define __RTPNODE_H__
#include"datatype.h"


struct CacheHdr;
class Director;

struct RtpNode {
    NodeBase m_base;

    Int32 (*recv)(RtpNode* _this, CacheHdr* hdr);
};

RtpNode* creatRtpNode(Int32 fd, Int32 node_type, NodeBase* parent, 
    Director* director);

#endif

