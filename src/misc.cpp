#include<unistd.h>
#include<sys/syscall.h>
#include<sys/types.h>
#include<signal.h>
#include<pthread.h>
#include<sys/types.h>
#include<sys/stat.h>
#include<fcntl.h>
#include<time.h>
#include<stdlib.h>
#include<regex.h>
#include<stdarg.h>
#include"misc.h"


struct UsrTime {
    Uint64 m_sec:54,
        m_msec:10;
};

static UsrTime _getMonoTime(); 

static const Char DEF_BUILD_VER[] = __BUILD_VER__;
static const Int32 MAX_LOG_FILE_SIZE = 0x2000000;
static const Int32 MAX_LOG_FILE_CNT = 3;
static const Int32 MAX_LOG_NAME_SIZE = 32;
static const Int32 MAX_LOG_CACHE_SIZE = 0x100000;
static const Int32 MAX_CACHE_INDEX = 8;

static const Char* DEF_LOG_DESC[MAX_LOG_LEVEL] = {
    "ERROR", "WARN", "INFO", "DEBUG", "VERB" 
};

static const Char DEF_LOG_LINK_NAME[] = "agent.log";
static const Char DEF_NULL_FILE[] = "/dev/null";
static const Char DEF_LOG_NAME_PATTERN[] = "^agent_([[:digit:]]{3}).log$";


struct SysParam {
    FILE* m_log_hd;
    Uint32 m_epoch; // time from 1970 in second
    Uint32 m_diff_ts; // clock time differ from epoch
    Int32 m_log_level;
    Bool m_log_stdin;
    Char m_log_buf[MAX_CACHE_INDEX][MAX_LOG_CACHE_SIZE];
    Byte m_log_buf_index;
    Byte m_cur_log_index;
}; 

static SysParam g_sys_param;

int getTid() {
    int tid = 0;

    tid = syscall(SYS_gettid);
    return tid;
}

void sleepSec(int sec) {
    sleep(sec);
}

void maskSig(int sig) {
    sigset_t sets;

    sigemptyset(&sets);
    sigaddset(&sets, sig);

    pthread_sigmask(SIG_BLOCK, &sets, NULL);
}

void armSig(int sig, void (*fn)(int)) {
    struct sigaction act;

    memset(&act, 0, sizeof(act));
    sigemptyset(&act.sa_mask);

    act.sa_flags = SA_RESTART;
    act.sa_handler = fn;

    sigaction(sig, &act, NULL);
}

void getRand(Void* buf, Int32 len) {
    static Uint64 g_rand = 0;
    int fd = -1;
    int cnt = 0;

    if (0 < len) {
        fd = open("/dev/urandom", O_RDONLY|O_NONBLOCK);
        if (0 <= fd) {
            cnt = read(fd, buf, len);
            close(fd);
        }

        if (cnt != len) {
            --g_rand;
            
            memcpy(buf, &g_rand, len);
        }
    }
    
    return;
} 

Uint32 getSysTime() {
    UsrTime usrTm = {0, 0};

    usrTm = _getMonoTime();
    
    usrTm.m_sec += g_sys_param.m_diff_ts;

    return usrTm.m_sec;
}

Uint64 getMonoTimeMs() {
    Uint64 ms = 0; 
    UsrTime usrTm = {0, 0};

    usrTm = _getMonoTime();
    ms = usrTm.m_sec * 1000 + usrTm.m_msec;

    return ms;
}

Int32 formatTime(Uint32 ts, const Char* format, Bool bUtc, 
    Char (&out)[DEF_TIME_FORMAT_SIZE]) {
    Int32 len = 0;
    time_t nTi = ts;
    struct tm oTm;
    struct tm* pTm = NULL;

    if (bUtc) {
        pTm = gmtime_r(&nTi, &oTm);
    } else {
        pTm = localtime_r(&nTi, &oTm);
    }

    if (NULL != pTm) {
        len = strftime(out, DEF_TIME_FORMAT_SIZE, format, pTm);
    } else {
        len = 0;
    }

    out[len] = '\0'; 
    return len;
}

static Void findLogIndex() {
    Int32 ret = 0; 
    int index = 0;
    regex_t reg;
    regmatch_t matchs[2]; 
    Char szFile[MAX_LOG_NAME_SIZE] = {0};
        
    ret = readlink(DEF_LOG_LINK_NAME, szFile, MAX_LOG_NAME_SIZE-1);
    if (0 <= ret) {
        szFile[ret] = '\0';
        
        regcomp(&reg, DEF_LOG_NAME_PATTERN, REG_EXTENDED);
        ret = regexec(&reg, szFile, 2, matchs, 0);
        if (0 == ret) {
            index = atoi(&szFile[ matchs[1].rm_so ]);
        }
    } 

    g_sys_param.m_cur_log_index = index;
}

static Void creatFileLog(const char mode[]) {
    const Char* psz = NULL;
    FILE* f = NULL;
    Int32 ret = 0;
    int index = 0;
    Char szFile[MAX_LOG_NAME_SIZE] = {0};
    
    index = ATOMIC_FETCH_INC(&g_sys_param.m_cur_log_index);
    index %= MAX_LOG_FILE_CNT;
    snprintf(szFile, sizeof(szFile), "agent_%03d.log", index);

    psz = szFile;
    if (NULL != g_sys_param.m_log_hd) {
        f = freopen(psz, mode, g_sys_param.m_log_hd); 
        if (NULL == f) {
            psz = DEF_NULL_FILE;
            freopen(psz, mode, g_sys_param.m_log_hd); 
        }
    } else {
        g_sys_param.m_log_hd = fopen(psz, mode);
        if (NULL == g_sys_param.m_log_hd) {
            psz = DEF_NULL_FILE;
            g_sys_param.m_log_hd = fopen(psz, mode);
        }
    }
    
    ret = unlink(DEF_LOG_LINK_NAME);
    if (0 == ret || ENOENT == errno) {
        symlink(psz, DEF_LOG_LINK_NAME);
    } 
}

static Void statFileLog() {
    static Bool g_log_creating = FALSE;
    Int32 pos = 0; 

    pos = (Int32)ftell(g_sys_param.m_log_hd);
    if (pos < MAX_LOG_FILE_SIZE || g_log_creating) {
        return;
    } else {
        g_log_creating = TRUE;
        
        creatFileLog("wb"); 

        g_log_creating = FALSE;
    }
}

Void setLogLevel(Int32 level) {
    if (MAX_LOG_LEVEL > level && 0 <= level) {
        g_sys_param.m_log_level = level;
    } else {
        g_sys_param.m_log_level = ENUM_LOG_INFO;
    }
}

Void setLogStdin(Bool yes) {
    g_sys_param.m_log_stdin = yes;
}

Void initLog() { 
    g_sys_param.m_log_level = ENUM_LOG_INFO;
    g_sys_param.m_log_stdin = TRUE;
    
    findLogIndex();
    creatFileLog("ab"); 
}

Void finishLog() {
    if (NULL != g_sys_param.m_log_hd) {
        fclose(g_sys_param.m_log_hd);
        g_sys_param.m_log_hd = NULL;
    }
} 

Void formatLog(int level, const char format[], ...) { 
    int maxlen = 0;
    int size = 0;
    int cnt = 0;
    Char* psz = NULL;
    struct tm* result = NULL; 
    struct tm tm;
    time_t t = 0;
    va_list ap;
    Byte index = 0;

    if (level > g_sys_param.m_log_level || NULL == g_sys_param.m_log_hd) {
        return;
    }
    
    t = (time_t)getSysTime();
    size = 0;
    maxlen = MAX_LOG_CACHE_SIZE-1;    
    
    result = localtime_r(&t, &tm); 
    if (NULL != result) { 
        index = ATOMIC_FETCH_INC(&g_sys_param.m_log_buf_index); 
        index = index % MAX_CACHE_INDEX;
        psz = g_sys_param.m_log_buf[index];
        
        cnt = snprintf(&psz[size], maxlen, "[%s]", DEF_LOG_DESC[level]);
        maxlen -= cnt;
        size += cnt;
        
        cnt = strftime(&psz[size], maxlen, "<%Y-%m-%d %H:%M:%S> ", &tm);
        maxlen -= cnt;
        size += cnt;
    
        va_start(ap, format);
        cnt = vsnprintf(&psz[size], maxlen, format, ap);
        va_end(ap); 
        if (0 <= cnt && cnt < maxlen) {
            maxlen -= cnt;
            size += cnt;
        } else if (cnt >= maxlen && 0 < maxlen) {
            cnt= maxlen - 1;
            maxlen -= cnt;
            size += cnt;
        }

        psz[size++] = '\n';

        fwrite(psz, 1, size, g_sys_param.m_log_hd); 
        
        statFileLog(); 

        if (g_sys_param.m_log_stdin) {
            fwrite(psz, 1, size, stdout); 
        }
    }
    
    return;
}

static Void initTime() {
    UsrTime usrTm = {0, 0};

    g_sys_param.m_epoch = (Uint32)time(NULL);

    usrTm = _getMonoTime(); 
    
    g_sys_param.m_diff_ts = g_sys_param.m_epoch - usrTm.m_sec; 
}

Void initLib() { 
    memset(&g_sys_param, 0, sizeof(g_sys_param));
    
    initTime();
    
    srand(g_sys_param.m_epoch);

    initLog(); 
}

Void finishLib() {
    LOG_INFO("prog_stop| version=%s| msg=exit now|", DEF_BUILD_VER);
    
    finishLog();
}

/* format = sec + milisec<16> */
static UsrTime _getMonoTime() {
    UsrTime usrTm = {0, 0};
    Int32 ret = 0;
    struct timespec ts;

    ret = clock_gettime(CLOCK_MONOTONIC_COARSE, &ts);
    if (0 == ret) {
        usrTm.m_sec = ts.tv_sec;
        usrTm.m_msec = ts.tv_nsec / 1000000;
    }

    return usrTm;
}

Void sysPause() {
    pause();
}

Uint32 sysRand() {
    Uint32 n = rand();

    return n;
}

