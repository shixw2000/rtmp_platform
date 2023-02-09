#ifndef __RTPDEC_H__
#define __RTPDEC_H__
#include"datatype.h"
#include"rtpmsg.h"
#include"sock/commsock.h"
#include"encoding/protocol.h"


struct AddrInfo;
struct Cache;
struct CacheHdr;

class RtpTools {
public:
    static CacheHdr* creatUdpSendMsg(const AddrInfo* addr,
        Byte* data1, Int32 data1Len,
        Byte* data2, Int32 data2Len, Cache* cache);

    static EnumSockState writeUdpMsg(Int32 fd, CacheHdr* hdr);
    
    static EnumSockRet writeUdpNode(NodeBase* node);

    static Cache* creatRtpPkg(const AddrInfo* addr, 
        const Void* data, Int32 len);
};

class RtpDecoder : public ProtoDecoder {
public:    
    Int32 parseObj(Int32 cnt, const EncodingRule* rule, Void* obj);

    Bool isRtcpMsg(Int32 msg_type);

    Int32 parseRtp(RtpPkg* pkg);
    Int32 parseRtcp(RtpPkg* pkg);
    Int32 parseSR(const RtcpHdr* hdr);
    Int32 parseRR(const RtcpHdr* hdr);
    Int32 parseSDes(const RtcpHdr* hdr);
};

#endif

