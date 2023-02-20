#include"objcenter.h"
#include"cache.h"
#include"sockutil.h"
#include"payloads/outputstream.h"
#include"payloads/inputstream.h"
#include"payloads/invokepld.h"
#include"rtp/rtspcenter.h"
#include"sock/udputil.h"


Void ObjCenter::initNode(NodeBase* node, Int32 node_type) {
    memset(node, 0, sizeof(NodeBase));

    node->m_node_type = node_type;
    
    INIT_LIST_NODE(&node->m_rcv_node);
    INIT_LIST_NODE(&node->m_snd_node);
    INIT_LIST_NODE(&node->m_cmd_node);
    
    INIT_LIST_HEAD(&node->m_snd_que_tmp);
    INIT_LIST_HEAD(&node->m_snd_que);
    INIT_LIST_HEAD(&node->m_deal_que_tmp);
    INIT_LIST_HEAD(&node->m_deal_que);
    INIT_LIST_HEAD(&node->m_cmd_que_tmp);
    INIT_LIST_HEAD(&node->m_cmd_que);

    INIT_TASK(&node->m_rcv_task);
    INIT_TASK(&node->m_snd_task);
    INIT_TASK(&node->m_deal_task);
    INIT_TASK(&node->m_cmd_task); 
}

Void ObjCenter::finishNode(NodeBase* node) {
    INIT_LIST_NODE(&node->m_rcv_node);
    INIT_LIST_NODE(&node->m_snd_node);
    INIT_LIST_NODE(&node->m_cmd_node);

    freeMsgQue(&node->m_snd_que_tmp);
    freeMsgQue(&node->m_snd_que);
    freeMsgQue(&node->m_deal_que_tmp);
    freeMsgQue(&node->m_deal_que);
    freeMsgQue(&node->m_cmd_que_tmp);
    freeMsgQue(&node->m_cmd_que);

    INIT_TASK(&node->m_rcv_task);
    INIT_TASK(&node->m_snd_task);
    INIT_TASK(&node->m_deal_task);
    INIT_TASK(&node->m_cmd_task);
}

Void ObjCenter::freeMsgQue(list_head* list) { 
    list_node* pos = NULL;
    list_node* n = NULL;
    CacheHdr* hdr = NULL;

    if (!list_empty(list)) {
        list_for_each_safe(pos, n, list) {
            list_del(pos, list);

            hdr = CacheCenter::cast(pos);
            
            CacheCenter::free(hdr);
        }
    }
}

ObjCenter::ObjCenter() {
    m_output = NULL;
    m_input = NULL;
    m_handler = NULL;
    m_rtsp_handler = NULL;
}

ObjCenter::~ObjCenter() {
}

Int32 ObjCenter::init() {
    Int32 ret = 0;

    I_NEW(MsgOutput, m_output);
    I_NEW(RtmpInput, m_input);
    
    m_handler = RtmpHandler::creat();
    if (NULL == m_handler) {
        return -1;
    }

    m_rtsp_handler = RtspHandler::creat();
    if (NULL == m_rtsp_handler) {
        return -1;
    }
    
    return ret;
}

Void ObjCenter::finish() {
    I_FREE(m_output);
    I_FREE(m_input);
    
    if (NULL != m_handler) {
        RtmpHandler::free(m_handler);
        m_handler = NULL;
    }

    if (NULL != m_rtsp_handler) {
        RtspHandler::freeHd(m_rtsp_handler);
        m_rtsp_handler = NULL;
    }
}

EnumSockRet ObjCenter::readSock(NodeBase* node, callback cb) {
    Int32 ret = 0;
    int rdlen = 0;
    Int32 fd = node->getFd(node); 

    rdlen = recvTcp(fd, m_buf, DEF_TCP_RCV_BUF_SIZE);
    if (0 < rdlen) { 
        ret = cb(node, m_buf, rdlen);
        if (0 == ret) {
            if (rdlen < DEF_TCP_RCV_BUF_SIZE) {
                return ENUM_SOCK_MARK_FINISH;
            } else {
                return ENUM_SOCK_MARK_PARTIAL;
            }
        } else {
            LOG_ERROR("read_sock| ret=%d| fd=%d| rdlen=%d|"
                " msg=parse read error|",
                ret, fd, rdlen);
            
            return ENUM_SOCK_MARK_ERR;
        }
    } else if (0 == rdlen) {
        return ENUM_SOCK_MARK_FINISH;
    } else {
        return ENUM_SOCK_MARK_ERR;
    }
}

EnumSockRet ObjCenter::writeSock(NodeBase* node) {
    EnumSockRet ret = ENUM_SOCK_MARK_FINISH;

    ret = m_output->writeMsg(node);

    return ret;
}

Int32 ObjCenter::parseRtmp(Rtmp* rtmp, Byte* data, Int32 len) {
    Int32 ret = 0;

    ret = m_input->parseMsg(rtmp, data, len);
    return ret;
}

EnumSockRet ObjCenter::readUdp(NodeBase* node, callback2 cb) {
    Int32 ret = 0;
    int rdlen = 0;
    Int32 fd = node->getFd(node); 
    AddrInfo addr;

    rdlen = UdpUtil::recvFrom(fd, m_buf, DEF_TCP_RCV_BUF_SIZE, &addr);
    if (0 < rdlen) { 
        ret = cb(node, m_buf, rdlen, &addr);
        if (0 == ret) {
            if (rdlen < DEF_TCP_RCV_BUF_SIZE) {
                return ENUM_SOCK_MARK_FINISH;
            } else {
                return ENUM_SOCK_MARK_PARTIAL;
            }
        } else {
            LOG_ERROR("read_udp| ret=%d| fd=%d| rdlen=%d|"
                " msg=parse read error|",
                ret, fd, rdlen);
            
            return ENUM_SOCK_MARK_ERR;
        }
    } else if (0 == rdlen) {
        return ENUM_SOCK_MARK_FINISH;
    } else {
        return ENUM_SOCK_MARK_ERR;
    }
}

