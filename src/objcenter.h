#ifndef __OBJCENTER_H__
#define __OBJCENTER_H__
#include"sysoper.h"
#include"datatype.h"


#define METHOD(iface, name, ret, this, ...) \
	static ret name(this, ##__VA_ARGS__); \
	typedef ret (*F_##name)(iface*, ##__VA_ARGS__); \
	static F_##name _##name = (F_##name)name; \
	static ret name(this, ##__VA_ARGS__)


struct RtmpNode;
struct Rtmp;
class MsgOutput;
class RtmpInput;
class RtmpHandler;

typedef Int32 (*callback)(NodeBase* node, Byte* data, Int32 len);

class ObjCenter {
public:
    ObjCenter();
    ~ObjCenter();

    Int32 init();
    Void finish();
    
    static Void initNode(NodeBase* node);
    static Void finishNode(NodeBase* node);
    static Void freeMsgQue(list_head* list);

    EnumSockRet readSock(NodeBase* node, callback cb);
    EnumSockRet writeSock(NodeBase* node);

    Int32 parseRtmp(Rtmp* rtmp, Byte* data, Int32 len);

    inline RtmpHandler* getRtmpHandler() const {
        return m_handler;
    }

private:
    MsgOutput* m_output;
    RtmpInput* m_input;
    RtmpHandler* m_handler;
    Byte m_buf[DEF_TCP_RCV_BUF_SIZE];
};

#endif

