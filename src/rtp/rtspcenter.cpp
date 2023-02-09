#include<stdlib.h>
#include"rtspcenter.h"
#include"rtspdec.h"
#include"cache.h"
#include"rtspnode.h"
#include"sock/commsock.h"


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
    m_port_base = MIN_RTSP_TRANSPORT_PORT;
    m_decoder = NULL;
}

Int32 RtspHandler::init() {
    I_NEW(RtspDec, m_decoder);

    memset(m_port_used_map, 0, sizeof(m_port_used_map));

    return 0;
}

Void RtspHandler::finish() {
    I_FREE(m_decoder);
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

Cache* RtspHandler::creatRtspPkg(Int32 len) {
    Cache* cache = NULL;
    RtspPkg* pkg = NULL;

    cache = MsgCenter::creatCache<RtspPkg>(len);
    if (NULL != cache) {
        pkg = MsgCenter::cast<RtspPkg>(cache); 

        pkg->m_len = len;
    }

    return cache;
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

Int32 RtspHandler::dealRtsp(Rtsp* rtsp, CacheHdr* hdr) {
    Int32 ret = 0;
    HtmlMsg* msg = NULL;
    HtmlCtx ctx; 

    resetCtx(&ctx);
    
    do { 
        msg = MsgCenter::body<HtmlMsg>(hdr);

        dspHtml("deal_rstp", hdr);
        
        ret = parseCtx(msg, &ctx); 
        if (0 != ret) {
            break;
        }

        if (ENUM_RTSP_HTML_RSP == ctx.m_html_type) {
            ret = dealRsp(rtsp, &ctx);
        } else if (ENUM_RTSP_HTML_REQ == ctx.m_html_type) {
            ret = dealReq(rtsp, &ctx);
        } else {
            ret = ENUM_RTSP_ERR_MSG_TYPE_INVAL;
            break;
        }
    } while (0); 

    dspHtmlCtx(ret, &ctx); 
    
    freeRtspMsg(hdr);
    return ret;
}

Int32 RtspHandler::parseCtx(HtmlMsg* msg, HtmlCtx* ctx) {
    Int32 ret = 0;
    Int32 len = 0;
    Span* span = NULL;
    Char* psz = NULL;

    do {
        if (0 == msg->m_field_num) {
            LOG_ERROR("parse_header| field_num=%d| msg=no field error|", 
                msg->m_field_num); 
            ret = ENUM_RTSP_ERR_FIELD_INVAL;
            break;
        }

        span = &msg->m_fields[0];
        psz = msg->m_txt + span->m_low;
        len = span->m_high - span->m_low;

        ret = parseMethod(ctx, psz, len);
        if (0 != ret) {
            LOG_ERROR("parse_method| ret=%d| msg=error|", ret);
            ret = ENUM_RTSP_ERR_METHOD_INVAL;
            break;
        }

        for (int i=1; i<msg->m_field_num; ++i) {
            span = &msg->m_fields[i];
            psz = msg->m_txt + span->m_low;
            len = span->m_high - span->m_low;

            ret = parseField(ctx, psz, len);
            if (0 != ret) {
                LOG_ERROR("parse_field| ret=%d| n=%d| msg=error|", ret, i);
                ret = ENUM_RTSP_ERR_FIELD_INVAL;
                break;
            }
        }

        if (0 == ret) {
            ctx->m_body = &msg->m_txt[msg->m_head_size];
            ctx->m_body_size = msg->m_body_size;
        } else { 
            break;
        }        
    } while (0);

    return ret;
}

Int32 RtspHandler::parseMethod(HtmlCtx* ctx, Char* data, Int32 len) {
    Int32 ret = 0;
    Char* psz1 = NULL;
    Char* psz2 = NULL;
    Char* psz3 = NULL;
    Char* pSave = NULL;

    do {
        data[len] = BYTE_RTSP_NULL;
        
        psz1 = strtok_r(data, " ", &pSave);
        if (NULL == psz1 || BYTE_RTSP_NULL == psz1[0]) {
            ret = -1;
            break;
        }

        psz2 = strtok_r(NULL, " ", &pSave);
        if (NULL == psz2 || BYTE_RTSP_NULL == psz2[0]) {
            ret = -2;
            break;
        }

        psz3 = strtok_r(NULL, " ", &pSave);
        if (NULL == psz3 || BYTE_RTSP_NULL == psz3[0]) {
            ret = -3;
            break;
        }

        /* must be the end of line */
        if (NULL != strtok_r(NULL, " ", &pSave)) {
            ret = -4;
            break;
        }
        
        if (isVer(psz1)) {
            ctx->m_html_type = ENUM_RTSP_HTML_RSP;
            ctx->m_ver = psz1;
            ctx->m_rsp_code = psz2;
            ctx->m_rsp_detail = psz3;
        } else if (isVer(psz3)) {
            ctx->m_html_type = ENUM_RTSP_HTML_REQ;
            ctx->m_ver = psz3;
            ctx->m_method = psz1; 

            ret = parsePath(ctx, psz2);
            if (0 != ret) {
                ret = -5;
                break;
            } 
        } else {
            /* neither req nor rsp, invalid txt */
            ret = -6;
            break;
        }
    } while (0);

    return ret;
}

Int32 RtspHandler::parsePath(HtmlCtx* ctx, Char* txt) {
    Int32 ret = 0;
    Char* psz = NULL;

    do {
        psz = strstr(txt, DEF_URL_SCHEMA_MARK);
        if (NULL != psz) {
            /* has prefix */
            psz += DEF_URL_SCHEMA_MARK_SIZE;
            psz = strchr(psz, DEF_PATH_SEPARATOR);
            if (NULL != psz) {
                ctx->m_prefix.assign(txt, psz - txt);

                /* skip to real path */
                txt = psz;
            } else {
                ret = -1;
                break;
            }
        } else if (DEF_PATH_SEPARATOR == txt[0]) {
            /* valid absolute path */
            ret = 0;
        } else {
            ret = -2;
            break;
        }

        psz = strchr(txt, DEF_QUERY_SEPARATOR);
        if (NULL != psz) {
            /* has query */
            ctx->m_path.assign(txt, psz - txt);
            ctx->m_query = psz + 1;
        } else { 
            ctx->m_path = txt;
        } 
    } while (0);

    return ret;
}

Int32 RtspHandler::parseField(HtmlCtx* ctx, Char* data, Int32 max) {
    Int32 len1 = 0;
    Int32 len2 = 0;
    Char* psz = NULL; 

    data[max] = BYTE_RTSP_NULL;
    
    psz = strstr(data, ": ");
    if (NULL != psz) {
        *psz = BYTE_RTSP_NULL;
        len1 = (Int32)(psz - data); 
        len2 = max - len1 - 2;
        psz += 2;

        if (0 < len1 && 0 < len2) {
            setField(ctx, data, psz);

            return 0;
        } else {
            return -1;
        }
    } else {
        return -1;
    }
}

Void RtspHandler::dspHtmlCtx(Int32 ret, const HtmlCtx* ctx) {
    Int32 total = 0;
    Int32 len = 0;
    Int32 n = 0;
    Char* psz = NULL;
    Char buff[1024] = {0};
    typeItrConst itr;
    const typeFields& mapFields = ctx->m_fields;

    total = (Int32)sizeof(buff) - 1;
    psz = buff;
    for (itr = mapFields.begin(); itr != mapFields.end(); ++itr) {  
        const typeString& strKey = itr->first;
        const typeString& strVal = itr->second;
        
        len = snprintf(psz, total, " [%d]%s=%s|",
            n++, strKey.c_str(), strVal.c_str()); 
        psz += len;
        total -= len;
    }

    *psz = BYTE_RTSP_NULL;

    if (ENUM_RTSP_HTML_RSP == ctx->m_html_type) {
        /* response */
        LOG_INFO("rsp_ret=%d| html_type=%d| ver=%s| rsp_code=%s| rsp_detail=%s|%s",
            ret,
            ctx->m_html_type,
            ctx->m_ver.c_str(),
            ctx->m_rsp_code.c_str(),
            ctx->m_rsp_detail.c_str(),            
            buff); 
    } else if (ENUM_RTSP_HTML_REQ == ctx->m_html_type) {
        /* request */
        LOG_INFO("req_ret=%d| html_type=%d| method=%s| prefix=%s|"
            " path=%s| query=%s| ver=%s|%s",
            ret,
            ctx->m_html_type,
            ctx->m_method.c_str(),
            ctx->m_prefix.c_str(),
            ctx->m_path.c_str(),
            ctx->m_query.c_str(),
            ctx->m_ver.c_str(),
            buff);  
    } 
}

Int32 RtspHandler::dealRsp(Rtsp* , HtmlCtx*) {
    Int32 ret = 0;

    return ret;
}

Int32 RtspHandler::dealReq(Rtsp* rtsp, const HtmlCtx* ctx) {
    Int32 ret = 0;
    const Callback* cb = NULL;
    HtmlCtx oRsp; 

    for (cb = m_cmds; NULL != cb->m_name; ++cb) {
        if (0 == ctx->m_method.compare(cb->m_name)) {
            resetCtx(&oRsp);
            
            ret = (this->*(cb->m_func))(rtsp, &oRsp, ctx);
            if (ENUM_RTSP_HTML_NULL != oRsp.m_html_type) { 
                sendCtx(rtsp, &oRsp);
            }
            
            return ret;
        }
    }

    /* unknown method */
    return ENUM_RTSP_ERR_UNKNOWN_METHOD;
}

Cache* RtspHandler::genRtspHtml(HtmlCtx* ctx) {
    Bool noQuery = FALSE;
    Int32 len = 0;
    Int32 total = 0;
    Int32 used = 0;
    Char* psz = NULL;
    Cache* cache = NULL;
    RtspPkg* pkg = NULL;
    typeItrConst itr;
    const typeFields& mapFields = ctx->m_fields;

    psz = m_buffer;
    total = (Int32)sizeof(m_buffer); 

    /* firstly must update content-length for buffer shared */
    snprintf(psz, total, "%d", ctx->m_body_size);
    setField(ctx, FIELD_CONTENT_LENGTH, psz);
    
    do {
        /* start with psz and total equals cap */ 
        if (ENUM_RTSP_HTML_RSP == ctx->m_html_type) { 
            len = snprintf(psz, total, "RTSP/1.0 %s %s\r\n",
                ctx->m_rsp_code.c_str(),
                ctx->m_rsp_detail.c_str());
        } else if (ENUM_RTSP_HTML_REQ == ctx->m_html_type) {
            noQuery = ctx->m_query.empty();
            
            len = snprintf(psz, total, "%s %s%s%s RTSP/1.0\r\n",
                ctx->m_method.c_str(),
                ctx->m_path.c_str(),
                noQuery ? "" : "?",
                noQuery ? "" : ctx->m_query.c_str());
        } else {
            /* no response */
            return NULL;
        }

        if (len < total) {
            psz += len;
            total -= len;
        } else {
            total = 0;
            break;
        }
        
        for (itr = mapFields.begin(); itr != mapFields.end(); ++itr) { 
            const typeString& strKey = itr->first;
            const typeString& strVal = itr->second;

            len = snprintf(psz, total, "%s: %s\r\n",
                strKey.c_str(), strVal.c_str());
            if (len < total) {
                psz += len;
                total -= len;
            } else {
                total = 0;
                break;
            }
        }
        
        /* add body delimeter */
        len = snprintf(psz, total, "\r\n");
        if (len < total) {
            psz += len;
            total -= len;
        } else {
            total = 0;
            break;
        }

        if (ctx->m_body_size < total) {
            if (0 < ctx->m_body_size) {
                CacheCenter::copy(psz, ctx->m_body, ctx->m_body_size);
                psz += ctx->m_body_size;
                total -= ctx->m_body_size; 
            } 
        } else {
            total = 0;
            break;
        } 

        *psz = BYTE_RTSP_NULL;
        used = (Int32)(psz - m_buffer); 

        cache = creatRtspPkg(used);
        if (NULL != cache) {
            pkg = MsgCenter::cast<RtspPkg>(cache); 

            CacheCenter::copy(pkg->m_data, m_buffer, used); 

            return cache;
        } else {
            break;
        }
    } while (0);

    used = (Int32)(psz - m_buffer); 

    LOG_ERROR("rtsp_rsp| used=%d| body_size=%d| left=%d|"
        " error=text exceeds max size|",
        used, ctx->m_body_size, total);
    
    return NULL;
} 

Void RtspHandler::dupKey(HtmlCtx* dst, const HtmlCtx* src, 
    const Char* key, const Char* defVal) {
    typeFields& mapDst = dst->m_fields;
    const typeFields& mapSrc = src->m_fields;
    typeString strKey(key);
    typeItrConst itr;

    itr = mapSrc.find(strKey);
    if (mapSrc.end() != itr) {
        const typeString& strVal = itr->second;

        if (!strVal.empty()) {
            mapDst[strKey] = strVal;
        } else {
            mapDst[strKey] = defVal;
        }
    } else {
        mapDst[strKey] = defVal;
    }
}

const typeString& RtspHandler::getField(const HtmlCtx* ctx, const Char* key) {
    typeString strKey(key);

    return StdTool::getVal(ctx->m_fields, strKey);
}

Void RtspHandler::setField(HtmlCtx* ctx, const Char* key, const Char* val) {
    StdTool::setVal(ctx->m_fields, key, val);
}

Void RtspHandler::setDateField(HtmlCtx* ctx) {
    Bool bUtc = TRUE;
    Uint32 ts = 0;
    Char date[DEF_TIME_FORMAT_SIZE] = {0};

    ts = getSysTime();
    formatTime(ts, "%a, %d %b %Y %H:%M:%S GMT", bUtc, date);
    setField(ctx, DEF_RTSP_FIELD_DATE, date);
}

Void RtspHandler::setSessionField(HtmlCtx* dst, const HtmlCtx* req) {
    Uint64 val = 0;
    Char buff[DEF_TMP_BUFFER_SIZE] = {0};

    getRand(&val, sizeof(val));
    snprintf(buff, DEF_TMP_BUFFER_SIZE, "%16llx", val);
    
    dupKey(dst, req, DEF_RTSP_FIELD_SESSION, buff);
}

Void RtspHandler::setContentBase(Rtsp* rtsp, HtmlCtx* ctx, 
    const HtmlCtx* req) {
    Int32 len = 0;
    Char buff[DEF_TMP_BUFFER_SIZE] = {0};

    len = snprintf(buff, DEF_TMP_BUFFER_SIZE, "rtsp://%s:%d%s/",
        rtsp->m_my_ip.m_ip, rtsp->m_my_ip.m_port, req->m_path.c_str());
    if (len >= DEF_TMP_BUFFER_SIZE) {
        len = DEF_TMP_BUFFER_SIZE - 1;
        buff[len] = BYTE_RTSP_NULL;
    }
    
    setField(ctx, DEF_RTSP_FIELD_CONTENT_BASE, buff);
}

Int32 RtspHandler::parseTransport(const HtmlCtx* ctx,
    typeVec& vecProto, typeVec& vecPort, typeFields& fields) {
    Int32 ret = 0;
    typeString req_txt;
    typeString usedTrans;
    typeString strCliPort;
    typeVec vecTrans;
    typeVec vecSpec;
    typeVec vecParam;

    do {
        req_txt = getField(ctx, DEF_RTSP_FIELD_TRANSPORT);
        if (req_txt.empty()) {
            ret = -1;
            break;
        }

        StdTool::split(req_txt, ",", vecTrans);
        if (!vecTrans.empty()) {
            usedTrans = vecTrans[0];
        } else {
            ret = -2;
            break;
        }
        
        StdTool::split(usedTrans, ";", vecSpec);
        if (!vecSpec.empty()) {
            StdTool::split(vecSpec[0], "/", vecProto);

            if (2 > vecProto.size()) {
                ret = -3;
                break;
            } else if (3 == vecProto.size()) {
                if (0 != vecProto[2].compare(DEF_PROTOCOL_UDP)) {
                    ret = -4;
                    break;
                }
            }
        } else {
            ret = -2;
            break;
        }

        for (typeSize i=1; i<vecSpec.size(); ++i) {
            StdTool::split(vecSpec[i], "=", vecParam);

            if (2 == vecParam.size()) {
                StdTool::setVal(fields, vecParam[0], vecParam[1]);
            } else if (1 == vecParam.size()) {
                StdTool::setVal(fields, vecParam[0], DEF_RTSP_EMPTY_TXT);
            } else {
                continue;
            }
        }

        strCliPort = StdTool::getVal(fields, DEF_RTSP_CLIENT_PORT_RANGE);
        if (!strCliPort.empty()) {
            StdTool::split(strCliPort, "-", vecPort);

            if (2 != vecPort.size()) {
                ret = -3;
                break;
            }
        } else {
            ret = -3;
            break;
        } 
    } while (0);

    return ret;
}

Void RtspHandler::setPortPairMap(Int32 port, Bool bUsed) {
    m_port_used_map[port] = bUsed;
}

Int32 RtspHandler::prepareRtp(Rtsp* rtsp, Int32 base_id, Int32* pPortBase) {
    Int32 ret = 0;
    Int32 srvPort = 0;
    Int32 fds[ENUM_RTSP_STREAM_END] = {0};
    RtspNode* entity = rtsp->m_entity;

    do {
        srvPort = getNextPortPair();
        if (0 > srvPort) {
            ret = -1;
            break;
        }

        ret = creatRtpPair(rtsp->m_my_ip.m_ip, srvPort, fds);
        if (0 != ret) {
            break;
        }

        rtsp->m_streams[base_id].m_node = entity->genRtp(entity, 
            fds[ENUM_RTSP_STREAM_RTP]);
        rtsp->m_streams[base_id + 1].m_node = entity->genRtcp(entity, 
            fds[ENUM_RTSP_STREAM_RTCP]);

        setPortPairMap(srvPort, TRUE);
        setPortPairMap(srvPort + 1, TRUE);

        *pPortBase = srvPort;

        return 0;
    } while (0);

    *pPortBase = -1;
    return ret;
}

Int32 RtspHandler::getNextPortPair() {
    
    for (Int32 i=0; i<MAX_RTSP_TRANSPORT_SIZE; i+=2) {
        if (!m_port_used_map[i]) {
            
            setPortPairMap(i, TRUE);
            return m_port_base + i;
        }
    }

    return -1;
}

Void RtspHandler::setTransport(Int32 srvPort, const typeVec& vecProto, 
    const typeVec& vecPort, HtmlCtx* ctx) {
    Char buff[DEF_TMP_BUFFER_SIZE] = {0};

    snprintf(buff, DEF_TMP_BUFFER_SIZE, 
        "%s/%s/UDP;unicast;client_port=%s-%s;server_port=%d-%d",
        vecProto[0].c_str(), vecProto[1].c_str(),
        vecPort[0].c_str(), vecPort[1].c_str(),
        srvPort, 
        srvPort + 1);
    
    setField(ctx, DEF_RTSP_FIELD_TRANSPORT, buff); 
}

Void RtspHandler::resetCtx(HtmlCtx* ctx) {
    ctx->m_html_type = ENUM_RTSP_HTML_NULL;
    ctx->m_body = NULL;
    ctx->m_body_size = 0;

    ctx->m_ver.clear();
    ctx->m_method.clear();
    ctx->m_prefix.clear();
    ctx->m_path.clear();
    ctx->m_query.clear();
    ctx->m_rsp_code.clear();
    ctx->m_rsp_code.clear();
    ctx->m_fields.clear();
}

Int32 RtspHandler::sendCtx(Rtsp* rtsp, HtmlCtx* ctx) {
    Int32 ret = 0;
    Cache* cache = NULL;
    RtspPkg* pkg = NULL;
    RtspNode* node = rtsp->m_entity;

    cache = genRtspHtml(ctx);
    if (NULL != cache) {
        pkg = MsgCenter::cast<RtspPkg>(cache);
        ret = node->sendData(node, NULL, 0, pkg->m_data, pkg->m_len, cache);

        LOG_INFO("send_rtsp_rsp| len=%d| txt=%.*s|",
            pkg->m_len, pkg->m_len, pkg->m_data);

        CacheCenter::del(cache);
    } 

    return ret;
}

Int32 RtspHandler::dealOption(Rtsp*, HtmlCtx* oRsp, const HtmlCtx* ctx) {
    Int32 ret = 0; 

    setField(oRsp, DEF_RTSP_FIELD_PUBLIC, DEF_RTSP_FIELD_PUBLIC_VAL);
    dupKey(oRsp, ctx, DEF_RTSP_FIELD_CSEQ); 

    oRsp->m_rsp_code = DEF_RTSP_RSP_CODE;
    oRsp->m_rsp_detail = RTSP_RSP_SUCCESS_DETAIL;
    oRsp->m_html_type = ENUM_RTSP_HTML_RSP;

    return ret;
}

Int32 RtspHandler::dealDescribe(Rtsp* rtsp, HtmlCtx* oRsp, const HtmlCtx* ctx) {
    Int32 ret = 0;
    const AVal* SDES_INFO = NULL;

    SDES_INFO = getDesc();
    oRsp->m_body = (const Char*)SDES_INFO->m_data;
    oRsp->m_body_size = SDES_INFO->m_size;

    dupKey(oRsp, ctx, DEF_RTSP_FIELD_CSEQ); 
    setDateField(oRsp);
    setField(oRsp, DEF_RTSP_FIELD_CONTENT_TYPE, 
        DEF_RTSP_FIELD_CONTENT_TYPE_VAL);

    setContentBase(rtsp, oRsp, ctx);
    
    oRsp->m_rsp_code = DEF_RTSP_RSP_CODE;
    oRsp->m_rsp_detail = RTSP_RSP_SUCCESS_DETAIL;
    oRsp->m_html_type = ENUM_RTSP_HTML_RSP;

    return ret;
}

Int32 RtspHandler::dealAnnounce(Rtsp* , HtmlCtx* oRsp, const HtmlCtx* ctx) {
    Int32 ret = 0; 

    dupKey(oRsp, ctx, DEF_RTSP_FIELD_CSEQ); 
    setDateField(oRsp);

    oRsp->m_rsp_code = DEF_RTSP_RSP_CODE;
    oRsp->m_rsp_detail = RTSP_RSP_SUCCESS_DETAIL;
    oRsp->m_html_type = ENUM_RTSP_HTML_RSP;

    return ret;
}

Int32 RtspHandler::dealSetup(Rtsp* rtsp, HtmlCtx* oRsp, const HtmlCtx* ctx) {
    Int32 ret = 0;
    Int32 base_id = 0;
    Int32 srvPort = -1;
    typeVec vecProto;
    typeVec vecPort;
    typeFields fields;

    do {
        oRsp->m_html_type = ENUM_RTSP_HTML_RSP;

        dupKey(oRsp, ctx, DEF_RTSP_FIELD_CSEQ); 
        setDateField(oRsp);
        
        ret = parseTransport(ctx, vecProto, vecPort, fields);
        if (0 != ret) { 
            break;
        } 

        if (NULL != rtsp->m_streams[0].m_node) {
            base_id += 2;
        }
        
        ret = prepareRtp(rtsp, base_id, &srvPort);
        if (0 != ret) {
            break;
        }
        
        setSessionField(oRsp, ctx);
        setTransport(srvPort, vecProto, vecPort, oRsp);
        
        oRsp->m_rsp_code = DEF_RTSP_RSP_CODE;
        oRsp->m_rsp_detail = RTSP_RSP_SUCCESS_DETAIL; 
        
        return 0;
    } while (0);

    oRsp->m_rsp_code = DEF_RTSP_RSP_NOT_SUPPORT_ERR;
    oRsp->m_rsp_detail = DEF_RTSP_RSP_NOT_SUPPORT_DETAIL;
    return ret;
}

Int32 RtspHandler::dealPlay(Rtsp* , HtmlCtx* oRsp, const HtmlCtx* ctx) {
    Int32 ret = 0;

    dupKey(oRsp, ctx, DEF_RTSP_FIELD_CSEQ); 
    setDateField(oRsp);
    setSessionField(oRsp, ctx);
    
    oRsp->m_rsp_code = DEF_RTSP_RSP_CODE;
    oRsp->m_rsp_detail = RTSP_RSP_SUCCESS_DETAIL;
    oRsp->m_html_type = ENUM_RTSP_HTML_RSP;

    return ret;
}

Int32 RtspHandler::dealRecord(Rtsp* , HtmlCtx* oRsp, const HtmlCtx* ctx) {
    Int32 ret = 0;

    dupKey(oRsp, ctx, DEF_RTSP_FIELD_CSEQ); 
    setDateField(oRsp);
    setSessionField(oRsp, ctx);
    
    oRsp->m_rsp_code = DEF_RTSP_RSP_CODE;
    oRsp->m_rsp_detail = RTSP_RSP_SUCCESS_DETAIL;
    oRsp->m_html_type = ENUM_RTSP_HTML_RSP;

    return ret;
}

Int32 RtspHandler::dealTeardown(Rtsp* , HtmlCtx* oRsp, const HtmlCtx* ctx) {
    Int32 ret = 0;

    dupKey(oRsp, ctx, DEF_RTSP_FIELD_CSEQ); 
    setDateField(oRsp);
    setSessionField(oRsp, ctx);
    
    oRsp->m_rsp_code = DEF_RTSP_RSP_CODE;
    oRsp->m_rsp_detail = RTSP_RSP_SUCCESS_DETAIL;
    oRsp->m_html_type = ENUM_RTSP_HTML_RSP;

    return ret;
}

/* used temporarily */
const AVal* RtspHandler::getDesc() {
    static const Byte DEF_DESC[] = 
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
    static const AVal DEF_SDP_AVAL = { (Byte*)DEF_DESC, DEF_DESC_SIZE };

    return &DEF_SDP_AVAL;
}


