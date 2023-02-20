#include<stdlib.h>
#include<sys/uio.h>
#include"rtspmsg.h"
#include"rtpmsg.h"
#include"rtpdec.h"
#include"cache.h" 
#include"sock/udputil.h"


CacheHdr* RtpTools::creatUdpSendMsg(const AddrInfo* addr,
    Byte* data1, Int32 data1Len,
    Byte* data2, Int32 data2Len, Cache* cache) {
    CacheHdr* hdr = NULL;
    MsgUdpSend* msg = NULL;

    hdr = MsgCenter::creatMsg<MsgUdpSend>(ENUM_MSG_RTP_UDP_SEND, data1Len); 
    if (NULL != hdr) {
        msg = MsgCenter::body<MsgUdpSend>(hdr);

        CommSock::copyAddr(addr, &msg->m_dst_addr);
        
        if (0 < data1Len) {
            CacheCenter::copy(msg->m_head, data1, data1Len);
            msg->m_hdr_size = data1Len;
        }

        if (0 < data2Len) {
            msg->m_body = data2;
            msg->m_body_size = data2Len; 

            /* add ref */
            CacheCenter::setCache(hdr, CacheCenter::ref(cache));
        }
    }

    return hdr;
}

EnumSockState RtpTools::writeUdpMsg(Int32 fd, CacheHdr* hdr) {
    Int32 len1 = 0;
    Int32 len2 = 0;
    Int32 iov_len = 0;
    Int32 total = 0;
    Int32 sndlen = 0;
    MsgUdpSend* msg = NULL;
    struct iovec vec[2];
    
    msg = MsgCenter::body<MsgUdpSend>(hdr);
    
    len1 = msg->m_hdr_size - msg->m_hdr_upto;
    if (0 < len1) {
        vec[iov_len].iov_base = msg->m_head;
        vec[iov_len].iov_len = len1;
        ++iov_len;
    }
    
    len2 = msg->m_body_size - msg->m_body_upto;
    if (0 < len2) {
        vec[iov_len].iov_base = msg->m_body;
        vec[iov_len].iov_len = len2;
        ++iov_len;
    }
    
    total = len1 + len2;
    if (0 < total) {
        sndlen = UdpUtil::sendUdp(fd, &msg->m_dst_addr, vec, iov_len); 
    } 

    if (total == sndlen) {
        /* send completed */
        return ENUM_SOCK_STAT_COMPLETED;
    } else if (0 == sndlen) {
        /* blocked and need wait */
        return ENUM_SOCK_STAT_IN_PROGRESS;
    } else {
        return ENUM_SOCK_STAT_FAIL;
    } 
}

EnumSockRet RtpTools::writeUdpNode(NodeBase* node) {
    EnumSockState enStat = ENUM_SOCK_STAT_END;
    Int32 fd = node->getFd(node); 
    CacheHdr* hdr = NULL;
    list_node* pos = NULL;
    list_node* n = NULL;
    list_head* list = &node->m_snd_que;
    
    list_for_each_safe(pos, n, list) {
        hdr = CacheCenter::cast(pos);

        enStat = writeUdpMsg(fd, hdr);
        if (ENUM_SOCK_STAT_COMPLETED == enStat) {
            /* all of this msg has been sent */
            list_del(pos, list);
            CacheCenter::free(hdr);
        } else if (ENUM_SOCK_STAT_IN_PROGRESS == enStat) {
            /* blocked and wait to */
            return ENUM_SOCK_MARK_BLOCK;
        } else {
            return ENUM_SOCK_MARK_ERR;
        }
    }

    return ENUM_SOCK_MARK_FINISH;
}

Cache* RtpTools::creatRtpPkg(const AddrInfo* addr, 
    const Void* data, Int32 len) {
    Cache* cache = NULL;
    RtpPkg* pkg = NULL;

    cache = MsgCenter::creatCache<RtpPkg>(len);
    if (NULL != cache) {
        pkg = MsgCenter::cast<RtpPkg>(cache); 

        CommSock::copyAddr(addr, &pkg->m_origin);

        CacheCenter::copy(pkg->m_data, data, len);
        pkg->m_length = len;
    }

    return cache;
}


static const EncodingRule RTP_HEADER_RULE[] = {
    { U_INT_2, offset_of(RtpHdr, m_version) },
    { U_INT_1, offset_of(RtpHdr, m_padding) },
    { U_INT_1, offset_of(RtpHdr, m_ext) },
    { U_INT_4, offset_of(RtpHdr, m_cc) },
    { U_INT_1, offset_of(RtpHdr, m_marker) },
    { U_INT_7, offset_of(RtpHdr, m_pt) },
    { U_INT_16, offset_of(RtpHdr, m_seq) },
    { U_INT_32, offset_of(RtpHdr, m_ts) },
    { U_INT_32, offset_of(RtpHdr, m_ssrc) },
};

static const EncodingRule RTCP_HEADER_RULE[] = {
    { U_INT_2, offset_of(RtcpHdr, m_version) },
    { U_INT_1, offset_of(RtcpHdr, m_padding) },
    { U_INT_5, offset_of(RtcpHdr, m_rc) },
    { U_INT_8, offset_of(RtcpHdr, m_pt) },
    { U_INT_16, offset_of(RtcpHdr, m_length) },
};

static const EncodingRule RTCP_SENDER_RULE[] = {
    { U_INT_32, offset_of(RtcpSenderInfo, m_ntp_high) },
    { U_INT_32, offset_of(RtcpSenderInfo, m_ntp_low) },
    { U_INT_32, offset_of(RtcpSenderInfo, m_rtp_ts) },
    { U_INT_32, offset_of(RtcpSenderInfo, m_snd_pkt_cnt) },
    { U_INT_32, offset_of(RtcpSenderInfo, m_snd_octet_cnt) },
};

static const EncodingRule RTCP_REPORT_RULE[] = {
    { U_INT_32, offset_of(RtcpReportBlock, m_ssrc) },
    { U_INT_32, offset_of(RtcpReportBlock, m_frac_lost) },
    { U_INT_32, offset_of(RtcpReportBlock, m_cumul_pkg_lost) },
    { U_INT_32, offset_of(RtcpReportBlock, m_ext_max_seq) },
    { U_INT_32, offset_of(RtcpReportBlock, m_jitter) },
    { U_INT_32, offset_of(RtcpReportBlock, m_lsr_ts) },
    { U_INT_32, offset_of(RtcpReportBlock, m_delay_lsr) },
};


Bool RtpDecoder::isRtcpMsg(Int32 msg_type) {
    if (ENUM_RTCP_MSG_SR <= msg_type && ENUM_RTCP_MSG_SR >= msg_type) {
        return TRUE;
    } else {
        return FALSE;
    }
}

Int32 RtpDecoder::parseRtcp(RtpPkg* pkg) {
    Int32 ret = 0;
    Int32 rule_cnt = 0;
    const EncodingRule* rule = NULL;
    RtcpHdr hdr;

    memset(&hdr, 0, sizeof(hdr));

    reset(pkg->m_data, pkg->m_length);
    
    rule = RTCP_HEADER_RULE;
    rule_cnt = (Int32)count_of(RTCP_HEADER_RULE);
    ret = parseObj(rule_cnt, rule, &hdr);
    if (0 == ret && DEF_RTP_VERSION == hdr.m_version) {
        LOG_INFO("parse_rtcp_hdr| version=%u| padding=%u|"
            " rc=%u| payload_type=%u| length=%u| total=%d|",
            hdr.m_version, hdr.m_padding,
            hdr.m_rc, hdr.m_pt, hdr.m_length, pkg->m_length);
    } else {
        LOG_ERROR("parse_rtcp_hdr| ret=%d| rule_cnt=%d|"
            " version=%u| padding=%u|"
            " rc=%u| payload_type=%u| length=%u| total=%d| msg=error|",
            ret, rule_cnt,
            hdr.m_version, hdr.m_padding,
            hdr.m_rc, hdr.m_pt, hdr.m_length, pkg->m_length);
        
        return ret;
    }

    switch (hdr.m_pt) {
    case ENUM_RTCP_MSG_SR:
        ret = parseSR(&hdr);
        break;

    case ENUM_RTCP_MSG_RR:
        ret = parseRR(&hdr);
        break;

    case ENUM_RTCP_MSG_SDES:
        ret = parseSDes(&hdr);
        break;

    case ENUM_RTCP_MSG_BYE:
        break;

    default:
        ret = -1;
        break;
    }

    return ret;
}

Int32 RtpDecoder::parseRtp(RtpPkg* pkg) {
    Int32 ret = 0;
    Int32 extLen = 0;
    Int32 padded = 0;
    Int32 rule_cnt = 0;
    const EncodingRule* rule = NULL;
    RtpHdr hdr;

    memset(&hdr, 0, sizeof(hdr));

    reset(pkg->m_data, pkg->m_length);

    do {
        rule = RTP_HEADER_RULE;
        rule_cnt = (Int32)count_of(RTP_HEADER_RULE);
        ret = parseObj(rule_cnt, rule, &hdr);
        if (0 != ret || DEF_RTP_VERSION != hdr.m_version) {
            ret = -1;
            break;
        }

        if (0 < hdr.m_cc) {
            for (Int32 i=0; i<(Int32)hdr.m_cc; ++i) {
                ret = parseRule(0, U_INT_32, &hdr.m_csrc[i]);
                if (0 != ret) {
                    break;
                }
            }

            if (0 != ret) {
                break;
            }
        }
        
        if (hdr.m_ext) {
            /* read the extensiion part */
            ret = parseRule(0, U_INT_16, NULL);
            if (0 != ret) {
                break;
            }
            
            ret = parseRule(0, U_INT_16, &extLen);
            if (0 != ret) {
                break;
            }

            ret = parseChunk(0, extLen, NULL);
            if (0 != ret) {
                break;
            }
        }

        pkg->m_payload_skip = m_upto;

        if (hdr.m_padding) {
            padded = pkg->m_data[pkg->m_length - 1];

            if (0 < padded && padded < pkg->m_length - 12) {
                pkg->m_length -= padded;
            } else {
                ret = -1;
                break;
            }
        }

        if (hdr.m_marker) {
            /* has a complete msg */
        } else {
            /* has a partial msg */
        } 

        LOG_DEBUG("parse_rtp| version=%u| padding=%u:%d| ext=%u:%d|"
            " cc=%u| marker=%u| payload_type=%u|"
            " seq=%u| ts=%u| ssrc=%u| payload_skip=%d| total=%d|",
            hdr.m_version, hdr.m_padding, padded, 
            hdr.m_ext, extLen,
            hdr.m_cc, hdr.m_marker, hdr.m_pt,
            hdr.m_seq, hdr.m_ts, hdr.m_ssrc,
            pkg->m_payload_skip, pkg->m_length);

        return 0;
    } while (0);

    LOG_ERROR("parse_rtp| ret=%d| rule_cnt=%d|"
        " version=%u| padding=%u:%d| ext=%u:%d|"
        " cc=%u| marker=%u| payload_type=%u|"
        " seq=%u| ts=%u| ssrc=%u| payload_skip=%d| total=%d| msg=error|",
        ret, rule_cnt,
        hdr.m_version, hdr.m_padding, padded, 
        hdr.m_ext, extLen,
        hdr.m_cc, hdr.m_marker, hdr.m_pt,
        hdr.m_seq, hdr.m_ts, hdr.m_ssrc,
        pkg->m_payload_skip, pkg->m_length);
    
    return ret;
}

Int32 RtpDecoder::parseObj(Int32 cnt, const EncodingRule* rule, Void* obj) {
    Int32 ret = 0;
    Int32 enc_type = 0;
    Byte* pbase = (Byte*)obj;
    Byte* pb = NULL;
    
    for (Int32 ruleNum = 0; ruleNum < cnt && 0 == ret; ++ruleNum) {
        enc_type = rule[ruleNum].m_enc_type;
        pb = pbase + rule[ruleNum].m_offset;

        ret = parseRule(ruleNum, enc_type, pb);
    }

    return ret;
}

Int32 RtpDecoder::parseSR(const RtcpHdr* hdr) {
    Int32 ret = 0;
    Int32 used = 0;
    Int32 begin = 0;
    Int32 rule_cnt = 0;
    const EncodingRule* rule = NULL;
    RtcpSR oSR; 
    
    do {
        begin = m_upto;
        
        memset(&oSR, 0, sizeof(oSR));

        oSR.m_length = hdr->m_length;

        oSR.m_report_cnt = hdr->m_rc;
        ret = parseRule(0, U_INT_32, &oSR.m_ssrc);
        if (0 != ret) {
            break;
        }

        rule = RTCP_SENDER_RULE;
        rule_cnt = (Int32)count_of(RTCP_SENDER_RULE);
        ret = parseObj(rule_cnt, rule, &oSR.m_sender);
        if (0 != ret) {
            break;
        }

        rule = RTCP_REPORT_RULE;
        rule_cnt = (Int32)count_of(RTCP_REPORT_RULE);
        for (int i = 0; i<oSR.m_report_cnt; ++i) {
            ret = parseObj(rule_cnt, rule, &oSR.m_reports[i]);
            if (0 != ret) {
                break;
            }
        }

        if (0 != ret) {
            break;
        }

        used = (m_upto - begin + 3) >> 2;

        if (used == oSR.m_length) {
            m_upto = begin + (oSR.m_length << 2);
        } else {
            ret = -1;
            break;
        }

        LOG_INFO("parse_sr| length=%d| m_upto=%d|"
            " msg=ok|",
        oSR.m_length, m_upto);

        return 0;
    } while (0);
    
    LOG_ERROR("parse_sr| ret=%d| length=%d| begin=%d| m_upto=%d|"
        " msg=parse sr error|",
        ret, oSR.m_length, begin, m_upto);
    return ret;
}

Int32 RtpDecoder::parseRR(const RtcpHdr* hdr) {
    Int32 ret = 0;
    Int32 used = 0;
    Int32 begin = 0;
    RtcpRR oRR;
    Int32 rule_cnt = 0;
    const EncodingRule* rule = NULL;

    do {
        begin = m_upto;
        
        memset(&oRR, 0, sizeof(oRR));

        oRR.m_length = hdr->m_length;

        oRR.m_report_cnt = hdr->m_rc;
        
        ret = parseRule(0, U_INT_32, &oRR.m_ssrc);
        if (0 != ret) {
            break;
        }

        rule = RTCP_REPORT_RULE;
        rule_cnt = (Int32)count_of(RTCP_REPORT_RULE);
        for (int i = 0; i<oRR.m_report_cnt; ++i) {
            ret = parseObj(rule_cnt, rule, &oRR.m_reports[i]);
            if (0 != ret) {
                break;
            }
        }

        if (0 != ret) {
            break;
        }

        used = (m_upto - begin + 3) >> 2;

        if (used == oRR.m_length) {
            m_upto = begin + (oRR.m_length << 2);
        } else {
            ret = -1;
            break;
        }

        LOG_INFO("parse_rr| length=%d| m_upto=%d|"
            " msg=ok|",
        oRR.m_length, m_upto);

        return 0;
    } while (0);
    
    LOG_ERROR("parse_rr| ret=%d| length=%d| begin=%d| m_upto=%d|"
        " msg=parse rr error|",
        ret, oRR.m_length, begin, m_upto);
    return ret;
}

Int32 RtpDecoder::parseSDes(const RtcpHdr* hdr) {
    Int32 ret = 0;
    Int32 used = 0;
    Int32 begin = 0;
    Int32 length = 0;
    Uint32 ssrc = 0;
    Int32 des_cnt = 0;
    RtcpSdesItem oItem;

    begin = m_upto;
    length = hdr->m_length;
    des_cnt = hdr->m_rc;
    
    for (Int32 i=0; i<des_cnt && 0 == ret; ++i) {
        ret = parseRule(0, U_INT_32, &ssrc);
        if (0 != ret) {
            break;
        }

        while (0 == ret) {
            ret = parseRule(0, U_INT_8, &oItem.m_type);
            if (0 != ret) {
                break;
            }

            if (0 == oItem.m_type) {
                /* 4 bytes alignment */
                m_upto = (m_upto + 3) & (~0x3);
                break;
            }

            ret = parseRule(1, CHUNK_8, &oItem.m_val);
            if (0 != ret) {
                break;
            }

            /* do sdes task */
        }
    }

    if (0 == ret) {
        used = (m_upto - begin + 3) >> 2;

        if (used == length) {
            m_upto = begin + (length << 2);
        } else {
            ret = -1;
        }
    }

    if (0 == ret) {
        LOG_INFO("parse_sdes| length=%d| m_upto=%d| msg=ok|",
            length, m_upto);
    } else {
        LOG_ERROR("parse_sdes| ret=%d| length=%d| begin=%d| m_upto=%d|"
            " msg=parse rr error|",
            ret, length, begin, m_upto);
    }
    
    return ret;
}


