#ifndef __MISC_H__
#define __MISC_H__
#include"sysoper.h"


enum EnumLogLevel {
    ENUM_LOG_ERR = 0,
    ENUM_LOG_WARN,
    ENUM_LOG_INFO,
    ENUM_LOG_DEBUG,
    ENUM_LOG_VERB,
    
    MAX_LOG_LEVEL
};

#define LOG_VERB(format,args...)  formatLog(ENUM_LOG_VERB, format, ##args)
#define LOG_DEBUG(format,args...)  formatLog(ENUM_LOG_DEBUG, format, ##args)
#define LOG_INFO(format,args...)  formatLog(ENUM_LOG_INFO, format, ##args)
#define LOG_WARN(format,args...)  formatLog(ENUM_LOG_WARN, format, ##args)
#define LOG_ERROR(format,args...)  formatLog(ENUM_LOG_ERR, format, ##args)

extern Void initLib();
extern Void finishLib();

extern Void formatLog(int level, const char format[], ...);
extern Void setLogLevel(Int32 level);
extern Void setLogStdin(Bool yes);

Int32 getTid();
Void maskSig(int sig);
Void armSig(int sig, void (*)(int));
Void sleepSec(int sec);
Void getRand(void* buf, int len);
Void sysPause();
Uint32 sysRand();

Uint32 getSysTime();

Uint64 getMonoTimeMs();

static const Int32 DEF_TIME_FORMAT_SIZE = 64;

Int32 formatTime(Uint32 ts, const Char* format, Bool bUtc, 
    Char (&out)[DEF_TIME_FORMAT_SIZE]); 

#endif

