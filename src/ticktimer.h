#ifndef __TICKTIMER_H__
#define __TICKTIMER_H__
#include"sysoper.h"
#include"listnode.h"


#define TV_1_BITS 8
#define TV_1_SIZE (1 << TV_1_BITS)
#define TV_1_MASK (TV_1_SIZE-1)
#define TV_2_BITS 8
#define TV_2_SIZE (1 << TV_2_BITS)
#define TV_2_MASK (TV_2_SIZE-1)


class TickTimer;

struct TimerEle {
    hlist_node m_node;
    TickTimer* m_base;
    Uint32 m_expires;
    Uint32 m_interval; 
    Int32 m_type;
};

class TickTimer { 
public:
    TickTimer();
    virtual ~TickTimer(); 

    Void stop();
    
    inline Uint32 monoTick() const {
        return m_tick;
    }

    inline Uint32 now() const {
        return m_time;
    }

    Void addTimer(struct TimerEle* ele);

    Void delTimer(struct TimerEle* ele);

    Void modTimer(struct TimerEle* ele, Uint32 timeout);

    void tick(Uint32 cnt);

protected:
    virtual Void doTimeout(struct TimerEle* ele) = 0;

private:
    void doTimer(struct TimerEle* ele);

private:
    Uint32 m_size;
    Uint32 m_tick;
    Uint32 m_time;
    hlist_head m_tv1[TV_1_SIZE];
    hlist_head m_tv2[TV_2_SIZE];
};


extern void INIT_TIMER_ELE(struct TimerEle* ele);

extern void updateTimer(struct TimerEle* ele);

extern void delTimer(struct TimerEle* ele);

#endif

