#ifndef __UDPUTIL_H__
#define __UDPUTIL_H__
#include"sysoper.h"


struct iovec;
struct AddrInfo;

class UdpUtil {
public:
    static Int32 creatUdp(Int32* pFd);

    static Int32 creatUniSvc(Int32 port, const Char* uniIP, Int32* pfd);
    
    static Int32 creatMultiSvc(Int32 port, const Char* multiIP,
        const Char* localIP, Int32* pfd);

    static Int32 sendTo(Int32 fd, const Void* buf, 
        Int32 maxlen, const AddrInfo* peer);

    static Int32 recvFrom(Int32 fd, Void* buf, 
        Int32 maxlen, AddrInfo* peer);

    static Int32 recvUdp(int fd, AddrInfo* peer, 
        struct iovec* iov, int iov_size);
    
    
    static Int32 sendUdp(int fd, const AddrInfo* peer,
        struct iovec* iov, int iov_size);
    

    static Int32 operMemship(Int32 fd, const Char* localIp,
        const Char* groupIp, Bool added);
};

#endif

