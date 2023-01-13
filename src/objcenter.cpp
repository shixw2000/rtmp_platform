#include"objcenter.h"
#include"cache.h"
#include"sockutil.h"
#include"payloads/outputstream.h"
#include"payloads/inputstream.h"
#include"payloads/invokepld.h"


Void ObjCenter::initNode(NodeBase* node) {
    memset(node, 0, sizeof(NodeBase));

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
    
    return ret;
}

Void ObjCenter::finish() {
    I_FREE(m_output);
    I_FREE(m_input);
    
    if (NULL != m_handler) {
        RtmpHandler::free(m_handler);
        m_handler = NULL;
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

Int32 ObjCenter::parseRtmp(RtmpNode* node, Rtmp* rtmp, 
    Byte* data, Int32 len) {
    Int32 ret = 0;

    ret = m_input->parseMsg(node, rtmp, data, len);
    return ret;
}

