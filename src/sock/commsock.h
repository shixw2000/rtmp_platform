#ifndef __COMMSOCK_H__
#define __COMMSOCK_H__
#include"sysoper.h"


enum EnumDomain {
    ENUM_DOMAIN_IP_V4 = 0,
    ENUM_DOMAIN_IP_V6 = 1,
    ENUM_DOMAIN_UNIX,

    ENUM_DOMAIN_END
};

enum EnumSockType {
    ENUM_SOCK_TYPE_TCP = 0,
    ENUM_SOCK_TYPE_UDP,

    ENUM_SOCK_TYPE_END
};

enum EnumSockLevel {
    ENUM_SOCK_LEVEL_SOL_SOCKET = 0,
    ENUM_SOCK_LEVEL_IPPROTO_IP,
    ENUM_SOCK_LEVEL_IPPROTO_UDP,
    
    ENUM_SOCK_LEVEL_END
};

enum EnumSockOptName {
    ENUM_SOCK_OPT_REUSE_ADDR = 0,
    ENUM_SOCK_SO_ERROR,
    ENUM_SOCK_OPT_LINGER,
    ENUM_SOCK_OPT_SNDBUF,
    ENUM_SOCK_OPT_RCVBUF,

    /* ip */
    ENUM_IP_OPT_ADD_MEM,
    ENUM_IP_OPT_DROP_MEM,
    ENUM_IP_OPT_HDR_INCL,
    ENUM_IP_OPT_MULTICAST_TTL,
        
    ENUM_SOCK_OPT_NAME_END
};

enum EnumSockState {
    ENUM_SOCK_STAT_FAIL = -1,
    ENUM_SOCK_STAT_COMPLETED = 0,
    ENUM_SOCK_STAT_IN_PROGRESS = 1,

    ENUM_SOCK_STAT_END = -2
};

static const Int32 DEF_OPT_VAL_ON = 1;
static const Int32 DEF_OPT_VAL_OFF = 1;
static const Int32 DEF_OPT_VAL_SIZE = sizeof(Int32);
static const Int32 MAX_IP_SIZE = 128;
static const Int32 MAX_ADDR_SIZE = 128;
static const Int32 COMM_BUFFER_SIZE = 128;
static const Byte UDP_MULTICAST_BASE = 224;
static const Int32 MAX_DEV_NAME_SIZE = 128;

struct IpInfo {
    Int32 m_port;
    Char m_ip[MAX_IP_SIZE];
};

struct AddrInfo {
    Int32 m_size;
    Byte m_addr[MAX_ADDR_SIZE];
};

class CommSock {
public:
    static Void copyAddr(const AddrInfo* src, AddrInfo* dst);
    static Void copyIP(const IpInfo* src, IpInfo* dst);
    static Void assignIP(const Char* ip, Int32 port, IpInfo* dst);
    
    static Int32 creatSock(EnumDomain domain, EnumSockType type, 
        Int32 protocols, Int32* pFd);

    static Int32 setSockOpt(Int32 fd, EnumSockLevel level,
        EnumSockOptName optName,
        const Void* optVal, Int32 optLen);

    static Int32 getSockOpt(Int32 fd, EnumSockLevel level,
        EnumSockOptName optName,
        Void* optVal, Int32* optLen);

    static Int32 setNonBlock(Int32 fd);
    static Int32 setReuse(Int32 fd);
    static Int32 setNoTimewait(Int32 fd);
    static Int32 setMulticastTtl(Int32 fd, Int32 ttl);

    static Bool isValidPort(Int32 port);

    static Int32 ip2Addr(EnumDomain domain, EnumSockType type,
        const IpInfo* ip, AddrInfo* addr);
    
    static Int32 addr2IP(const AddrInfo* addr, IpInfo* ip);

    static Int32 ipstr2N(EnumDomain domain, const Char* ip, 
        Byte (&addr)[MAX_ADDR_SIZE]);
    
    static Int32 ipn2Str(EnumDomain domain, const Void* addr,
        Char (&ip)[MAX_IP_SIZE]);

    static Int32 closeHd(Int32 fd);

    static Int32 bindAddr(Int32 fd, const AddrInfo* addr);
    
    static Int32 getLocalInfo(Int32 fd, IpInfo* ip, AddrInfo* addr);
    static Int32 getPeerInfo(Int32 fd, IpInfo* ip, AddrInfo* addr);

    /**
        On individual connections, the socket buffer size must be  set  prior  to
       the listen(2) or connect(2) calls in order to have it take effect.
        **/
    static Int32 setSndBufferSize(Int32 fd, Int32 size);
    static Int32 setRcvBufferSize(Int32 fd, Int32 size);
    static Int32 getSndBufferSize(Int32 fd, Int32* psize); 
    static Int32 getRcvBufferSize(Int32 fd, Int32* psize);
    

    static Int32 devName2Index(const Char* name, Int32* pIndex);
    static Int32 devIndex2Name(Int32 index, 
        Char (&devname)[MAX_DEV_NAME_SIZE]); 
};

#endif

