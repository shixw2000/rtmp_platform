#ifndef __CTRLNODE_H__
#define __CTRLNODE_H__
#include"datatype.h"


class Director;

struct CtrlNode {
    NodeBase m_base;

    Int32 (*recv)(struct CtrlNode* _this, CacheHdr* hdr);
};

CtrlNode* creatCtrlNode(Director* director);

#endif

