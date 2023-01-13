#ifndef __TASKER_H__
#define __TASKER_H__
#include"sysoper.h"
#include"listnode.h"


class BitTool {
public:
    static unsigned int addBit(unsigned int* pstate, unsigned int n); // |= n
    
    static unsigned int setBit(unsigned int* pstate, unsigned int n); // = n 
    
    static bool casBit(unsigned int* pstate, 
        unsigned int newval, unsigned int oldval); 

    static bool isMember(unsigned int state, unsigned int n);
};

class BinaryDoor {
    static const unsigned int DEF_LOCK_FLAG = 0x1;
    static const unsigned int DEF_LOGIN_FLAG = (0x1 << 1); 
    
public:
    BinaryDoor();
    
    /* used befor action */
    void lock();

    /* return true if it is the first one */
    bool login();

    /* return true if it is the last one */
    bool release();

private:
    unsigned int m_state;
};


class Service {
public:
    Service();
    virtual ~Service();
    
    void startSvc();
    
    void stopSvc(); 

    void resume(); 

    bool isRunning() const {
        return m_running;
    }

protected: 
    virtual bool doService() = 0; 

    virtual void wait() = 0;
    virtual void alarm() = 0;

private:
    BinaryDoor m_door;
    volatile bool m_running;
};


enum BIT_EVENT_TYPE { 
    BIT_EVENT_LOCK = 0, // just used by intern system, but should never used by user
    
    BIT_EVENT_NORM, // if checkin, then normal, if checkout, as no requirements for check
    
    BIT_EVENT_CUSTOM, 
    BIT_EVENT_READ,
    BIT_EVENT_WRITE,
    BIT_EVENT_DEAL,
    BIT_EVENT_FLOW_CTL,

    BIT_EVENT_COND_APPROVAL,
    BIT_EVENT_ERROR,

    BIT_EVENT_END = 15, // notify eof of the task, just used by intern
    
    BIT_EVENT_MAX = 16, // the max slot
    BIT_EVENT_IN,       // used only by checkout if keep in queue
};

class BitDoor {
public:
    BitDoor();

    bool isEof(unsigned int state);
        
    unsigned int lock(unsigned int* pstate);

    bool release(unsigned int* pstate);

    bool checkin(unsigned int* pstate, unsigned int ev);

    bool checkout(unsigned int* pstate, unsigned int ev);

private:
    unsigned int m_low_bits[BIT_EVENT_MAX];
    unsigned int m_high_bits[BIT_EVENT_MAX];
    unsigned int m_mask_bits[BIT_EVENT_MAX];
}; 


class Pooler {
public:
    Pooler();
    virtual ~Pooler();
  
    bool consume();

    bool endTask(struct Task* task);
    bool addTask(struct Task* task, unsigned int ev); 
    
    Void condition(struct Task* task, unsigned int ev); 

protected: 
    virtual Bool lock() = 0;
    virtual Bool unlock() = 0;
    virtual void procTaskEnd(struct Task* task) = 0;
    virtual unsigned int procTask(struct Task* task) = 0;

private:
    bool _add(struct Task* task, unsigned int ev); 
    void _splice();
    unsigned int doTask(struct Task* task, unsigned int oldstate); 

private: 
    struct root m_node_list;
    struct root m_tmp_list;
    BitDoor m_door;
};


class Tasker : public Service, public Pooler {
public:     
    bool endTask(struct Task* task);
    bool addTask(struct Task* task, unsigned int ev); 

protected:    
    virtual bool doService();
};

#endif

