#include<poll.h>
#include"sender.h"
#include"director.h"
#include"objcenter.h"
#include"msgtype.h"
#include"sockutil.h"


Sender::Sender(Int32 capacity, Director* director) 
    : m_capacity(capacity), m_director(director) {
    m_fds = NULL;
    m_nodes = NULL;

    INIT_LIST_HEAD(&m_run_list);

    m_event_fd = -1;
}

Sender::~Sender() {
}

int Sender::init() {
    int ret = 0;
    
    ret = TaskThread::init();
    if (0 != ret) {
        return ret;
    }

    setName(m_director->getThrName(CTRL_TYPE_SND));

    m_event_fd = creatEventFd();
    if (0 > m_event_fd) {
        return -1;
    }

    ARR_NEW(struct pollfd, m_capacity, m_fds);
    memset(m_fds, 0, sizeof(struct pollfd) * m_capacity); 

    ARR_NEW(NodeBase*, m_capacity, m_nodes);
    memset(m_nodes, 0, sizeof(NodeBase*) * m_capacity); 

    return 0;
}

void Sender::finish() { 
    if (NULL != m_fds) { 
        ARR_FREE(m_fds);
    }

    if (NULL != m_nodes) { 
        ARR_FREE(m_nodes);
    }

    if (0 <= m_event_fd) {
        closeHd(m_event_fd);

        m_event_fd = -1;
    }
    
    TaskThread::finish();
}

int Sender::setup() { 
    return 0;
}

void Sender::teardown() {
    m_director->stopTimer(CTRL_TYPE_SND);

    clearRunlist();
    
    /* if exit, then stop all threads */
    m_director->stop();
}

void Sender::wait() { 
    waitEvent(DEF_POLL_WAIT_MILLISEC);
} 

Void Sender::alarm() {
    writeEvent(m_event_fd);
}

Void Sender::addNode(NodeBase* node) { 
    list_add_back(&node->m_snd_node, &m_run_list); 
}

Void Sender::delNode(NodeBase* node) {
    list_del(&node->m_snd_node, &m_run_list);
}

Void Sender::closeTask(NodeBase* node, Int32 errcode) {
    node->m_can_wr = FALSE;
    
    delNode(node);
    m_director->unregTask(node, errcode);
}

Int32 Sender::waitEvent(Int32 timeout) { 
    static const Int32 ERR_FLAG = POLLERR | POLLHUP | POLLNVAL; 
    list_node* pos = NULL;
    struct pollfd* pfd = NULL;
    NodeBase* node = NULL;
    Uint32 val = 0;
    Int32 cnt = 0;
    Int32 ret = 0;

    /* add event fd */
    m_fds[0].fd = m_event_fd;
    m_fds[0].events = POLLIN; 
    m_fds[0].revents = 0;
    ++cnt;
    
    list_for_each(pos, &m_run_list) {
        node = list_entry(pos, NodeBase, m_snd_node);

        if (!node->m_can_wr) { 
            pfd = &m_fds[cnt]; 
            m_nodes[cnt] = node; 
            ++cnt; 
            
            pfd->fd = node->getFd(node);
            pfd->events = POLLOUT; 
            pfd->revents = 0;
        } else {
            continue;
        }
    }

    ret = poll(m_fds, cnt, timeout);
    if (0 < ret) {  
        /* check event fd */
        if (POLLIN & m_fds[0].revents) {
            readEvent(m_event_fd, &val);
            
            --ret;
        } 
        
        for (int i=1; i<cnt && 0<ret; ++i) {
            pfd = &m_fds[i]; 
            node = m_nodes[i]; 

            if (POLLOUT & pfd->revents) { 
                node->m_can_wr = TRUE;
                
                addTask(&node->m_snd_task, BIT_EVENT_WRITE);

                --ret;
            } else if (ERR_FLAG & pfd->revents) { 
                LOG_INFO("chk_event_poll| fd=%d| revent=0x%x|"
                    " error=poll error|",
                    pfd->fd, pfd->revents); 

                closeTask(node, ENUM_ERR_SOCK_CHK_IO);

                --ret;
            } else {
                continue;
            }
        }
    } else if (0 == ret || EINTR == errno) {
        ret = 0;
    } else {
        ret = -1;
    }

    return ret;
}

unsigned int Sender::procTask(struct Task* task) {
    Uint32 ret = BIT_EVENT_NORM;
    NodeBase* node = NULL;

    node = list_entry(task, NodeBase, m_snd_task);

    if (node->m_can_wr) {
        ret = writeNode(node);
    } else {
        ret = BIT_EVENT_WRITE;
    }
    
    return ret;
}

void Sender::procTaskEnd(struct Task* task) {
    NodeBase* node = NULL;

    node = list_entry(task, NodeBase, m_snd_task); 

    if (node->m_can_wr) {
        /* if can write, then write the data in queue first */
        writeNode(node); 
    }
    
    delNode(node);
    m_director->directExch(node, ENUM_MSG_TYPE_STOP, ENUM_FD_STOP_SND);
}

Uint32 Sender::writeNode(NodeBase* node) {
    Bool bOk = FALSE;
    EnumSockRet ret = ENUM_SOCK_MARK_FINISH; 

    bOk = m_director->lock(node);
    if (bOk) {
        if (!list_empty(&node->m_snd_que_tmp)) {
            list_splice_back(&node->m_snd_que_tmp, &node->m_snd_que);
        }
        
        m_director->unlock(node);
    } 

    ret = node->writeNode(node);
    if (ENUM_SOCK_MARK_FINISH == ret) {
        /* write all of data completely */
        return BIT_EVENT_NORM;
    } else if (ENUM_SOCK_MARK_PARTIAL == ret) {
        /* write partial and would continue for next round */
        return BIT_EVENT_IN;
    } else if (ENUM_SOCK_MARK_BLOCK == ret) {
        /* write blocked */
        node->m_can_wr = FALSE;
        return BIT_EVENT_WRITE;
    } else {
        /* write error */
        closeTask(node, ENUM_ERR_SOCK_DATA_WR);
        
        return BIT_EVENT_ERROR;
    }
}

Void Sender::clearRunlist() {
    list_node* pos = NULL;
    list_node* n = NULL;
    list_head* list = &m_run_list;

    if (!list_empty(list)) {
        list_for_each_safe(pos, n, list) {
            list_del(pos, list);
        }
    }
}
