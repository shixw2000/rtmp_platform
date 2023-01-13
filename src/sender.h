#ifndef __SENDER_H__
#define __SENDER_H__
#include"taskpool.h"
#include"listnode.h"
#include"objcenter.h"


struct pollfd;
struct NodeBase;
class Director;

class Sender : public TaskThread {
public:
    Sender(Int32 capacity, Director* director);
    virtual ~Sender();
    
    virtual int init();
    virtual void finish(); 

    Void addNode(NodeBase* node);
    
protected: 
    virtual void wait();
    virtual void alarm(); 

    virtual int setup();
    virtual void teardown();
    
    virtual void procTaskEnd(struct Task* task);
    virtual unsigned int procTask(struct Task* task);

private: 
    Int32 waitEvent(Int32 timeout);
    void closeTask(NodeBase* node, Int32 errcode);

    Uint32 writeNode(NodeBase* node); 

    Void delNode(NodeBase* node);

    Void clearRunlist();

private:
    const Int32 m_capacity;
    
    Director* m_director;
    struct pollfd* m_fds;
    NodeBase** m_nodes;
    list_head m_run_list;
    Int32 m_event_fd;
};

#endif

