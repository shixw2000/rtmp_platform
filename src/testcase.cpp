#include<string.h>
#include<stdlib.h>
#include<stdio.h>
#include<signal.h>
#include<unistd.h>
#include"testcase.h"
#include"datatype.h"
#include"director.h"
#include"config.h"


static const Int32 MAX_FILENAME_PATH_SIZE = 256;
static const Char DEF_AGENT_CONF_PATH[] = "service.conf";

static const Char DEF_SEC_GLOBAL_NAME[] = "global";
static const Char DEF_KEY_PASSWD_NAME[] = "password";
static const Char DEF_SYS_PASSWD[] = "123456";
static const Char DEF_KEY_LOG_LEVEL_NAME[] = "log_level";
static const Char DEF_KEY_LOG_STDIN_NAME[] = "log_stdin";

static const Char DEF_KEY_CHUNK_SIZE_NAME[] = "chunk_size";
static const Char DEF_KEY_WIN_SIZE_NAME[] = "window_size";
static const Char DEF_KEY_PRNT_FLV_NAME[] = "print_flv";
static const Char DEF_KEY_PORT_NAME[] = "rtmp_port";
static const Char DEF_KEY_IP_NAME[] = "rtmp_ip";

static Director* g_director = NULL;
Config g_conf;

static Void sigHandler(Int32 sig) {
    LOG_ERROR("*******sig=%d| msg=stop server|", sig);

    if (NULL != g_director) {
        g_director->stop();
    }
}

void test_offset() {
    Int32 offset = 0;

    offset = offset_of(Chunk, m_size);
    
    LOG_ERROR("test_offset| offset=%d|", offset);
}

Int32 initConf(const Char* path, Config* conf) {
    Int32 ret = 0;
    ConfParser parser;

    ret = parser.parse(path);
    if (0 != ret) {
        return ret;
    }

    ret = parser.getParamStr(DEF_SEC_GLOBAL_NAME, 
        DEF_KEY_PASSWD_NAME, conf->m_passwd,
        MAX_PIN_PASSWD_SIZE);
    if (0 == ret) {
        if (0 != strncmp(DEF_SYS_PASSWD, conf->m_passwd, MAX_PIN_PASSWD_SIZE)) { 
            LOG_ERROR("init_conf| path=%s| sec=%s| key=%s|"
                " passwd=%s| msg=invalid param|",
                path, DEF_SEC_GLOBAL_NAME, 
                DEF_KEY_PASSWD_NAME,
                conf->m_passwd);
            
            return -1;
        }
    } else {
        return ret;
    }

    ret = parser.getParamInt(DEF_SEC_GLOBAL_NAME, 
        DEF_KEY_LOG_LEVEL_NAME, &conf->m_log_level);
    if (0 == ret) {
        setLogLevel(conf->m_log_level);
    } else {
        return ret;
    }
    
    ret = parser.getParamInt(DEF_SEC_GLOBAL_NAME, 
        DEF_KEY_LOG_STDIN_NAME, &conf->m_log_stdin);
    if (0 == ret) {
        setLogStdin(conf->m_log_stdin);
    } else {
        return ret;
    } 

    ret = parser.getParamInt(DEF_SEC_GLOBAL_NAME, 
        DEF_KEY_CHUNK_SIZE_NAME, &conf->m_chunk_size);
    if (0 == ret) {
        if (RTMP_MIN_CHUNK_SIZE > conf->m_chunk_size
            || RTMP_MAX_CHUNK_SIZE <= conf->m_chunk_size) {
            LOG_ERROR("init_conf| path=%s| sec=%s| key=%s|"
                " val=%d| msg=invalid param|",
                path, DEF_SEC_GLOBAL_NAME, 
                DEF_KEY_CHUNK_SIZE_NAME,
                conf->m_chunk_size);
            
            return -1;
        }
    } else {
        return ret;
    }

    ret = parser.getParamInt(DEF_SEC_GLOBAL_NAME, 
        DEF_KEY_WIN_SIZE_NAME, &conf->m_window_size);
    if (0 == ret) {
        if (0 > conf->m_window_size) {
            LOG_ERROR("init_conf| path=%s| sec=%s| key=%s|"
                " val=%d| msg=invalid param|",
                path, DEF_SEC_GLOBAL_NAME, 
                DEF_KEY_WIN_SIZE_NAME,
                conf->m_window_size);
            
            return -1;
        }
    } else {
        return ret;
    }

    ret = parser.getParamInt(DEF_SEC_GLOBAL_NAME, 
        DEF_KEY_PRNT_FLV_NAME, &conf->m_prnt_flv);
    if (0 != ret) {
        return ret;
    }

    ret = parser.getParamInt(DEF_SEC_GLOBAL_NAME, 
        DEF_KEY_PORT_NAME, &conf->m_port);
    if (0 != ret) { 
        return ret;
    }

    ret = parser.getParamStr(DEF_SEC_GLOBAL_NAME, 
        DEF_KEY_IP_NAME, conf->m_ip, sizeof(conf->m_ip));
    if (0 != ret) { 
        return ret;
    } 

    return 0;
}

void test_service(const char* path) {
    Int32 ret = 0; 

    ret = initConf(path, &g_conf);
    if (0 != ret) {
        return;
    }
    
    I_NEW_P(Director, g_director, DEF_FD_MAX_CAPACITY);

    ret = g_director->init();
    if (0 == ret) {
        g_director->addListener(ENUM_LISTENER_RTMP, 
            g_conf.m_ip, g_conf.m_port);
        g_director->addListener(ENUM_LISTENER_SOCK, 
            g_conf.m_ip, g_conf.m_port + 1);  // just for test
        g_director->addListener(ENUM_LISTENER_RTSP, 
            g_conf.m_ip, g_conf.m_port + 2);  // for rtsp
        g_director->start();
        g_director->join();
    }

    g_director->finish();

    I_FREE(g_director);

    sleepSec(3);
}

void usage(int, char* argv[]) {
    LOG_ERROR("usage: %s <opt> <conf_path>\n"
        "opt: [0]-test_service, default\n"
        "conf_path: [service.conf], default",
        argv[0]);
}

int test_main(int argc, char* argv[]) {
    int opt = 0;
    const char* path = NULL;

    armSig(SIGHUP, &sigHandler);
    armSig(SIGINT, &sigHandler);
    armSig(SIGQUIT, &sigHandler);
    armSig(SIGTERM, &sigHandler);

    if (2 <= argc) {
        opt = atoi(argv[1]);
    } else {
        opt = 0;
    }
    
    if (0 == opt) {
        if (3 <= argc) {
            path = argv[2];
        } else {
            path = DEF_AGENT_CONF_PATH;
        }
        
        test_service(path);
    } else if (1 == opt) {
        test_offset();
    } else {
        usage(argc, argv);
    }

    return 0;
}

