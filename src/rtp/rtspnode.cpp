#include<stdlib.h>
#include"rtspnode.h"
#include"objcenter.h"
#include"director.h"
#include"cache.h"
#include"sockutil.h"
#include"rtspmsg.h"
#include"rtspcenter.h"
#include"listenernode.h"
#include"sock/commsock.h"
#include"sock/udputil.h"
#include"rtpnode.h"


struct RtspListenerPriv {
    ListenerNode m_pub;

    Director* m_director; 
    Int32 m_listener_fd;
    Rtp m_rtp;
};

struct RtspNodePriv {
    RtspNode m_pub;

    Director* m_director;
    ObjCenter* m_center;
    RtspHandler* m_handler;

    Rtsp m_rtsp;
};

static Int32 parse(NodeBase* node, Byte* data, Int32 len) {
    Int32 ret = 0;
    RtspNodePriv* _this = (RtspNodePriv*)node;

    ret = _this->m_handler->parseRtsp(&_this->m_rtsp, data, len); 
    return ret;
}

static Void initRtsp(Rtsp* rtsp) {
    rtsp->m_fd = -1;    
    
    rtsp->m_input.m_rd_stat = ENUM_RTSP_DEC_INIT;
}

static Void finishRtsp(Rtsp* rtsp) {
    RtspInput* chn = &rtsp->m_input;
    
    if (NULL != chn->m_txt) {
        free(chn->m_txt);
        chn->m_txt = NULL;
    }

    if (0 <= rtsp->m_fd) {
        closeHd(rtsp->m_fd);
        rtsp->m_fd = -1;
    }

    rtsp->m_sess = NULL;
    rtsp->m_rtp = NULL;
}

METHOD(NodeBase, getFd, Int32, RtspNodePriv* _this) {
    return _this->m_rtsp.m_fd;
}

METHOD(NodeBase, readNode, EnumSockRet, RtspNodePriv* _this) {
	EnumSockRet ret = ENUM_SOCK_MARK_FINISH;

    ret = _this->m_center->readSock(&_this->m_pub.m_base, &parse);

    return ret;
}

METHOD(NodeBase, writeNode, EnumSockRet, RtspNodePriv* _this) {
    EnumSockRet ret = ENUM_SOCK_MARK_FINISH;

    ret = _this->m_center->writeSock(&_this->m_pub.m_base); 
    return ret;
}

METHOD(NodeBase, dealMsg, Int32, RtspNodePriv* _this, CacheHdr* hdr) {
    Int32 ret = 0;

    ret = _this->m_handler->dealRtsp(&_this->m_rtsp, hdr);
    return ret;
}

METHOD(NodeBase, dealCmd, Void, RtspNodePriv* , CacheHdr* ) {
    
    return;
}

METHOD(NodeBase, onClose, Void, RtspNodePriv* _this) {
    Bool hasChild = FALSE;
    Rtsp* rtsp = &_this->m_rtsp;

    (Void)rtsp->m_sess;

    /* has no child */
    if (!hasChild) {
        _this->m_director->notifyExit(&_this->m_pub.m_base);
    }
}

METHOD(NodeBase, onChildExit, Void, RtspNodePriv* _this, NodeBase*) {
    Bool hasChild = FALSE;
    Rtsp* rtsp = &_this->m_rtsp;

    (Void)rtsp->m_sess;
    
    /* has no child */
    if (!hasChild && _this->m_pub.m_base.m_stop_deal) {
        _this->m_director->notifyExit(&_this->m_pub.m_base);
    }
}

METHOD(NodeBase, destroy, Void, RtspNodePriv* _this) { 
    finishRtsp(&_this->m_rtsp);
    
    ObjCenter::finishNode(&_this->m_pub.m_base); 
    
    I_FREE(_this);
}


METHOD(RtspNode, onAccept, Int32, RtspNodePriv* ) {
    return 0;
}

METHOD(RtspNode, onConnOk, Int32, RtspNodePriv* ) {
    return 0;
}

METHOD(RtspNode, onConnFail, Void, RtspNodePriv* ) {
    return;
}

METHOD(RtspNode, recv, Int32, RtspNodePriv* _this, CacheHdr* hdr) {
    Int32 ret = 0;

    ret = _this->m_director->dispatch(&_this->m_pub.m_base, hdr); 
    return ret;
}

METHOD(RtspNode, sendData, Int32, RtspNodePriv* _this, 
    Char* data1, Int32 data1Len,
    Char* data2, Int32 data2Len, Cache* cache) {
    Int32 ret = 0;
    CacheHdr* hdr = NULL;

    hdr = MsgCenter::creatSendMsg((Byte*)data1, data1Len, 
        (Byte*)data2, data2Len, cache);
    
    ret = _this->m_director->send(&_this->m_pub.m_base, hdr);
    return ret;
}

RtspNode* creatRtspNode(Int32 fd, Rtp* rtp, Director* director) {
    RtspNodePriv* _this = NULL;

    I_NEW(RtspNodePriv, _this);
    CacheCenter::zero(_this, sizeof(RtspNodePriv));

    ObjCenter::initNode(&_this->m_pub.m_base, ENUM_NODE_RTSP); 
    
    initRtsp(&_this->m_rtsp);

    _this->m_rtsp.m_fd = fd;
    _this->m_rtsp.m_rtp = rtp;
    _this->m_rtsp.m_entity = &_this->m_pub;

    CommSock::getLocalInfo(fd, &_this->m_rtsp.m_my_ip, NULL);
    CommSock::getPeerInfo(fd, &_this->m_rtsp.m_peer_ip, NULL);
    
    _this->m_director = director;
    _this->m_center = director->getCenter(); 
    _this->m_handler = _this->m_center->getRtspHandler();

    director->condition(&_this->m_pub.m_base.m_rcv_task, BIT_EVENT_READ);
    director->condition(&_this->m_pub.m_base.m_snd_task, BIT_EVENT_WRITE);
    
    _this->m_pub.m_base.getFd = _getFd;
    _this->m_pub.m_base.readNode = _readNode;
    _this->m_pub.m_base.writeNode = _writeNode;

    _this->m_pub.m_base.dealMsg = _dealMsg;
    _this->m_pub.m_base.dealCmd = _dealCmd;
    
    _this->m_pub.m_base.onClose = _onClose;
    _this->m_pub.m_base.onChildExit = _onChildExit;
    _this->m_pub.m_base.destroy = _destroy;

    _this->m_pub.onAccept = _onAccept;
    _this->m_pub.onConnOk = _onConnOk;
    _this->m_pub.onConnFail = _onConnFail;
    
    _this->m_pub.recv = _recv;
    _this->m_pub.sendData = _sendData;
  
    return &_this->m_pub;
}


static Void acceptRtsp(RtspListenerPriv* _this) { 
    Int32 ret = 0;
    Int32 newfd = -1;
    int peer_port = 0;
    Char peer_ip[DEF_IP_SIZE];
    RtspNode* node = NULL;

    newfd = acceptCli(_this->m_listener_fd);
    while (0 <= newfd) { 
        getPeerInfo(newfd, &peer_port, peer_ip, DEF_IP_SIZE);
        
        node = creatRtspNode(newfd, &_this->m_rtp, _this->m_director);
        if (NULL != node) {
            ret = node->onAccept(node);
            if (0 == ret) {
                LOG_DEBUG("rtsp_accept| fd=%d| addr=%s:%d| msg=ok|",
                    newfd, peer_ip, peer_port);
                
                _this->m_director->registTask(&node->m_base, EVENT_TYPE_ALL);
            } else {
                LOG_ERROR("rtsp_accept| fd=%d| addr=%s:%d|"
                    " msg=onAccept error|",
                    newfd, peer_ip, peer_port);
                
                node->m_base.destroy(&node->m_base);
            }
        } else {
            LOG_ERROR("rtsp_accept| fd=%d| addr=%s:%d|"
                " msg=creat node error|",
                newfd, peer_ip, peer_port);
            
            closeHd(newfd);
        }

        newfd = acceptCli(_this->m_listener_fd);
    }
}


METHOD(NodeBase, rtsp_getFd, Int32, RtspListenerPriv* _this) {
    return _this->m_listener_fd;
}

METHOD(NodeBase, rtsp_readNode, EnumSockRet, RtspListenerPriv* _this) {
    acceptRtsp(_this);

    return ENUM_SOCK_MARK_FINISH;
}

METHOD(NodeBase, rtsp_onClose, Void, RtspListenerPriv* _this) {
    Bool hasChild = FALSE;

    if (NULL != _this->m_rtp.m_rtp_node) {
        _this->m_director->unregTask(&_this->m_rtp.m_rtp_node->m_base,
            ENUM_ERR_SOCK_PARENT_STOP);

        if (!hasChild) {
            hasChild = TRUE;
        }
    }

    if (NULL != _this->m_rtp.m_rtcp_node) {
        _this->m_director->unregTask(&_this->m_rtp.m_rtcp_node->m_base,
            ENUM_ERR_SOCK_PARENT_STOP);

        if (!hasChild) {
            hasChild = TRUE;
        }
    }

    if (!hasChild) {
        _this->m_director->notifyExit(&_this->m_pub.m_base);
    } 
}

METHOD(NodeBase, rtsp_onChildExit, Void, RtspListenerPriv* _this,
    NodeBase* child) {
    Bool hasChild = FALSE;
    RtpNode* node = (RtpNode*)child;

    if (NULL != _this->m_rtp.m_rtp_node) {
        if (node == _this->m_rtp.m_rtp_node) {
            _this->m_director->notifyExit(child);
            _this->m_rtp.m_rtp_node = NULL;
        } else {
            hasChild = TRUE;
        }
    }

    if (NULL != _this->m_rtp.m_rtcp_node) {
        if (node == _this->m_rtp.m_rtcp_node) {
            _this->m_director->notifyExit(child);
            _this->m_rtp.m_rtcp_node = NULL;
        } else {
            hasChild = TRUE;
        }
    }
    
    if (!hasChild && _this->m_pub.m_base.m_stop_deal) {
        _this->m_director->notifyExit(&_this->m_pub.m_base);
    }
}

METHOD(NodeBase, rtsp_destroy, Void, RtspListenerPriv* _this) {
    if (NULL != _this->m_rtp.m_rtp_node) {
        _this->m_rtp.m_rtp_node->m_base.destroy(&_this->m_rtp.m_rtp_node->m_base);
        _this->m_rtp.m_rtp_node = NULL;
    }

    if (NULL != _this->m_rtp.m_rtcp_node) {
        _this->m_rtp.m_rtcp_node->m_base.destroy(&_this->m_rtp.m_rtcp_node->m_base);

        _this->m_rtp.m_rtcp_node = NULL;
    }
    
    ObjCenter::finishNode(&_this->m_pub.m_base); 

    closeHd(_this->m_listener_fd);
    I_FREE(_this);
}

static Int32 creatRtpPair(const Char* ip, Int32 port_base, Int32 fds[]) {
    Int32 ret = 0;
    
    ret = UdpUtil::creatUniSvc(port_base, ip, &fds[ENUM_RTSP_STREAM_RTP]);
    if (0 == ret) {
        ret = UdpUtil::creatUniSvc(port_base + 1, ip, 
            &fds[ENUM_RTSP_STREAM_RTCP]);

        if (0 == ret) {
            return 0;
        } else {
            CommSock::closeHd(fds[ENUM_RTSP_STREAM_RTP]);
            fds[ENUM_RTSP_STREAM_RTP] = -1;

            return -1;
        }
    } else {
        return -1;
    } 
}

static Int32 prepareRtp(RtspListenerPriv* _this) {
    Int32 ret = 0;
    Int32 port_base = 0;
    IpInfo my_ip;
    Int32 fds[ENUM_RTSP_STREAM_END] = {0};
    NodeBase* parent = &_this->m_pub.m_base;
    Rtp* rtp = &_this->m_rtp;
    
    CommSock::getLocalInfo(_this->m_listener_fd, &my_ip, NULL);

    port_base = MIN_RTSP_TRANSPORT_PORT;
    while (MAX_RTSP_TRANSPORT_PORT > port_base) {
        ret = creatRtpPair(my_ip.m_ip, port_base, fds);
        if (0 == ret) {
            break;
        } else {
            port_base += ENUM_RTSP_STREAM_END;
            continue;
        }
    }

    if (0 == ret) {
        strncpy(rtp->m_ip, my_ip.m_ip, sizeof(rtp->m_ip));
        rtp->m_port_base = port_base;
    } else {
        LOG_ERROR("creat_rtp| ret=%d| ip=%s| msg=creat rtp error|", 
            ret, my_ip.m_ip);
        
        return ret;
    }
        
    rtp->m_rtp_node = creatRtpNode(fds[ENUM_RTSP_STREAM_RTP], 
        ENUM_RTSP_STREAM_RTP, parent, _this->m_director);
    rtp->m_rtcp_node = creatRtpNode(fds[ENUM_RTSP_STREAM_RTCP], 
        ENUM_RTSP_STREAM_RTCP, parent, _this->m_director);        

    LOG_INFO("creat_rtp| ip=%s| port_base=%d| msg=ok|", 
        my_ip.m_ip, port_base);
    return 0;
}

ListenerNode* creatRtspListener(Int32 fd, Director* director) {
    Int32 ret = 0;
    RtspListenerPriv* _this = NULL;

    I_NEW(RtspListenerPriv, _this);
    memset(_this, 0, sizeof(*_this));

    ObjCenter::initNode(&_this->m_pub.m_base, ENUM_NODE_LISTENER);

    _this->m_director = director;
    _this->m_listener_fd = fd; 

    ret = prepareRtp(_this);
    if (0 == ret) {
        director->registTask(&_this->m_rtp.m_rtp_node->m_base, EVENT_TYPE_ALL);
        director->registTask(&_this->m_rtp.m_rtcp_node->m_base, EVENT_TYPE_ALL);

        director->condition(&_this->m_pub.m_base.m_rcv_task, BIT_EVENT_READ);
        
        _this->m_pub.m_base.getFd = _rtsp_getFd;
        _this->m_pub.m_base.readNode = _rtsp_readNode;
        _this->m_pub.m_base.onClose = _rtsp_onClose;
        _this->m_pub.m_base.onChildExit = _rtsp_onChildExit;
        _this->m_pub.m_base.destroy = _rtsp_destroy;

        return &_this->m_pub;
    } else {
        _this->m_pub.m_base.destroy(&_this->m_pub.m_base);
        return NULL;
    }
}

