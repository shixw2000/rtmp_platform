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

    for (Int32 i=0; i<MAX_RTSP_STREAM_NUM; ++i) {
        rtsp->m_streams[i].m_unit_id = i; 
    }
    
    rtsp->m_input.m_rd_stat = ENUM_RTSP_DEC_INIT;
}

static Void finishRtsp(Rtsp* rtsp) {
    RtspInput* chn = &rtsp->m_input;

    for (Int32 i=0; i<MAX_RTSP_STREAM_NUM; ++i) {
        if (NULL != rtsp->m_streams[i].m_node) {
            rtsp->m_streams[i].m_node = NULL;
        }
    }
    
    if (NULL != chn->m_txt) {
        free(chn->m_txt);
        chn->m_txt = NULL;
    }

    if (0 <= rtsp->m_fd) {
        closeHd(rtsp->m_fd);
        rtsp->m_fd = -1;
    }
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
    Rtsp* rtsp = &_this->m_rtsp;

    for (Int32 i=0; i<MAX_RTSP_STREAM_NUM; ++i) {
        if (NULL != rtsp->m_streams[i].m_node) {

            _this->m_director->unregTask(rtsp->m_streams[i].m_node, 
                ENUM_ERR_SOCK_PARENT_STOP);
            
            rtsp->m_streams[i].m_node = NULL;
        }
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

METHOD(RtspNode, genRtp, NodeBase*, RtspNodePriv* _this, Int32 fd) {
    RtpNode* child = NULL;
    
    child = creatRtpNode(fd, &_this->m_pub, _this->m_director);
    if (NULL != child) {
        child->m_base.m_node_type = ENUM_NODE_RTP;
        
        _this->m_director->registTask(&child->m_base, EVENT_TYPE_ALL);
    }
    
    return &child->m_base;
}

METHOD(RtspNode, genRtcp, NodeBase*, RtspNodePriv* _this, Int32 fd) {
    RtpNode* child = NULL;
    
    child = creatRtpNode(fd, &_this->m_pub, _this->m_director);
    if (NULL != child) {
        child->m_base.m_node_type = ENUM_NODE_RTCP;
        
        _this->m_director->registTask(&child->m_base, EVENT_TYPE_ALL);
    }
    
    return &child->m_base;
}

RtspNode* creatRtspNode(Int32 fd, Director* director) {
    RtspNodePriv* _this = NULL;

    I_NEW(RtspNodePriv, _this);
    CacheCenter::zero(_this, sizeof(RtspNodePriv));

    ObjCenter::initNode(&_this->m_pub.m_base); 
    
    initRtsp(&_this->m_rtsp);

    _this->m_pub.m_base.m_node_type = ENUM_NODE_RTSP;
    _this->m_rtsp.m_fd = fd;
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
    _this->m_pub.m_base.destroy = _destroy;

    _this->m_pub.onAccept = _onAccept;
    _this->m_pub.onConnOk = _onConnOk;
    _this->m_pub.onConnFail = _onConnFail;
    
    _this->m_pub.recv = _recv;
    _this->m_pub.sendData = _sendData;
    _this->m_pub.genRtp = _genRtp;
    _this->m_pub.genRtcp = _genRtcp;
  
    return &_this->m_pub;
}


static Void acceptRtsp(Int32 listener_fd, Director* director) { 
    Int32 ret = 0;
    Int32 newfd = -1;
    int peer_port = 0;
    Char peer_ip[DEF_IP_SIZE];
    RtspNode* node = NULL;

    newfd = acceptCli(listener_fd);
    while (0 <= newfd) { 
        getPeerInfo(newfd, &peer_port, peer_ip, DEF_IP_SIZE);
        
        node = creatRtspNode(newfd, director);
        if (NULL != node) {
            ret = node->onAccept(node);
            if (0 == ret) {
                LOG_DEBUG("rtsp_accept| fd=%d| addr=%s:%d| msg=ok|",
                    newfd, peer_ip, peer_port);
                
                director->registTask(&node->m_base, EVENT_TYPE_ALL);
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

        newfd = acceptCli(listener_fd);
    }
}


METHOD(NodeBase, rtsp_getFd, Int32, RtspListenerPriv* _this) {
    return _this->m_listener_fd;
}

METHOD(NodeBase, rtsp_readNode, EnumSockRet, RtspListenerPriv* _this) {
    acceptRtsp(_this->m_listener_fd, _this->m_director);

    return ENUM_SOCK_MARK_FINISH;
}


METHOD(NodeBase, rtsp_destroy, Void, RtspListenerPriv* _this) {
    
    ObjCenter::finishNode(&_this->m_pub.m_base); 

    closeHd(_this->m_listener_fd);
    I_FREE(_this);
}

ListenerNode* creatRtspListener(Int32 fd, Director* director) {
    RtspListenerPriv* _this = NULL;

    I_NEW(RtspListenerPriv, _this);

    ObjCenter::initNode(&_this->m_pub.m_base);

    _this->m_pub.m_base.m_node_type = ENUM_NODE_LISTENER;
    _this->m_director = director;
    _this->m_listener_fd = fd; 

    director->condition(&_this->m_pub.m_base.m_rcv_task, BIT_EVENT_READ);
    
    _this->m_pub.m_base.getFd = _rtsp_getFd;
    _this->m_pub.m_base.readNode = _rtsp_readNode;
    _this->m_pub.m_base.destroy = _rtsp_destroy;

    return &_this->m_pub;
}

Int32 creatRtpPair(const Char* ip, Int32 port_base, Int32 fds[]) {
    Int32 ret = 0;
    Int32 fd = -1;
    
    ret = UdpUtil::creatUniSvc(port_base, ip, &fd);
    if (0 == ret) {
        ret = UdpUtil::creatUniSvc(port_base + 1, ip, 
            &fds[ENUM_RTSP_STREAM_RTCP]);

        if (0 == ret) {
            fds[ENUM_RTSP_STREAM_RTP] = fd;
            
            return 0;
        } else {
            CommSock::closeHd(fd);

            return -1;
        }
    } else {
        return -1;
    } 
}

