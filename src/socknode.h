#ifndef __SOCKNODE_H__
#define __SOCKNODE_H__
#include"datatype.h"


class Director;

struct SockNode {
    NodeBase m_base;

    Int32 (*parsePkg)(struct SockNode* _this, Byte* data, Int32 len);
    Int32 (*onAccept)(struct SockNode* _this);
    Int32 (*onConnOk)(struct SockNode* _this);
    Void (*onConnFail)(struct SockNode* _this);
};

SockNode* creatSockNode(Int32 fd, Director* director);


#endif

