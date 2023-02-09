#ifndef __DIRECTOR_H__
#define __DIRECTOR_H__
#include"globaltype.h"
#include"taskpool.h"
#include"datatype.h"
#include"ticktimer.h"


struct CacheHdr;
struct NodeBase;
struct CtrlNode;
struct TimerNode;
struct TimerEle;

class Lock;
class ObjCenter;
class Sender;
class Dealer;
class Receiver;

enum EnumListenerType {
    ENUM_LISTENER_NULL = 0,

    ENUM_LISTENER_SOCK,
    ENUM_LISTENER_RTMP,
    ENUM_LISTENER_RTSP,

    ENUM_LISTENER_END
};

class Director : public TaskWorker {
public:
    explicit Director(int capacity);
    virtual ~Director();
    
    virtual int init();
    virtual void finish(); 

    const Char* getThrName(EnumCtrlType type) const;
    
    inline ObjCenter* getCenter() {
        return m_center;
    }

    virtual int start();
    virtual void join();
    virtual void stop();

    virtual Bool lock(NodeBase* base);
    virtual Bool unlock(NodeBase* base);

    template<EnumCtrlType type>
    Void procTimeout(struct TimerEle* ele);
    
    Int32 dispatch(NodeBase* node, CacheHdr* msg);
    Int32 send(NodeBase* node, CacheHdr* hdr);
    Int32 direct(NodeBase* node, CacheHdr* hdr);

    Int32 dispatchExch(NodeBase* node, Int32 cmd, Int64 val);
    Int32 directExch(NodeBase* node, Int32 cmd, Int64 val);
    
    Int32 readTimer(Uint32 val);
    
    Void dealCtrlRcv(CacheHdr* hdr);
    Void dealCtrlSnd(CacheHdr* hdr);
    
    void addTimer(EnumCtrlType ctype, struct TimerEle* ele, 
        Int32 type, Uint32 interval); 

    void stopTimer(EnumCtrlType ctype);

    Void procTimer(EnumCtrlType type, CacheHdr* hdr);

    Void registTask(NodeBase* node, Int32 ev);
    Void unregTask(NodeBase* node, Int32 errcode);

    Int32 addListener(EnumListenerType type, const Char szip[], Int32 port);
    
protected:
    virtual int setup();
    virtual void teardown();

    virtual void procTaskEnd(struct Task* task);
    virtual unsigned int procTask(struct Task* task); 

private:
    Void procNodeQue(NodeBase* node);
    Void dealCmd(NodeBase* node, CacheHdr* hdr);
    Void dealSysMsg(NodeBase* node, CacheHdr* hdr);
    Void procStopCmd(NodeBase* node, CacheHdr* hdr);
    Void procUnregCmd(NodeBase* node, CacheHdr* hdr);
    Void procRegistCmd(NodeBase* node, CacheHdr* hdr);
    
    Int32 recv(CacheHdr* hdr); 

    /* this function can only recv hdr to ctrlnode */
    Int32 recvExch(Int32 cmd, Int64 val);

    /* this function can only send hdr to timernode or ctrlnode */
    Int32 sendExch(NodeBase* node, Int32 cmd, Int64 val);
    
    CacheHdr* genExchMsg(Int32 cmd, Int64 val);
    Int64 getExchVal(CacheHdr* hdr);

    Void addQue(NodeBase* node);
    Void delQue(NodeBase* node);
    Void freeNodes();

private:
    const int m_capacity;

    list_head m_cmd_list;
    
    Lock* m_lock;
    ObjCenter* m_center;
    Dealer* m_dealer;
    Sender* m_sender;
    Receiver* m_recver;

    CtrlNode* m_ctrl_node;
    TimerNode* m_timer_node;
    Bool m_is_ready;

    struct TimerEle m_timer_ele[CTRL_TYPE_END];
};


#endif

