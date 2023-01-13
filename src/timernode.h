#ifndef __TIMERNODE_H__
#define __TIMERNODE_H__
#include"globaltype.h"
#include"datatype.h"


enum EnumCtrlType;
struct TimerEle;
class Director;

struct TimerNode {
    NodeBase m_base;
    
    void (*addTimer)(TimerNode*, EnumCtrlType type, TimerEle* ele);
    void (*stopTimer)(TimerNode*, EnumCtrlType type);
    void (*doTick)(TimerNode*, EnumCtrlType type, Uint32 cnt);
};

TimerNode* creatTimerNode(Director* director);

#endif

