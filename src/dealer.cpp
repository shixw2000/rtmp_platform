#include"dealer.h"
#include"director.h"
#include"datatype.h"
#include"cache.h"
#include"msgtype.h"


Dealer::Dealer(Director* director) : m_director(director) {
}

Dealer::~Dealer() {
}

Int32 Dealer::init() {
    Int32 ret = 0;

    ret = TaskWorker::init();
    if (0 != ret) {
        return ret;
    }

    setName(m_director->getThrName(CTRL_TYPE_DEAL));

    return ret;
}

Void Dealer::finish() {
    TaskWorker::finish();
} 

int Dealer::setup() { 
    return 0;
}

void Dealer::teardown() {
    m_director->stopTimer(CTRL_TYPE_DEAL);
    
    /* if exit, then stop all threads */
    m_director->stop();
}

Void Dealer::closeTask(NodeBase* node, Int32 errcode) {
    if (!node->m_stop_deal) {
        node->m_stop_deal = TRUE;
        
        m_director->unregTask(node, errcode);
    }
}

unsigned int Dealer::procTask(struct Task* task) {
    NodeBase* node = NULL;

    node = list_entry(task, NodeBase, m_deal_task);

    dealNode(node);
    
    return BIT_EVENT_NORM; 
}

void Dealer::procTaskEnd(struct Task* task) {
    NodeBase* node = NULL;

    node = list_entry(task, NodeBase, m_deal_task);

    LOG_INFO("proc_task_end| fd=%d| name=dealer|",
        node->getFd(node));

    m_director->directExch(node, ENUM_MSG_TYPE_STOP, ENUM_FD_STOP_DEAL);
    
    return;
}

Void Dealer::dealNode(NodeBase* node) { 
    Bool bOk = TRUE;
    list_head* list = NULL;
    list_node* pos = NULL;
    list_node* n = NULL;
    CacheHdr* hdr = NULL;

    bOk = m_director->lock(node);
    if (bOk) {
        if (!list_empty(&node->m_deal_que_tmp)) {
            list_splice_back(&node->m_deal_que_tmp, &node->m_deal_que);
        }
        
        m_director->unlock(node);
    }

    list = &node->m_deal_que;
    if (!list_empty(list)) {
        list_for_each_safe(pos, n, list) {
            list_del(pos, list);

            hdr = CacheCenter::cast(pos);
            procMsg(node, hdr);
        }
    }
}

Void Dealer::procMsg(NodeBase* node, CacheHdr* hdr) {
    Int32 ret = 0;

    if (MsgCenter::chkSysMsg(hdr)) {
        procSysMsg(node, hdr);
    } else if (!node->m_stop_deal) {
        ret = node->dealMsg(node, hdr);
        if (0 != ret) {
            closeTask(node, ENUM_ERR_SOCK_DATA_DEAL);
        }
    } else {
        CacheCenter::free(hdr);
    }
}

Void Dealer::procSysMsg(NodeBase* node, CacheHdr* hdr) {  
    Int32 type = CacheCenter::getType(hdr);
    
    if (ENUM_MSG_TIMER_TICK == type) {
        m_director->procTimer(CTRL_TYPE_DEAL, hdr);
    } else if (ENUM_MSG_TYPE_STOP == type) {
        procStopCmd(node, hdr);
    } else { 
        CacheCenter::free(hdr);
    }
}

Void Dealer::procStopCmd(NodeBase* node, CacheHdr* hdr) {   
    if (!node->m_stop_deal) {
        node->m_stop_deal = TRUE;
    }
    
    node->onClose(node); 
    endTask(&node->m_deal_task); 
    
    CacheCenter::free(hdr);
}

