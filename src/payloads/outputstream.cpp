#include<sys/uio.h>
#include"outputstream.h"
#include"datatype.h"
#include"sockutil.h"
#include"cache.h"
#include"msgtype.h"


EnumSockRet MsgOutput::writeMsg(NodeBase* node) {
    EnumSockRet ret = ENUM_SOCK_MARK_FINISH;
    Int32 fd = node->getFd(node); 
    list_head* list = &node->m_snd_que;

    ret = sendQue(fd, list);
    return ret;
}

EnumSockRet MsgOutput::sendQue(Int32 fd, list_head* list) {
    Int32 sndlen = 0;
    Int32 total = 0;
    Int32 avail = 0;
    Int32 cnt = 0;
    Int32 maxlen = DEF_TCP_SEND_BUF_SIZE;
    CacheHdr* hdr = NULL;
    MsgSend* msg = NULL;
    list_node* pos = NULL;
    struct iovec vec[DEF_VEC_SIZE];

    if (!list_empty(list)) {
        list_for_each(pos, list) {
            hdr = CacheCenter::cast(pos);
            msg = MsgCenter::body<MsgSend>(hdr);

            avail = msg->m_hdr_size - msg->m_hdr_upto;
            if (0 < avail) {
                vec[cnt].iov_base = msg->m_head + msg->m_hdr_upto; 
                
                if (avail < maxlen) {
                    vec[cnt].iov_len = avail;
                    maxlen -= avail;

                    ++cnt;
                    if (DEF_VEC_SIZE <= cnt) {
                        break;
                    }
                } else {
                    vec[cnt].iov_len = maxlen;
                    maxlen = 0;

                    ++cnt;
                    break;
                }
            } 

            avail = msg->m_body_size - msg->m_body_upto;
            if (0 < avail) {
                vec[cnt].iov_base = msg->m_body + msg->m_body_upto; 

                if (avail < maxlen) {
                    vec[cnt].iov_len = avail;
                    maxlen -= avail;

                    ++cnt;
                    if (DEF_VEC_SIZE <= cnt) {
                        break;
                    }
                } else {
                    vec[cnt].iov_len = maxlen;
                    maxlen = 0;

                    ++cnt;
                    break;
                }
            }
        }

        total = DEF_TCP_SEND_BUF_SIZE - maxlen;
        sndlen = sendVec(fd, vec, cnt, total);
        if (0 < sndlen) { 
            prune(list, sndlen);

            if (list_empty(list)) {
                return ENUM_SOCK_MARK_FINISH;
            } else if (sndlen == total) {
                return ENUM_SOCK_MARK_PARTIAL;
            } else {
                return ENUM_SOCK_MARK_BLOCK;
            }
        } else if (0 == sndlen) {
            /* send blocked */
            return ENUM_SOCK_MARK_BLOCK;
        } else {
            return ENUM_SOCK_MARK_ERR;
        }
       
    } else {
        return ENUM_SOCK_MARK_FINISH;
    }
}

Void MsgOutput::prune(list_head* list, Int32 maxlen) {
    Int32 len1 = 0;
    Int32 len2 = 0;
    CacheHdr* hdr = NULL;
    MsgSend* msg = NULL;
    list_node* pos = NULL;
    list_node* n = NULL;
    
    list_for_each_safe(pos, n, list) {
        hdr = CacheCenter::cast(pos);
        msg = MsgCenter::body<MsgSend>(hdr);
        
        len1 = msg->m_hdr_size - msg->m_hdr_upto;
        len2 = msg->m_body_size - msg->m_body_upto;

        if (len1 + len2 <= maxlen) { 
            msg->m_hdr_upto += len1;
            msg->m_body_upto += len2;
            
            /* all of this msg has been sent */
            list_del(pos, list);
            CacheCenter::free(hdr);

            maxlen -= len1 + len2;
            if (0 == maxlen) {
                break;
            }
        } else if (len1 <= maxlen) {
            msg->m_hdr_upto += len1;
            maxlen -= len1;
            
            msg->m_body_upto += maxlen;
            break;
        } else {
            msg->m_hdr_upto += maxlen;
            break;
        }
    }

    return;
} 


