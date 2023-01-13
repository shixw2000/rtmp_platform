#ifndef __OUTPUTSTREAM_H__
#define __OUTPUTSTREAM_H__
#include"globaltype.h"
#include"datatype.h"


class MsgOutput {
public:
    EnumSockRet writeMsg(NodeBase* node);

private:
    EnumSockRet sendQue(Int32 fd, list_head* list);
    Void prune(list_head* list, Int32 maxlen);    
};

#endif

