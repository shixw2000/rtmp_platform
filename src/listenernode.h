#ifndef __LISTENERNODE_H__
#define __LISTENERNODE_H__
#include"datatype.h"


class Director;

struct ListenerNode {
    NodeBase m_base;
};

ListenerNode* creatListenerNode(Int32 fd, Director* director);

#endif

