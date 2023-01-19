#ifndef __LISTENERNODE_H__
#define __LISTENERNODE_H__
#include"datatype.h"


class Director;

struct ListenerNode {
    NodeBase m_base;
};

ListenerNode* creatRtmpListener(Int32 fd, Director* director);
ListenerNode* creatSockListener(Int32 fd, Director* director);

#endif

