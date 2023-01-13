#include<string.h>
#include<stdlib.h>
#include<stdio.h>
#include<signal.h>
#include<unistd.h>
#include"testcase.h"
#include"datatype.h"
#include"director.h"


static Director* g_director = NULL;

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

void test_service(const Char szip[], int port) {
    Int32 ret = 0;

    I_NEW_P(Director, g_director, DEF_FD_MAX_CAPACITY);

    ret = g_director->init();
    if (0 == ret) {
        g_director->addListener(szip, port);
        g_director->start();
        g_director->join();
    }

    g_director->finish();

    I_FREE(g_director);

    sleepSec(3);
}

int test_main(int argc, char* argv[]) {
    int opt = 0;
    const Char* ip = NULL;
    int port = -1;

    armSig(SIGHUP, &sigHandler);
    armSig(SIGINT, &sigHandler);
    armSig(SIGQUIT, &sigHandler);
    armSig(SIGTERM, &sigHandler);

    opt = atoi(argv[1]);
    if (0 == opt) {
        if (4 == argc) {
            ip = argv[2];
            port = atoi(argv[3]);
            
            test_service(ip, port);
        }
    } else if (1 == opt) {
    } else if (2 == opt) {
    } else if (3 == opt) {
    } else if (4 == opt) {
    } else if (5 == opt) {
    } else if (6 == opt) {
    } else if (7 == opt) {
    } else if (8 == opt) {
        test_offset();
    } else {
    }

    return 0;
}

