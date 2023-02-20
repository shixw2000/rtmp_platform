#include<stdlib.h>
#include"rtspcenter.h"
#include"rtspdec.h"
#include"cache.h"
#include"rtspnode.h"
#include"sock/commsock.h"
#include"httpdec.h"
#include"tokenutil.h"
#include"resmanage.h"
#include"sesscenter.h"


Void StdTool::split(const typeString& txt, const Char* delim, typeVec& vecs) {
    typeSize pos = 0;
    typeSize found = 0;
    typeString line;

    vecs.clear();
    
    do { 
        found = txt.find_first_not_of(delim, pos);
        if (DEF_STR_END != found) {
            pos = found;

            found = txt.find_first_of(delim, pos);
            if (DEF_STR_END != found) {
                line = txt.substr(pos, found - pos);
            } else {
                line = txt.substr(pos);
            }

            strip(line);

            if (!line.empty()) {
                vecs.push_back(line); 
            }
        }

        pos = found;
    } while (DEF_STR_END != pos);
}

Void StdTool::strip(typeString& txt) {
    typeSize beg = 0;
    typeSize end = 0;

    beg = txt.find_first_not_of(DEF_BLANK_TOKEN, 0);
    if (DEF_STR_END != beg) {
        end = txt.find_last_not_of(DEF_BLANK_TOKEN, DEF_STR_END);

        txt = txt.substr(beg, end + 1 - beg);
    } else {
        txt.clear();
    } 
}

const typeString& StdTool::getVal(const typeFields& fields, 
    const typeString& key) {
    typeItrConst itr;

    itr = fields.find(key);
    if (fields.end() != itr) {
        return itr->second;
    } else {
        return DEF_RTSP_EMPTY_STR;
    } 
}

Void StdTool::setVal(typeFields& fields, 
    const typeString& key, const typeString& val) {
    typeItr itr;

    itr = fields.find(key);
    if (fields.end() != itr) {
        itr->second = val;
    } else {
        fields[key] = val;
    } 
}


static const RtspHandler::Callback g_cmds[] = {
    {"OPTIONS", &RtspHandler::dealOption},
    {"DESCRIBE", &RtspHandler::dealDescribe},
    {"ANNOUNCE", &RtspHandler::dealAnnounce},
    {"SETUP", &RtspHandler::dealSetup},
        
    {"PLAY", &RtspHandler::dealPlay}, 
    {"RECORD", &RtspHandler::dealRecord},

    {"TEARDOWN", &RtspHandler::dealTeardown},

    {NULL, NULL}
};

const RtspHandler::Callback* const RtspHandler::m_cmds = g_cmds;

RtspHandler::RtspHandler() { 
    m_res_center = NULL;
    m_sess_center = NULL;
    m_decoder = NULL;

    m_seq = 0;
}

Int32 RtspHandler::init() {
    I_NEW(RtspDec, m_decoder);
    I_NEW(ResCenter, m_res_center);
    I_NEW(SessCenter, m_sess_center);

    return 0;
}

Void RtspHandler::finish() {
    I_FREE(m_decoder);
    I_FREE(m_res_center);
    I_FREE(m_sess_center);
}

RtspHandler* RtspHandler::creat() {
    Int32 ret = 0;
    RtspHandler* hd = NULL;
    
    I_NEW(RtspHandler, hd);
    if (NULL != hd) {
        ret = hd->init();
        if (0 != ret) {
            freeHd(hd);

            hd = NULL;
        }
    }
    
    return hd;
}

Void RtspHandler::freeHd(RtspHandler* hd) {
    if (NULL != hd) {
        hd->finish();
        I_FREE(hd);
    }
}

CacheHdr* RtspHandler::creatRtspMsg() {
    CacheHdr* hdr = NULL;

    hdr = MsgCenter::creatMsg<HtmlMsg>(ENUM_MSG_RTSP_HTML); 
    return hdr;
}

Void RtspHandler::freeRtspMsg(CacheHdr* hdr) {
    HtmlMsg* msg = NULL;

    if (NULL != hdr) {
        msg = MsgCenter::body<HtmlMsg>(hdr);
        if (NULL != msg->m_txt) {
            free(msg->m_txt);
            msg->m_txt = NULL;
        }

        CacheCenter::free(hdr);
    }
}

Bool RtspHandler::isVer(const char* txt) {
    Int32 ret = 0;

    ret = strncmp(DEF_RTSP_RESP_PREFIX, txt, DEF_RTSP_RESP_PREFIX_SIZE);
    if (0 == ret) {
        return TRUE;
    } else {
        return FALSE;
    }
}

Bool RtspHandler::hasError(const char* txt) {
    Int32 ret = 0;

    ret = atoi(txt);
    if (100 <= ret && 400 > ret) {
        return FALSE;
    } else {
        return TRUE;
    }
}

Void RtspHandler::dspHtml(const Char* promt, CacheHdr* hdr) {
    Int32 total = 0;
    Int32 len = 0;
    Span* span = NULL;
    HtmlMsg* msg = NULL;
    Char* psz = NULL;
    Char buff[1024] = {0};

    msg = MsgCenter::body<HtmlMsg>(hdr);

    total = (Int32)sizeof(buff) - 1;
    psz = buff;
    for (int i=0; i<msg->m_field_num; ++i) {
        span = &msg->m_fields[i];
        
        len = snprintf(psz, total, " [%d]=%.*s|",
            i, span->m_high - span->m_low,
            msg->m_txt + span->m_low); 
        psz += len;
        total -= len;
    }

    *psz = BYTE_RTSP_NULL;
    
    LOG_INFO("%s| head_size=%d| body_size=%d| field_num=%d|%s"
        "\n==Msg_Begin==>>>>>>>>>>>\n%s\n<<<<<<<==End==|",
        promt,
        msg->m_head_size, 
        msg->m_body_size,
        msg->m_field_num,
        buff,
        msg->m_txt);
}

Int32 RtspHandler::parseRtsp(Rtsp* rtsp, Byte* data, Int32 len) {
    Int32 ret = 0;

    ret = m_decoder->parseMsg(rtsp, data, len);
    return ret;
}

Void RtspHandler::dspRtspCtx(const RtspCtx* ctx) {
    Token t = DEF_EMPTY_TOKEN;

    TokenUtil::setToken(&t, m_buffer, sizeof(m_buffer));
    
    HttpTool::toStr(ctx, &t);
    LOG_INFO("%s", t.m_token); 
}

Int32 RtspHandler::dealRtsp(Rtsp* rtsp, CacheHdr* hdr) {
    Int32 ret = 0;
    HtmlMsg* msg = NULL;
    RtspCtx oReq;
        
    do { 
        msg = MsgCenter::body<HtmlMsg>(hdr);

        dspHtml("deal_rstp", hdr);

        HttpTool::reset(&oReq);
        ret = HttpTool::parseRtspCtx(msg, &oReq); 
        
        dspRtspCtx(&oReq);
        
        if (0 != ret) {
            LOG_ERROR("deal_rtsp| ret=%d| msg=parse rtsp msg error|", ret);
            break;
        }

        if (oReq.m_is_req) {
            ret = dealReq(rtsp, &oReq);
        } else {
            ret = dealRsp(rtsp, &oReq);
        }
        
    } while (0); 

    LOG_INFO("deal_rstp| ret=%d|", ret);
    freeRtspMsg(hdr);
    return ret;
}

Int32 RtspHandler::dealRsp(Rtsp* , const RtspCtx*) {
    Int32 ret = 0;

    return ret;
}

Int32 RtspHandler::dealReq(Rtsp* rtsp, const RtspCtx* req) {
    Int32 ret = 0;
    const Callback* cb = NULL;
    RtspCtx oRsp;

    HttpTool::reset(&oRsp);

    /* default status */ 
    oRsp.m_status_code = RTSP_STATUS_NOT_FOUND;
    oRsp.m_seq = req->m_seq; 

    ret = -1;
    for (cb = m_cmds; NULL != cb->m_name; ++cb) {
        if (0 == TokenUtil::cmpStr(&req->m_method, cb->m_name)) {             
            ret = (this->*(cb->m_func))(rtsp, req, &oRsp);
            break;
        }
    } 
    
    sendRtspMsg(rtsp, &oRsp);
    
    return ret;
}

Int32 RtspHandler::sendRtspMsg(Rtsp* rtsp, const RtspCtx* ctx) {
    Int32 ret = 0;
    Bool bOk = FALSE;
    RtspNode* node = rtsp->m_entity;
    Token t = DEF_EMPTY_TOKEN; 

    TokenUtil::setToken(&t, m_buffer, sizeof(m_buffer));

    bOk = HttpTool::genRtspMsg(ctx, &t);
    if (bOk) { 
        node->sendData(node, t.m_token, t.m_size, NULL, 0, NULL);
    } else {
        ret = -1;
    }

    LOG_INFO("send_rtsp_msg| ret=%d| http_status=%d| len=%d| txt=%.*s|",
        ret, ctx->m_status_code, t.m_size, 
        t.m_size, t.m_token); 

    return ret;
}

Int32 RtspHandler::sendErrorReply(Rtsp* rtsp, Int32 code) {
    Int32 ret = 0;
    RtspCtx oRsp; 

    HttpTool::reset(&oRsp);

    oRsp.m_status_code = code;
    oRsp.m_seq = ++m_seq; 

    ret = sendRtspMsg(rtsp, &oRsp); 
    return ret;
}

Int32 RtspHandler::dealOption(Rtsp* , const RtspCtx* req, RtspCtx* rsp) {
    Bool bOk = FALSE;
    
    bOk = hasSession(req);
    if (bOk) {
        strncpy(rsp->m_sid, req->m_sid, sizeof(rsp->m_sid));
    } 
    
    TokenUtil::setToken(&rsp->m_public, DEF_RTSP_FIELD_PUBLIC_VAL);
    
    rsp->m_status_code = RTSP_STATUS_OK;
    return 0;
}

Int32 RtspHandler::dealDescribe(Rtsp* , const RtspCtx* req, RtspCtx* rsp) { 
    Bool bOk = FALSE;
    
    bOk = hasSession(req);
    if (bOk) {
        strncpy(rsp->m_sid, req->m_sid, sizeof(rsp->m_sid));
    }

    TokenUtil::setToken(&rsp->m_content_type, DEF_RTSP_FIELD_CONTENT_TYPE_VAL);

    rsp->m_control_uri = req->m_url; 
    rsp->m_body = *getDesc();
    
    rsp->m_status_code = RTSP_STATUS_OK;
    return 0;
}

Int32 RtspHandler::dealAnnounce(Rtsp* , const RtspCtx* req, RtspCtx* rsp) {
    Int32 ret = 0;
    Bool bOk = FALSE;
    
    bOk = hasSession(req);
    if (bOk) {
        strncpy(rsp->m_sid, req->m_sid, sizeof(rsp->m_sid));
    }

    do {
        ret = parseSDP(req);
        if (0 != ret) {
            rsp->m_status_code = RTSP_STATUS_PARAM_NOT_UNDERSTOOD; 
            break;
        }
        
        rsp->m_status_code = RTSP_STATUS_OK;
        return 0;
    } while (0);
    
    return ret;
}

Int32 RtspHandler::dealSetup(Rtsp* rtsp, const RtspCtx* req, RtspCtx* rsp) {
    Int32 ret = 0;
    Bool bOk = FALSE;
    Uint64 val = 0; 

    do {
        bOk = hasSession(req);
        if (bOk) {
            strncpy(rsp->m_sid, req->m_sid, sizeof(rsp->m_sid));
        } else {
            getRand(&val, sizeof(val));
            
            snprintf(rsp->m_sid, sizeof(rsp->m_sid), "%16llx", val);
        }
  
        bOk = chkTransport(req);
        if (!bOk) {
            rsp->m_status_code = RTSP_STATUS_TRANSPORT;
            ret = -1;
            break;
        }

        memcpy(&rsp->m_transport, &req->m_transport, sizeof(RtspTransport));
        rsp->m_transport.m_server_port[0] = rtsp->m_rtp->m_port_base;
        rsp->m_transport.m_server_port[1] = rtsp->m_rtp->m_port_base + 1;  
        
        rsp->m_status_code = RTSP_STATUS_OK;
        return 0;
    } while (0);
    
    return ret;
}

Bool RtspHandler::hasSession(const RtspCtx* ctx) {
    if (DEF_NULL_CHAR != ctx->m_sid[0]) {
        return TRUE;
    } else {
        return FALSE;
    }
}

Bool RtspHandler::chkTransport(const RtspCtx* ctx) {
    const RtspTransport* trans = &ctx->m_transport;

    do {
        if (!ctx->m_transport.m_valid) {
            break;
        }
        
        if (trans->m_is_unicast && (0 >= trans->m_client_port[0]
            || 0 >= trans->m_client_port[1])) {
            break;
        } else if (!trans->m_is_unicast && (0 >= trans->m_port[0]
            || 0 >= trans->m_port[1])) {
            break;
        }

        return TRUE;
    } while (0);

    return FALSE;
}

Int32 RtspHandler::dealPlay(Rtsp* , const RtspCtx* req, RtspCtx* rsp) {
    Int32 ret = 0;
    Bool bOk = FALSE;

    do {
        bOk = hasSession(req);
        if (bOk) {
            strncpy(rsp->m_sid, req->m_sid, sizeof(rsp->m_sid));
        } else {
            rsp->m_status_code = RTSP_STATUS_SESSION;
            ret = -1;
            break;
        } 

        rsp->m_status_code = RTSP_STATUS_OK;
        return 0;
    } while (0);

    return ret; 
}

Int32 RtspHandler::dealRecord(Rtsp* , const RtspCtx* req, RtspCtx* rsp) {
    Int32 ret = 0;
    Bool bOk = FALSE;

    do {
        bOk = hasSession(req);
        if (bOk) {
            strncpy(rsp->m_sid, req->m_sid, sizeof(rsp->m_sid));
        } else {
            rsp->m_status_code = RTSP_STATUS_SESSION;
            ret = -1;
            break;
        }
        
        rsp->m_status_code = RTSP_STATUS_OK;
        return 0;
    } while (0);

    return ret; 
}

Int32 RtspHandler::dealTeardown(Rtsp* , const RtspCtx* req, RtspCtx* rsp) {
    Int32 ret = 0;
    Bool bOk = FALSE;

    do {
        bOk = hasSession(req);
        if (bOk) {
            strncpy(rsp->m_sid, req->m_sid, sizeof(rsp->m_sid));
        } else {
            rsp->m_status_code = RTSP_STATUS_SESSION;
            ret = -1;
            break;
        }

        rsp->m_status_code = RTSP_STATUS_OK;
        return 0;
    } while (0);

    return ret; 
}

Int32 RtspHandler::parseSDP(const RtspCtx* req) {
    Int32 ret = 0;
    Presentation* sdp = NULL;

    sdp = ResTool::creatPresent();
    ret = ResTool::parseSDP(&req->m_body, sdp);

    ResTool::freePresent(sdp);
    return ret;
}

/* used temporarily */
const Token* RtspHandler::getDesc() {
    static const Char DEF_DESC[] = 
"v=0\r\n"
"o=- 0 0 IN IP4 127.0.0.1\r\n"
"s=No Title\r\n"
"c=IN IP4 0.0.0.0\r\n"
"t=0 0\r\n"
"a=tool:libavformat 57.25.100\r\n"
"m=video 0 RTP/AVP 96\r\n"
"b=AS:212\r\n"
"a=rtpmap:96 H264/90000\r\n"
"a=fmtp:96 packetization-mode=1; sprop-parameter-sets=Z2QAIKzZQMApsBEAAAMAAQAAAwAyDxgxlg==,aOvssiw=; profile-level-id=640020\r\n"
"a=control:streamid=0\r\n"
"m=audio 0 RTP/AVP 97\r\n"
"b=AS:30\r\n"
"a=rtpmap:97 MPEG4-GENERIC/44100/2\r\n"
"a=fmtp:97 profile-level-id=1;mode=AAC-hbr;sizelength=13;indexlength=3;indexdeltalength=3; config=1210\r\n"
"a=control:streamid=1\r\n";

    static const Int32 DEF_DESC_SIZE = sizeof(DEF_DESC) - 1;
    static const Token DEF_SDP_AVAL = { (Char*)DEF_DESC, DEF_DESC_SIZE };

    return &DEF_SDP_AVAL;
}



