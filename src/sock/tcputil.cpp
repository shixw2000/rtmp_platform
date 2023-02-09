#include<stdlib.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<netdb.h>
#include<netinet/in.h>
#include<netinet/ip.h>
#include<unistd.h>
#include<fcntl.h>
#include"commsock.h"
#include"misc.h"
#include"tcputil.h"


EnumSockState TcpUtil::connPeer(Int32 fd, const AddrInfo* peer) {
    Int32 ret = 0;
    
    ret = connect(fd, (const struct sockaddr*)peer->m_addr, peer->m_size);
    if (0 == ret) {
        return ENUM_SOCK_STAT_COMPLETED;
    } else if (EINPROGRESS == errno) {
        return ENUM_SOCK_STAT_IN_PROGRESS;
    } else {
        LOG_ERROR("conn_peer| fd=%d| addr_len=%d| error=%s|", 
            fd, peer->m_size, ERR_MSG());

        return ENUM_SOCK_STAT_FAIL;
    } 
}

Int32 TcpUtil::listenTcp(Int32 fd, Int32 maxConn) {
    Int32 ret = 0;
    
    ret = listen(fd, maxConn);
    if (0 == ret) {
        LOG_INFO("listen_svc| fd=%d| maxConn=%d| msg=ok|", fd, maxConn);

        return 0;
    } else {
        LOG_ERROR("listen_tcp| ret=%d| fd=%d| maxConn=%d| error=%s|",
            ret, fd, maxConn, ERR_MSG());
        
        return -1; 
    } 
}

EnumSockState TcpUtil::chkConnStatus(Int32 fd) {
    Int32 ret = 0;
    Int32 errcode = 0;
	Int32 optLen = 0;

    optLen = sizeof(errcode);
    ret = CommSock::getSockOpt(fd, ENUM_SOCK_LEVEL_SOL_SOCKET,
        ENUM_SOCK_SO_ERROR, &errcode, &optLen);
    if (0 == ret && 0 == errcode) {
        return ENUM_SOCK_STAT_COMPLETED;
    } else {
        LOG_ERROR("chk_conn_status| ret=%d| errcode=%d| error=%s|",
            ret, errcode, strerror(errcode));
        return ENUM_SOCK_STAT_FAIL;
    }
}

EnumSockState TcpUtil::connFast(const IpInfo* peer, Int32* pfd) {
    Int32 ret = 0;
    Int32 fd = -1; 
    EnumSockState enStat = ENUM_SOCK_STAT_END;
    AddrInfo addr;

    do {
        ret = CommSock::ip2Addr(ENUM_DOMAIN_IP_V4, 
            ENUM_SOCK_TYPE_TCP, peer, &addr);
        if (0 != ret) { 
            break;
        }

        ret = CommSock::creatSock(ENUM_DOMAIN_IP_V4, 
            ENUM_SOCK_TYPE_TCP, 0, &fd);
        if (0 != ret) {
            break;
        } 
    
        CommSock::setNonBlock(fd);
        CommSock::setNoTimewait(fd);
        //CommSock::setSndBufferSize(fd, DEF_TCP_SEND_BUF_SIZE);
        //CommSock::setRcvBufferSize(fd, DEF_TCP_RCV_BUF_SIZE);

        enStat = connPeer(fd, &addr);
        if (ENUM_SOCK_STAT_COMPLETED == enStat
            || ENUM_SOCK_STAT_IN_PROGRESS == enStat) {
            *pfd = fd; 
        } else { 
            break;
        }

        LOG_INFO("conn_fast| stat=%d| fd=%d| peer=%s:%d|"
            " msg=connect done|",
            enStat, fd, peer->m_ip, peer->m_port);

        return enStat;
    } while (0); 

    LOG_ERROR("conn_fast| stat=%d(%d)| peer=%s:%d| msg=connect fail|",
        enStat, ret, peer->m_ip, peer->m_port);
    
    CommSock::closeHd(fd);
    *pfd = -1;
    return ENUM_SOCK_STAT_FAIL;
}

EnumSockState TcpUtil::connSlow(const IpInfo* peer, Int32* pfd) {
    Int32 ret = 0;
    Int32 fd = -1; 
    EnumSockState enStat = ENUM_SOCK_STAT_END;
    AddrInfo addr;

    do {
        ret = CommSock::ip2Addr(ENUM_DOMAIN_IP_V4, 
            ENUM_SOCK_TYPE_TCP, peer, &addr);
        if (0 != ret) { 
            break;
        }

        ret = CommSock::creatSock(ENUM_DOMAIN_IP_V4, 
            ENUM_SOCK_TYPE_TCP, 0, &fd);
        if (0 != ret) {
            break;
        } 

        //CommSock::setSndBufferSize(fd, DEF_TCP_SEND_BUF_SIZE);
        //CommSock::setRcvBufferSize(fd, DEF_TCP_RCV_BUF_SIZE); 
        
        enStat = connPeer(fd, &addr);
        if (ENUM_SOCK_STAT_COMPLETED == enStat) {
            CommSock::setNonBlock(fd);
            CommSock::setNoTimewait(fd); 

            *pfd = fd; 
        } else {
            break;
        }

        LOG_INFO("conn_slow| fd=%d| peer=%s:%d|"
            " msg=connect ok|",
            ret, fd, peer->m_ip, peer->m_port); 
        
        return ENUM_SOCK_STAT_COMPLETED;
    } while (0); 

    LOG_ERROR("conn_slow| stat=%d(%d) peer=%s:%d|"
        " msg=connect fail|",
        enStat, ret, peer->m_ip, peer->m_port);
    
    CommSock::closeHd(fd);
    *pfd = -1;
    return ENUM_SOCK_STAT_FAIL;
}

Int32 TcpUtil::creatSvc(const IpInfo* myIp, Int32* pfd) {
    Int32 ret = 0;
    Int32 fd = -1; 
    AddrInfo addr; 

    do { 
        ret = CommSock::ip2Addr(ENUM_DOMAIN_IP_V4, 
            ENUM_SOCK_TYPE_TCP, myIp, &addr);
        if (0 != ret) { 
            break;
        } 

        ret = CommSock::creatSock(ENUM_DOMAIN_IP_V4, 
            ENUM_SOCK_TYPE_TCP, 0, &fd);
        if (0 == ret) {
            CommSock::setNonBlock(fd);
            CommSock::setReuse(fd);
            //CommSock::setSndBufferSize(fd, DEF_TCP_SEND_BUF_SIZE);
            //CommSock::setRcvBufferSize(fd, DEF_TCP_RCV_BUF_SIZE);
        } else {
            break;
        }
        
        ret = CommSock::bindAddr(fd, &addr);
        if (0 != ret) { 
            break; 
        }

        ret = listenTcp(fd, DEF_LISTEN_SIZE);
        if (0 != ret) { 
            break; 
        } 

        LOG_INFO("creat_tcp_svc| fd=%d| ip=%s:%d| msg=ok|", 
            fd, myIp->m_ip, myIp->m_port);
        
        *pfd = fd;
        return 0;
    } while (0);

    LOG_ERROR("creat_tcp_svc| ret=%d| ip=%s:%d| msg=failed",
        ret, myIp->m_ip, myIp->m_port);

    CommSock::closeHd(fd);
    *pfd = -1;
    return ret;
}

Int32 TcpUtil::shutdownHd(Int32 fd) {
    Int32 ret = 0;
    
    if (0 <= fd) {
        ret = shutdown(fd, SHUT_RDWR);
    }

    return ret;
}

EnumSockState TcpUtil::acceptTcp(Int32 listener, Int32* pfd, IpInfo* peer) {
    Int32 fd = -1;
    socklen_t addrLen = 0;
    AddrInfo addr;

    addrLen = sizeof(addr.m_addr);
    fd = accept(listener, (struct sockaddr*)addr.m_addr, &addrLen);
    if (0 <= fd) {
        CommSock::setNonBlock(fd);
        CommSock::setNoTimewait(fd);
        
        addr.m_size = addrLen;

        if (NULL != peer) {
            CommSock::addr2IP(&addr, peer);
        }
        
        *pfd = fd; 
        return ENUM_SOCK_STAT_IN_PROGRESS;
    } else if (EAGAIN == errno) {
        *pfd = -1;
        return ENUM_SOCK_STAT_COMPLETED;
    } else {
        *pfd = -1;
        return ENUM_SOCK_STAT_FAIL;
    }
}

Int32 TcpUtil::sendTcp(Int32 fd, const Void* buf, Int32 len) {
    Int32 sndlen = 0;
    Int32 flags = MSG_NOSIGNAL;
    
    if (0 < len) {
        sndlen = send(fd, buf, len, flags);
        if (0 <= sndlen) {
            LOG_DEBUG("sendTcp| fd=%d| snd_len=%d| max_len=%d|"
                " msg=ok|",
                fd, sndlen, len); 
        } else if (EAGAIN == errno) {
            /* blocked and wait  */
            sndlen = 0;
        } else {
            LOG_ERROR("sendTcp| fd=%d| snd_len=%d| maxlen=%d| error=%s|",
                fd, sndlen, len, ERR_MSG());
            
            sndlen = -1;
        }
    } 

    return sndlen;
}

Int32 TcpUtil::recvTcp(Int32 fd, Void* buf, Int32 maxlen) {
    Int32 rdlen = 0;
    Int32 flags = 0;

    if (0 < maxlen) { 
        rdlen = recv(fd, buf, maxlen, flags);
        if (0 < rdlen) {
            LOG_DEBUG("recvTcp| fd=%d| maxlen=%d| rdlen=%d| msg=ok|",
                fd, maxlen, rdlen);
        } else if (0 == rdlen) {
            /* end of read */
            LOG_ERROR("recvTcp| fd=%d| maxlen=%d| error=eof|",
                fd, maxlen);
            
            rdlen = -2;
        } else if (EAGAIN == errno) {
            /* empty and must wait to  */
            rdlen = 0;
        } else {
            LOG_ERROR("recvTcp| fd=%d| maxlen=%d| error=%s|",
                fd, maxlen, ERR_MSG());
            
            rdlen = -1;
        }
    } 

    return rdlen;
}

