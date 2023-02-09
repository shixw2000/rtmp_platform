#ifndef __TCPUTIL_H__
#define __TCPUTIL_H__
#include"sysoper.h"
#include"commsock.h"


static const Int32 DEF_LISTEN_SIZE = 1000;

class TcpUtil {
public:
    static EnumSockState connPeer(Int32 fd, const AddrInfo* peer);
    static Int32 listenTcp(Int32 fd, Int32 maxConn);

    static EnumSockState chkConnStatus(Int32 fd);
    
    static EnumSockState connFast(const IpInfo* peer, Int32* pfd);
    static EnumSockState connSlow(const IpInfo* peer, Int32* pfd);

    static Int32 creatSvc(const IpInfo* myIp, Int32* pfd);

    /**
        result means:
            ENUM_SOCK_STAT_COMPLETED: read all and need wait,
            ENUM_SOCK_STAT_IN_PROGRESS: can read more
            ENUM_SOCK_STAT_FAIL: error to close
        **/
    static EnumSockState acceptTcp(Int32 listener, Int32* pfd, IpInfo* peer);
    
    static Int32 shutdownHd(Int32 fd);

    static Int32 sendTcp(Int32 fd, const Void* buf, Int32 len);

    static Int32 recvTcp(Int32 fd, Void* buf, Int32 maxlen);
};

#endif

