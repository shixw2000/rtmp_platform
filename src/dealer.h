#ifndef __DEALER_H__
#define __DEALER_H__
#include"taskpool.h"


struct NodeBase;
struct CacheHdr;
class Director;

class Dealer : public TaskWorker {
public:
    explicit Dealer(Director* director);
    ~Dealer();
    
    virtual int init();
    virtual void finish(); 
    
    virtual int setup();
    virtual void teardown();
    
    virtual void procTaskEnd(struct Task* task);
    virtual unsigned int procTask(struct Task* task);

private:
    Void dealNode(NodeBase* node);
    Void procMsg(NodeBase* node, CacheHdr* hdr);

    Void procSysMsg(NodeBase* node, CacheHdr* hdr);

    Void closeTask(NodeBase* node, Int32 errcode);

    Void procStopCmd(NodeBase* node, CacheHdr* hdr);

    Void handleChildExit(NodeBase* node, CacheHdr* hdr);

    Void handleParentAllowExit(NodeBase* node, CacheHdr* hdr);

private:
    Director* m_director;
};

#endif

