#include<stdlib.h>
#include<sys/uio.h>
#include<sys/types.h>
#include<sys/socket.h> 
#include"commsock.h"
#include"udputil.h"
#include"misc.h"


static const Int32 TEST_BUFF_SIZE = 0x100000; // 1MB

Int32 udpCli(const IpInfo* my_ip, const IpInfo* peer_ip) {
    Int32 ret = 0;
    Uint32 seq = 0;
    Int32 fd = -1;
    Int32 choose = 0;
    Int32 len1 = 0;
    Int32 len2 = 0;
    Int32 appendLen = 0;
    Int32 bufLen = 0;
    Int32 sndlen = 0;
    Int32 rcvlen = 0;
    AddrInfo addr2;
    struct iovec iov[1];
    static Char buff1[TEST_BUFF_SIZE] = {0};
    static Char buff2[TEST_BUFF_SIZE] = {0};

    ret = CommSock::ip2Addr(ENUM_DOMAIN_IP_V4, 
        ENUM_SOCK_TYPE_UDP, peer_ip, &addr2);
    if (0 != ret) {
        return ret;
    } 
    
    ret = UdpUtil::creatUniSvc(my_ip->m_port, my_ip->m_ip, &fd);
    if (0 != ret) {
        return ret;
    } 

    LOG_INFO("Enter a sock_buff_len and chosen[0-vec, 1-sendto]:");
    scanf("%d%d", &bufLen, &choose);

    CommSock::setSndBufferSize(fd, bufLen);
    CommSock::setRcvBufferSize(fd, bufLen);

    CommSock::getSndBufferSize(fd, &ret);
    CommSock::getRcvBufferSize(fd, &ret); 

    LOG_INFO("Enter a send_append_len and rcv_total_len:");
    scanf("%d%d", &appendLen, &len2);

    
    do { 
        len1 = snprintf(buff1, sizeof(buff1), "<%u>====helo, world!", seq++);
        len1 += appendLen;
        
        iov[0].iov_base = buff1;
        iov[0].iov_len = len1;

        if (0 == choose) {
            sndlen = UdpUtil::sendUdp(fd, &addr2, iov, 1); 
        } else {
            sndlen = UdpUtil::sendTo(fd, buff1, len1, &addr2);
        }

        iov[0].iov_base = buff2;
        iov[0].iov_len = len2;

        if (0 == choose) {
            rcvlen = UdpUtil::recvUdp(fd, NULL, iov, 1); 
        } else {
            rcvlen = UdpUtil::recvFrom(fd, buff2, len2, NULL);
        }
        
        if (0 < rcvlen) {
            LOG_INFO("udp_cli_rcv| rcv_len=%d| msg=%.*s|",
                rcvlen, rcvlen, buff2);
        }
        
        if (0 <= sndlen && 0 <= rcvlen) {
            ret = 0;
            sleepSec(1);
        } else {
            ret = -1;
        } 
    } while (0 == ret);

    CommSock::closeHd(fd);
    return ret;
}

Int32 udpServ(Int32 port, const Char* localIP, const Char* groupIP) {
    Bool isMulticast = FALSE;
    Int32 ret = 0;
    Int32 fd = -1;
    Int32 len1 = 0;
    Int32 sndlen = 0;
    Int32 rcvlen = 0;
    AddrInfo addr;
    struct iovec iov[2];
    static Char buff[TEST_BUFF_SIZE] = {0};
    
    if (NULL != groupIP) { 
        isMulticast = TRUE;
        ret = UdpUtil::creatMultiSvc(port, groupIP, localIP, &fd);
    } else {
        ret = UdpUtil::creatUniSvc(port, localIP, &fd);
    }

    if (0 != ret) {
        return ret;
    }

    CommSock::getSndBufferSize(fd, &ret);
    CommSock::getRcvBufferSize(fd, &ret);

    do {
        iov[0].iov_base = buff;
        iov[0].iov_len = sizeof(buff);
        
        rcvlen = UdpUtil::recvUdp(fd, &addr, iov, 1); 
        if (0 < rcvlen) {
            iov[0].iov_base = buff;
            iov[0].iov_len = rcvlen;

            LOG_INFO("udp_serv_rcv| rcv_len=%d| msg=%.*s|",
                rcvlen, rcvlen, buff);

            len1 = snprintf(&buff[rcvlen], 30, "=====>>>hhahahahaha");
            iov[1].iov_base = &buff[rcvlen];
            iov[1].iov_len = len1;
            sndlen = UdpUtil::sendUdp(fd, &addr, iov, 2);
        }

        if (0 <= sndlen && 0 <= rcvlen) {
            ret = 0;
            sleepSec(1);
        } else {
            ret = -1;
        } 
    } while (0 == ret); 

    /* drop from a group */
    if (isMulticast) {
        UdpUtil::operMemship(fd, localIP, groupIP, FALSE);
    }
    
    CommSock::closeHd(fd);
    return ret;
}


#ifdef __TEST__

static Void usage(Int32 , Char* argv[]) {
    LOG_ERROR("usage: %s <opt> <my_ip> <my_port>"
        " [<peer_ip> [<peer_port>]]",
        argv[0]);
}

Int32 main(Int32 argc, Char* argv[]) {
    Int32 ret = 0;
    Int32 opt = 0;
    IpInfo my_ip;
    IpInfo peer_ip;

    initLib(); 

    if (2 <= argc) {
        opt = atoi(argv[1]);
    } else {
        usage(argc, argv);
        return -1;
    }
    
    switch (opt) {
    case 0:
        /* udp client */
        if (6 == argc) { 
            strncpy(my_ip.m_ip, argv[2], sizeof(my_ip.m_ip));
            my_ip.m_port = atoi(argv[3]);

            strncpy(peer_ip.m_ip, argv[4], sizeof(peer_ip.m_ip));
            peer_ip.m_port = atoi(argv[5]);
            
            ret = udpCli(&my_ip, &peer_ip);
        } else {
            usage(argc, argv);
        }
        
        break;        
        
    case 1:
        /* udp server */
        if (4 == argc) { 
            ret = udpServ(atoi(argv[3]), argv[2], NULL);
        } else if (5 == argc) { 
            ret = udpServ(atoi(argv[3]), argv[2], argv[4]);
        } else {
            usage(argc, argv);
        } 
        
        break;

    default:
        usage(argc, argv);
        break;
    } 

    finishLib(); 
    return ret;
}

#endif

