#include"rtpnode.h"
#include"rtspnode.h"
#include"rtpmsg.h"
#include"rtspmsg.h"
#include"objcenter.h"
#include"director.h"
#include"sock/udputil.h"
#include"cache.h"
#include"msgtype.h"
#include"rtpdec.h"
#include"sock/commsock.h" 
#include"rtspcenter.h"
#include"rtpdec.h"


struct RtpNodePriv {
    RtpNode m_pub;

    Director* m_director;
    ObjCenter* m_center;
    RtspHandler* m_handler;
    NodeBase* m_parent;
    Int32 m_fd;
};

static Int32 parse(NodeBase* node, Byte* data, Int32 len, 
    const AddrInfo* addr) {
    Int32 ret = 0;
    Cache* cache = NULL;
    CacheHdr* hdr = NULL;
    RtpNodePriv* _this = (RtpNodePriv*)node;

    cache = RtpTools::creatRtpPkg(addr, data, len);
   
    hdr = CacheCenter::creat(ENUM_MSG_RTP_DATA_ORIGIN, 0); 
    CacheCenter::setCache(hdr, cache); 
    
    ret = _this->m_director->dispatch(&_this->m_pub.m_base, hdr); 
    return ret;
}

METHOD(NodeBase, getFd, Int32, RtpNodePriv* _this) {
    return _this->m_fd;
}

METHOD(NodeBase, readNode, EnumSockRet, RtpNodePriv* _this) {
	EnumSockRet ret = ENUM_SOCK_MARK_FINISH;

    ret = _this->m_center->readUdp(&_this->m_pub.m_base, &parse);

    return ret;
}

METHOD(NodeBase, writeNode, EnumSockRet, RtpNodePriv* _this) {
    EnumSockRet ret = ENUM_SOCK_MARK_FINISH;

    ret = RtpTools::writeUdpNode(&_this->m_pub.m_base); 
    return ret;
}

METHOD(NodeBase, dealMsg, Int32, RtpNodePriv* _this, CacheHdr* hdr) {
    Int32 ret = 0;
    Cache* cache = NULL;
    RtpPkg* pkg = NULL;
    IpInfo originIP;
    RtpDecoder decoder;

    cache = CacheCenter::getCache(hdr);
    pkg = MsgCenter::cast<RtpPkg>(cache); 

    CommSock::addr2IP(&pkg->m_origin, &originIP);     
    
    LOG_DEBUG("rtp_deal_msg| ip=%s| port=%d| data_len=%d|",
        originIP.m_ip, originIP.m_port, pkg->m_length);
    
    if (ENUM_NODE_RTP == _this->m_pub.m_base.m_node_type) {
        ret = decoder.parseRtp(pkg);
    } else {
        ret = decoder.parseRtcp(pkg);
    }
    
    CacheCenter::free(hdr);
    return ret;
}

METHOD(NodeBase, dealCmd, Void, RtpNodePriv* , CacheHdr* ) {
    
    return;
}

METHOD(NodeBase, onClose, Void, RtpNodePriv* _this) {
    _this->m_director->signalChildExit(_this->m_parent, &_this->m_pub.m_base); 
}

METHOD(NodeBase, destroy, Void, RtpNodePriv* _this) { 
    if (0 <= _this->m_fd) {
        CommSock::closeHd(_this->m_fd);
        _this->m_fd = -1; 
    } 
    
    ObjCenter::finishNode(&_this->m_pub.m_base); 
    
    I_FREE(_this);
}

METHOD(RtpNode, recv, Int32, RtpNodePriv* _this, CacheHdr* hdr) {
    Int32 ret = 0;

    ret = _this->m_director->dispatch(&_this->m_pub.m_base, hdr); 
    return ret;
}

RtpNode* creatRtpNode(Int32 fd, Int32 node_type, 
    NodeBase* parent, Director* director) {
    RtpNodePriv* _this = NULL;

    I_NEW(RtpNodePriv, _this);
    CacheCenter::zero(_this, sizeof(RtpNodePriv));

    ObjCenter::initNode(&_this->m_pub.m_base, node_type); 
        
    _this->m_fd = fd;
        
    _this->m_parent = parent;
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
    
    _this->m_pub.recv = _recv;
  
    return &_this->m_pub;
}


