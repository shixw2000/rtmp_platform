#include<stdlib.h>
#include"tokenutil.h"
#include"httpdec.h"
#include"rtspmsg.h"


struct HttpStatusMsg {
    Int32 m_code;
    const Char* m_message;
};

static const HttpStatusMsg DEF_HTTP_STATUS_STR[] = {
    {RTSP_STATUS_CONTINUE,               "Continue"},
    {RTSP_STATUS_OK,                     "OK"},
    {RTSP_STATUS_CREATED,                "Created"},
    {RTSP_STATUS_LOW_ON_STORAGE_SPACE,   "Low on Storage Space"},
    {RTSP_STATUS_MULTIPLE_CHOICES,       "Multiple Choices"},
    {RTSP_STATUS_MOVED_PERMANENTLY,      "Moved Permanently"},
    {RTSP_STATUS_MOVED_TEMPORARILY,      "Moved Temporarily"},
    {RTSP_STATUS_SEE_OTHER,              "See Other"},
    {RTSP_STATUS_NOT_MODIFIED,           "Not Modified"},
    {RTSP_STATUS_USE_PROXY,              "Use Proxy"},
    {RTSP_STATUS_BAD_REQUEST,            "Bad Request"},
    {RTSP_STATUS_UNAUTHORIZED,           "Unauthorized"},
    {RTSP_STATUS_PAYMENT_REQUIRED,       "Payment Required"},
    {RTSP_STATUS_FORBIDDEN,              "Forbidden"},
    {RTSP_STATUS_NOT_FOUND,              "Not Found"},
    {RTSP_STATUS_METHOD,                 "Method Not Allowed"},
    {RTSP_STATUS_NOT_ACCEPTABLE,         "Not Acceptable"},
    {RTSP_STATUS_PROXY_AUTH_REQUIRED,    "Proxy Authentication Required"},
    {RTSP_STATUS_REQ_TIME_OUT,           "Request Time-out"},
    {RTSP_STATUS_GONE,                   "Gone"},
    {RTSP_STATUS_LENGTH_REQUIRED,        "Length Required"},
    {RTSP_STATUS_PRECONDITION_FAILED,    "Precondition Failed"},
    {RTSP_STATUS_REQ_ENTITY_2LARGE,      "Request Entity Too Large"},
    {RTSP_STATUS_REQ_URI_2LARGE,         "Request URI Too Large"},
    {RTSP_STATUS_UNSUPPORTED_MTYPE,      "Unsupported Media Type"},
    {RTSP_STATUS_PARAM_NOT_UNDERSTOOD,   "Parameter Not Understood"},
    {RTSP_STATUS_CONFERENCE_NOT_FOUND,   "Conference Not Found"},
    {RTSP_STATUS_BANDWIDTH,              "Not Enough Bandwidth"},
    {RTSP_STATUS_SESSION,                "Session Not Found"},
    {RTSP_STATUS_STATE,                  "Method Not Valid in This State"},
    {RTSP_STATUS_INVALID_HEADER_FIELD,   "Header Field Not Valid for Resource"},
    {RTSP_STATUS_INVALID_RANGE,          "Invalid Range"},
    {RTSP_STATUS_RONLY_PARAMETER,        "Parameter Is Read-Only"},
    {RTSP_STATUS_AGGREGATE,              "Aggregate Operation no Allowed"},
    {RTSP_STATUS_ONLY_AGGREGATE,         "Only Aggregate Operation Allowed"},
    {RTSP_STATUS_TRANSPORT,              "Unsupported Transport"},
    {RTSP_STATUS_UNREACHABLE,            "Destination Unreachable"},
    {RTSP_STATUS_INTERNAL,               "Internal Server Error"},
    {RTSP_STATUS_NOT_IMPLEMENTED,        "Not Implemented"},
    {RTSP_STATUS_BAD_GATEWAY,            "Bad Gateway"},
    {RTSP_STATUS_SERVICE,                "Service Unavailable"},
    {RTSP_STATUS_GATEWAY_TIME_OUT,       "Gateway Time-out"},
    {RTSP_STATUS_VERSION,                "RTSP Version not Supported"},
    {RTSP_STATUS_UNSUPPORTED_OPTION,     "Option not supported"},

    {0, NULL}
};

/* the order of this names must be consistent with the enum */
static const Char* DEF_HTTP_FIELD_NAME[ENUM_HTTP_FIELD_END] = 
{ 
    "Content-Length",
    "Session", 
    "Content-Type",
    "Content-Base",
    "Location",
    
    "Server", 
    "CSeq",
    "Date",
    "Transport",
    "Public",

    "WWW-Authenticate",
};

const Char* HttpTool::getHttpStatusStr(Int32 code) {
    const HttpStatusMsg* p = NULL;

    for (p = DEF_HTTP_STATUS_STR; NULL != p->m_message; ++p) {
        if (code == p->m_code) {
            return p->m_message;
        }
    }

    return "ERROR";
}

Void HttpTool::splitUri(const Token* uri, Token* schema, Token* auth,
    Token* ip, Int32* port, Token* path) {
    Bool bOk = FALSE;
    Token t = DEF_EMPTY_TOKEN;
    TokenUtil util;

    util.reset(uri);

    util.stripAll();

    /* erase the last '/' */
    util.skipTail("/");

    bOk = util.findStr(DEF_SCHEMA_MARKER_STR, &t);
    if (bOk) {
        util.cutBefore(&t, schema);

        /* forward to schema */
        util.seekAfter(&t);
    } else {
        /* just path*/
        util.copy(path);
        return;
    }

    /* get auth if has */
    bOk = util.findChar(DEF_AT_CHAR, &t);
    if (bOk) {
        util.cutBefore(&t, auth);
        
        util.seekAfter(&t);
    }

    /* get path */
    bOk = util.findFirstOf("/?", &t);
    if (bOk) {
        if (DEF_URL_SEP_CHAR == t.m_token[0]) {
            /* has path as '/' */
            util.cutFrom(&t, path);
        } else {
            /* has path as '?' */
            util.cutAfter(&t, path);
        }

        util.trancate(&t);
    } else {
        /* has no path in uri */
    }

    bOk = util.startEqual(DEF_SQUARE_BRACKET_LEFT); 
    if (bOk) {
        
        /* [host]:port */
        util.addHead(1);

        bOk = util.findChar(DEF_SQUARE_BRACKET_RIGHT, &t);
        if (bOk) {
            util.cutBefore(&t, ip);

            util.seekAfter(&t);

            bOk = util.startEqual(DEF_COLON_CHAR);
            if (bOk) {
                util.addHead(1);

                util.copy(&t);
                if (NULL != port) {
                    *port = TokenUtil::toNum(&t);
                }
            }
        } else {
            /* invalid uri*/
        } 
    } else {
        bOk = util.findChar(DEF_COLON_CHAR, &t);
        if (bOk) {
            util.cutBefore(&t, ip);
            util.seekAfter(&t);

            util.copy(&t);
            
            if (NULL != port) {
                *port = TokenUtil::toNum(&t);
            }
        } else {
            util.copy(ip);
        }
    }

    return;
}

Int32 HttpTool::parseCmdLine(const Token* orig, RtspCtx* ctx) {
    Int32 ret = 0;
    Bool bOk = FALSE;
    TokenUtil util;
    Token t1 = DEF_EMPTY_TOKEN;
    Token t2 = DEF_EMPTY_TOKEN;
    Token t3 = DEF_EMPTY_TOKEN;

    util.reset(orig);

    util.stripAll();

    do {        
        memset(ctx, 0, sizeof(RtspCtx));
        
        bOk = util.nextWord(&t1);
        if (!bOk) {
            ret = -1;
            break;
        }

        bOk = util.nextWord(&t2);
        if (!bOk) {
            ret = -2;
            break;
        }

        bOk = util.nextToken(DEF_NEWLINE_STR, &t3);
        if (!bOk) {
            ret = -3;
            break;
        }

        if (isRtspVer(&t1)) {
            /* response */
            ctx->m_is_req = FALSE;

            ctx->m_ver = t1;
            ctx->m_status_code = TokenUtil::toNum(&t2);
            ctx->m_reason = t3;
        } else if (isRtspVer(&t3)) {
            ctx->m_is_req = TRUE;

            ctx->m_ver = t3;
            ctx->m_method = t1;
            ctx->m_url = t2;
        } else {
            ret = -1;
            break;
        }

        return 0;
    } while (0);

    return ret;
}

Int32 HttpTool::parseHdrLine(const Token* orig, RtspCtx* ctx) {
    Int32 ret = 0;
    Bool bOk = FALSE;
    TokenUtil util;
    Token t = DEF_EMPTY_TOKEN;
    Token t1 = DEF_EMPTY_TOKEN;
    Token t2 = DEF_EMPTY_TOKEN;

    util.reset(orig);
        
    bOk = util.findChar(DEF_COLON_CHAR, &t);
    if (!bOk) {
        return -1;
    }
    
    util.cutBefore(&t, &t1);
    util.cutAfter(&t, &t2);
    
    TokenUtil::stripToken(&t1);
    TokenUtil::stripToken(&t2);

    if (0 == TokenUtil::cmpStr(&t1, DEF_HTTP_FIELD_NAME[ENUM_HTTP_SEQ])) {
        ctx->m_seq = TokenUtil::toNum(&t2);
    } else if (0 == TokenUtil::cmpStr(&t1, DEF_HTTP_FIELD_NAME[ENUM_HTTP_SESSION])) {
        ret = parseSess(&t2, ctx);
    } else if (0 == TokenUtil::cmpStr(&t1, DEF_HTTP_FIELD_NAME[ENUM_HTTP_TRANSPORT])) {
        ret = parseTransport(&t2, &ctx->m_transport);
    } else if (0 == TokenUtil::cmpStr(&t1, DEF_HTTP_FIELD_NAME[ENUM_HTTP_WWW_AUTH])) {
        ret = parseAuthInfo(&t2, &ctx->m_auth_info);
    } else if (0 == TokenUtil::cmpStr(&t1, DEF_HTTP_FIELD_NAME[ENUM_HTTP_CONTENT_BASE])) {
        ret = parseContentBase(&t2, ctx);
    } else if (0 == TokenUtil::cmpStr(&t1, DEF_HTTP_FIELD_NAME[ENUM_HTTP_CONTENT_TYPE])) { 
        ctx->m_content_type = t2;
    } else {
    }

    return ret;
}

Int32 HttpTool::parseTransport(const Token* orig, RtspTransport* param) {
    Int32 ret = 0;
    Bool bOk = FALSE;
    TokenUtil util;
    Token t = DEF_EMPTY_TOKEN;
    Token next = DEF_EMPTY_TOKEN; 

    next = *orig; 
    while (0 < next.m_size) { 
        memset(param, 0, sizeof(*param));
        
        util.reset(&next); 
        
        bOk = util.findChar(DEF_COMMA_CHAR, &t);
        if (bOk) {
            util.cutAfter(&t, &next);

            /* parse transport one by one */
            util.trancate(&t);
        } else {
            util.setToken(&next, NULL, 0);
        }

        util.stripAll();

        bOk = util.nextToken(DEF_SEMICOLON_CHAR, &t);
        if (bOk) {
            ret = parseTransSpec(&t, param);
            if (0 != ret) {
                /* if no support, then ignore */
                continue;
            } 
        } else {
            /* ignore this condition */
            continue;
        }
        
        bOk = util.nextToken(DEF_SEMICOLON_CHAR, &t);
        while (bOk) { 
            ret = parseTransParam(&t, param); 
            if (0 != ret) {
                break;
            }
            
            bOk = util.nextToken(DEF_SEMICOLON_CHAR, &t);
        } 

        if (0 != ret) {
            break;
        }

        if (!param->m_is_unicast && 0 == param->m_ttl) {
            param->m_ttl = 16;
        }
        
        param->m_valid = TRUE;

        return 0;
    } 

    return -1; 
}

Int32 HttpTool::parseTransSpec(const Token* orig, RtspTransport* param) {
    Int32 ret = 0;
    Bool bOk = FALSE;
    Token t = DEF_EMPTY_TOKEN;
    TokenUtil util;

    do {
        util.reset(orig);
            
        bOk = util.nextToken(DEF_URL_SEP_CHAR, &t);
        if (!bOk || 0 != TokenUtil::caseCmpStr(&t, DEF_RTP_STR)) {
            /* ignore not rtp */
            ret = -1;
            break;
        }
        
        bOk = util.nextToken(DEF_URL_SEP_CHAR, &t);
        if (!bOk || 0 != TokenUtil::caseCmpStr(&t, DEF_AVP_STR)) {
            /* ignore not avp */
            ret = -2;
            break;
        }

        bOk = util.nextToken(DEF_URL_SEP_CHAR, &t);
        if (bOk && 0 != TokenUtil::caseCmpStr(&t, DEF_UDP_STR)) {
            /* only support out of udp */
            ret = -1;
            break;
        }

        param->m_is_rtp_udp = TRUE;
        return 0;
    } while (0);

    return ret;
}

Int32 HttpTool::parseTransParam(const Token* orig, RtspTransport* param) {
    Int32 ret = 0;
    Bool bOk = FALSE;
    Token key = DEF_EMPTY_TOKEN;
    Token val = DEF_EMPTY_TOKEN;
    TokenUtil util;

    do {
        util.reset(orig);

        bOk = util.nextToken(DEF_URL_EQUATION_CHAR, &key);
        if (!bOk) {
            ret = -1;
            break;
        }

        /* may be null */
        util.nextToken(DEF_URL_EQUATION_CHAR, &val);

        if (0 == TokenUtil::cmpStr(&key, "port")) {
            parseRange(&val, param->m_port);
        } else if (0 == TokenUtil::cmpStr(&key, "client_port")) {
            parseRange(&val, param->m_client_port);
        } else if (0 == TokenUtil::cmpStr(&key, "server_port")) {
            parseRange(&val, param->m_server_port);
        } else if (0 == TokenUtil::cmpStr(&key, "unicast")) {
            param->m_is_unicast = TRUE;
        } else if (0 == TokenUtil::cmpStr(&key, "ttl")) {
            param->m_ttl = TokenUtil::toNum(&val);
        } else if (0 == TokenUtil::cmpStr(&key, "destination")) {
            param->m_dest = val;
        } else if (0 == TokenUtil::cmpStr(&key, "source")) {
            param->m_source = val;
        } else if (0 == TokenUtil::cmpStr(&key, "mode")) {
            if (0 == TokenUtil::caseCmpStr(&val, "record")
                || 0 == TokenUtil::caseCmpStr(&val, "receive")) {
                param->m_mode_record = TRUE;
            }
        } else {
        }

        return 0;
    } while (0);

    return ret;
}

Int32 HttpTool::parseRange(const Token* orig, Int32 fd[]) {
    Bool bOk = FALSE;
    Token t = DEF_EMPTY_TOKEN;
    Token r1 = DEF_EMPTY_TOKEN;
    Token r2 = DEF_EMPTY_TOKEN;
    TokenUtil util;

    util.reset(orig);

    bOk = util.findChar(DEF_URL_RANGE_CHAR, &t);
    if (bOk) {
        util.cutBefore(&t, &r1);
        util.cutAfter(&t, &r2);

        fd[0] = TokenUtil::toNum(&r1);
        fd[1] = TokenUtil::toNum(&r2);

        return 0;
    } else {
        return -1;
    }
}

Int32 HttpTool::parseRtspCtx(const HtmlMsg* msg, RtspCtx* ctx) {
    Int32 ret = 0;
    Int32 len = 0;
    const Span* span = NULL;
    Char* line = NULL;
    Token orig = DEF_EMPTY_TOKEN;

    if (0 < msg->m_field_num) { 
        span = &msg->m_fields[0];
        line = msg->m_txt + span->m_low;
        len = span->m_high - span->m_low;
        
        TokenUtil::setToken(&orig, line, len); 
    
        ret = parseCmdLine(&orig, ctx);
        if (0 != ret) {
            LOG_ERROR("parse_cmd| ret=%d| msg=error|", ret);
            return ret;
        }
    } else {
        LOG_ERROR("parse_header| msg=no fields error|"); 
        return ENUM_RTSP_ERR_FIELD_INVAL;
    }

    for (int i=1; i<msg->m_field_num; ++i) {
        span = &msg->m_fields[i];
        line = msg->m_txt + span->m_low;
        len = span->m_high - span->m_low;
    
        TokenUtil::setToken(&orig, line, len);

        ret = parseHdrLine(&orig, ctx); 
        if (0 != ret) {
            LOG_ERROR("parse_field| ret=%d| n=%d| msg=error|", ret, i);
            return ENUM_RTSP_ERR_FIELD_INVAL;
        }
    }
    
    TokenUtil::setToken(&ctx->m_body, msg->m_txt + msg->m_head_size, 
        msg->m_body_size);

    return 0;
}

Int32 HttpTool::parseAuthInfo(const Token* orig, RtspAuthInfo* param) {
    Int32 ret = 0;
    Bool bOk = FALSE;
    Token t = DEF_EMPTY_TOKEN;
    TokenUtil util;

    util.reset(orig);

    if (util.startWith("Basic ", &t)) {
        util.seekAfter(&t);

        bOk = util.nextWord(&t);
        while (bOk) { 
            if (0 == TokenUtil::caseCmpStr(&t, "realm")) {
                bOk = util.nextWord(&t);
                if (bOk && 0 == TokenUtil::caseCmpStr(&t, "=")) {
                    bOk = util.nextWord(&t);
                    if (bOk) {
                        param->m_realm = t;

                        break;
                    } 
                }
            } 

            bOk = util.nextWord(&t);
        } 
    } else if (util.startWith("Digest ", &t)) {
    } else {
    }
    
    return ret;
}

Int32 HttpTool::parseSess(const Token* orig, RtspCtx* ctx) {
    Int32 ret = 0;
    Bool bOk = FALSE;
    Token t = DEF_EMPTY_TOKEN;
    Token t1 = DEF_EMPTY_TOKEN;
    Token t2 = DEF_EMPTY_TOKEN;
    TokenUtil util;

    util.reset(orig);

    /* session: s;timeout= */
    util.nextToken(DEF_SEMICOLON_CHAR, &t);

    TokenUtil::stripToken(&t);
    TokenUtil::getToken(&t, ctx->m_sid, sizeof(ctx->m_sid));

    bOk = util.findChar(DEF_URL_EQUATION_CHAR, &t);
    if (bOk) {
        util.cutBefore(&t, &t1);
        util.cutAfter(&t, &t2);

        TokenUtil::stripToken(&t1);
        TokenUtil::stripToken(&t2);

        if (0 == TokenUtil::cmpStr(&t1, "timeout")) {
            ctx->m_timeout = TokenUtil::toNum(&t2);
        }
    }
    
    return ret;
}

Int32 HttpTool::parseContentBase(const Token* orig, RtspCtx* ctx) {
    Int32 ret = 0;
    Int32 port = 0;
    Token host = DEF_EMPTY_TOKEN;
    Token path = DEF_EMPTY_TOKEN;
    
    HttpTool::splitUri(orig, NULL, NULL, &host, &port, &path);

    if (DEF_URL_SEP_CHAR == path.m_token[0]) {
        ctx->m_control_uri = path;
    }

    return ret;
}

Bool HttpTool::isRtspVer(const Token* t) {
    TokenUtil util;

    util.reset(t); 
    if (util.startWith(DEF_RTSP_VER_PREFIX, NULL)) {
        return TRUE;
    } else {
        return FALSE;
    }
}

Int32 HttpTool::genTransport(const RtspTransport* trans, Token* buffer) {
    Int32 max = buffer->m_size;
    Int32 len = 0;

    if (!trans->m_valid) {
        return 0;
    }
    
    do { 
        len = snprintf(buffer->m_token, buffer->m_size, "%s",
            trans->m_is_rtp_udp ? "RTP/AVP/UDP" : "");
        if (len < buffer->m_size) {
            buffer->m_token += len;
            buffer->m_size -= len;
        } else {
            break;
        }
        
        if (trans->m_is_unicast) {
            len = snprintf(buffer->m_token, buffer->m_size, ";unicast");
            if (len < buffer->m_size) {
                buffer->m_token += len;
                buffer->m_size -= len;
            } else {
                break;
            }

            if (0 < trans->m_client_port[0]) {
                len = snprintf(buffer->m_token, buffer->m_size, 
                    ";client_port=%d-%d",
                    trans->m_client_port[0], 
                    trans->m_client_port[1]);
                if (len < buffer->m_size) {
                    buffer->m_token += len;
                    buffer->m_size -= len;
                } else {
                    break;
                }
            }

            if (0 < trans->m_server_port[0]) {
                len = snprintf(buffer->m_token, buffer->m_size,
                    ";server_port=%d-%d",
                    trans->m_server_port[0], 
                    trans->m_server_port[1]);
                if (len < buffer->m_size) {
                    buffer->m_token += len;
                    buffer->m_size -= len;
                } else {
                    break;
                }
            } 
        } else {
            len = snprintf(buffer->m_token, buffer->m_size, 
                ";multicast;ttl=%d",
                trans->m_ttl);
            if (len < buffer->m_size) {
                buffer->m_token += len;
                buffer->m_size -= len;
            } else {
                break;
            }

            if (0 < trans->m_port[0]) {
                len = snprintf(buffer->m_token, buffer->m_size, 
                    ";port=%d-%d",
                    trans->m_port[0], 
                    trans->m_port[1]);
                if (len < buffer->m_size) {
                    buffer->m_token += len;
                    buffer->m_size -= len;
                } else {
                    break;
                }
            }
        }

        len = snprintf(buffer->m_token, buffer->m_size,
            ";mode=%s",
            trans->m_mode_record ? "RECORD" : "PLAY");
        if (len < buffer->m_size) {
            buffer->m_token += len;
            buffer->m_size -= len;
        } else {
            break;
        }

        if (0 < trans->m_dest.m_size) {
            len = snprintf(buffer->m_token, buffer->m_size, 
                ";destination=%.*s",
                trans->m_dest.m_size, 
                trans->m_dest.m_token);
            if (len < buffer->m_size) {
                buffer->m_token += len;
                buffer->m_size -= len;
            } else {
                break;
            }
        }

        if (0 < trans->m_source.m_size) {
            len = snprintf(buffer->m_token, buffer->m_size, 
                ";source=%.*s",
                trans->m_source.m_size, 
                trans->m_source.m_token);
            if (len < buffer->m_size) {
                buffer->m_token += len;
                buffer->m_size -= len;
            } else {
                break;
            }
        }

        return max - buffer->m_size;
    } while (0);

    buffer->m_token[0] = DEF_NULL_CHAR;
    return -1;
}

Bool HttpTool::toStr(const RtspCtx* ctx, Token* orig) {
    Bool bOk = FALSE;
    Int32 len = 0;
    Token t = *orig;
    
    do {
        if (ctx->m_is_req) {
            len = snprintf(t.m_token, t.m_size, "print_rtsp_req|"
                " method=%.*s| url=%.*s| ver=%.*s|",
                ctx->m_method.m_size, ctx->m_method.m_token,
                ctx->m_url.m_size, ctx->m_url.m_token,
                ctx->m_ver.m_size, ctx->m_ver.m_token); 
        } else {
            len = snprintf(t.m_token, t.m_size, "print_rtsp_rsp|"
                " ver=%.*s| status_code=%d| reason=%.*s|",
                ctx->m_ver.m_size, ctx->m_ver.m_token,
                ctx->m_status_code,
                ctx->m_reason.m_size, ctx->m_reason.m_token);
        }

        if (len < t.m_size) {
            t.m_token += len;
            t.m_size -= len;
        } else {
            break;
        }

        len = snprintf(t.m_token, t.m_size,
            " seq=%u| sid=%s| timeout=%d|",
            ctx->m_seq, ctx->m_sid,
            ctx->m_timeout);
        if (len < t.m_size) {
            t.m_token += len;
            t.m_size -= len;
        } else {
            break;
        }

        if (0 < ctx->m_content_type.m_size) {
            len = snprintf(t.m_token, t.m_size, " content_type=%.*s|",
                ctx->m_content_type.m_size, 
                ctx->m_content_type.m_token);
            if (len < t.m_size) {
                t.m_token += len;
                t.m_size -= len;
            } else {
                break;
            }
        }

        if (0 < ctx->m_control_uri.m_size) {
            len = snprintf(t.m_token, t.m_size, " control_uri=%.*s|",
                ctx->m_control_uri.m_size, 
                ctx->m_control_uri.m_token);
            if (len < t.m_size) {
                t.m_token += len;
                t.m_size -= len;
            } else {
                break;
            }
        }

        if (ctx->m_transport.m_valid) {
            len = snprintf(t.m_token, t.m_size, " transport=[");
            if (len < t.m_size) {
                t.m_token += len;
                t.m_size -= len;
            } else {
                break;
            }

            /* this has already updated t */
            len = genTransport(&ctx->m_transport, &t);
            if (0 > len) {
                break;
            }

            len = snprintf(t.m_token, t.m_size, "]");
            if (len < t.m_size) {
                t.m_token += len;
                t.m_size -= len;
            } else {
                break;
            }
        } 

        len = snprintf(t.m_token, t.m_size, "] body=%d:%.*s|",
            ctx->m_body.m_size,
            ctx->m_body.m_size, ctx->m_body.m_token);
        if (len < t.m_size) {
            t.m_token += len;
            t.m_size -= len;
        } else {
            break;
        }
    } while (0);

    t.m_token[0] = DEF_NULL_CHAR;

    orig->m_size -= t.m_size;
    return bOk;
}

Void HttpTool::reset(RtspCtx* ctx) {
    memset(ctx, 0, sizeof(*ctx)); 
}

Bool HttpTool::genRtspMsg(const RtspCtx* ctx, Token* orig) {
    Bool bOk = FALSE;
    Int32 len = 0;
    Token buffer = *orig;
    
    do { 
        if (!ctx->m_is_req) {
            len = snprintf(buffer.m_token, buffer.m_size,
                "RTSP/1.0 %d %s\r\n",
                ctx->m_status_code,
                getHttpStatusStr(ctx->m_status_code));
        } else {
            len = snprintf(buffer.m_token, buffer.m_size, 
                "%.*s %.*s RTSP/1.0\r\n",
                ctx->m_method.m_size,ctx->m_method.m_token,
                ctx->m_url.m_size,ctx->m_url.m_token);
        }
        
        if (len < buffer.m_size) {
            buffer.m_token += len;
            buffer.m_size -= len;
        } else {
            break;
        }

        len = addFieldUint(&buffer, DEF_HTTP_FIELD_NAME[ENUM_HTTP_SEQ], 
            ctx->m_seq);
        if (0 > len) {
            break;
        }

        len = addDateField(&buffer);
        if (0 > len) {
            break;
        }

        if (ctx->m_transport.m_valid) {
            len = snprintf(buffer.m_token, buffer.m_size, "%s: ",
                DEF_HTTP_FIELD_NAME[ENUM_HTTP_TRANSPORT]);
            if (len < buffer.m_size) {
                buffer.m_token += len;
                buffer.m_size -= len;
            } else {
                break;
            }

            /* generate transport value in data */
            len = genTransport(&ctx->m_transport, &buffer);
            if (0 > len) {
                break;
            }

            len = snprintf(buffer.m_token, buffer.m_size, "\r\n");
            if (len < buffer.m_size) {
                buffer.m_token += len;
                buffer.m_size -= len;
            } else {
                break;
            }
        }

        if (0 < ctx->m_content_type.m_size) {
            len = addFieldToken(&buffer, 
                DEF_HTTP_FIELD_NAME[ENUM_HTTP_CONTENT_TYPE], 
                &ctx->m_content_type);
            if (0 > len) {
                break;
            }
        }

        if (0 < ctx->m_control_uri.m_size) {
            len = addFieldToken(&buffer, 
                DEF_HTTP_FIELD_NAME[ENUM_HTTP_CONTENT_BASE], 
                &ctx->m_control_uri);
            if (0 > len) {
                break;
            }
        }

        if (0 < ctx->m_public.m_size) {
            len = addFieldToken(&buffer, 
                DEF_HTTP_FIELD_NAME[ENUM_HTTP_PUBLIC], 
                &ctx->m_public);
            if (0 > len) {
                break;
            }
        }

        if (DEF_NULL_CHAR != ctx->m_sid[0]) {
            len = addField(&buffer, 
                DEF_HTTP_FIELD_NAME[ENUM_HTTP_SESSION], 
                ctx->m_sid);
            if (0 > len) {
                break;
            }
        }

        len = addBody(&buffer, &ctx->m_body);
        if (0 > len) {
            break;
        }

        bOk = TRUE;
    } while (0);

    buffer.m_token[0] = DEF_NULL_CHAR;
    
    orig->m_size -= buffer.m_size; 
    return bOk;
}

Int32 HttpTool::addDateField(Token* buffer) {
    Int32 len = 0;
    Uint32 ts = 0;
    Char date[DEF_TIME_FORMAT_SIZE] = {0};

    ts = getSysTime();
    formatTime(ts, "%a, %d %b %Y %H:%M:%S GMT", TRUE, date);

    len = addField(buffer, DEF_HTTP_FIELD_NAME[ENUM_HTTP_DATE], date);
    return len;
}

Int32 HttpTool::addField(Token* buffer, const Char* key, const Char* val) {
    Int32 len = 0;

    len = snprintf(buffer->m_token, buffer->m_size, 
        "%s: %s\r\n", key, val);
    if (len < buffer->m_size) { 
        buffer->m_token += len;
        buffer->m_size -= len;

        return len;
    } else {
        buffer->m_token[0] = DEF_NULL_CHAR;
        return -1;
    }
}

Int32 HttpTool::addFieldToken(Token* buffer, const Char* key, 
    const Token* val) {
    Int32 len = 0;

    if (0 < val->m_size) {
        len = snprintf(buffer->m_token, buffer->m_size, 
            "%s: %.*s\r\n", 
            key, 
            val->m_size, val->m_token);
        if (len < buffer->m_size) { 
            buffer->m_token += len;
            buffer->m_size -= len;

            return len;
        } else {
            buffer->m_token[0] = DEF_NULL_CHAR;
            return -1;
        }
    } else {
        return 0;
    }
}

Int32 HttpTool::addFieldInt(Token* buffer, const Char* key, Int32 val) {
    Int32 len = 0;

    len = snprintf(buffer->m_token, buffer->m_size, 
        "%s: %d\r\n", key, val);
    if (len < buffer->m_size) { 
        buffer->m_token += len;
        buffer->m_size -= len;

        return len;
    } else {
        buffer->m_token[0] = DEF_NULL_CHAR;
        return -1;
    }
}

Int32 HttpTool::addFieldUint(Token* buffer, const Char* key, Uint32 val) {
    Int32 len = 0;

    len = snprintf(buffer->m_token, buffer->m_size, 
        "%s: %u\r\n", key, val);
    if (len < buffer->m_size) { 
        buffer->m_token += len;
        buffer->m_size -= len;

        return len;
    } else {
        buffer->m_token[0] = DEF_NULL_CHAR;
        return -1;
    }
}

Int32 HttpTool::addBody(Token* buffer, const Token* body) {
    Int32 max = buffer->m_size;
    Int32 body_len = 0;
    Int32 len = 0;
    
    if (NULL != body && 0 < body->m_size) {
        body_len = body->m_size;
    }

    do {
        len = snprintf(buffer->m_token, buffer->m_size, 
            "%s: %d\r\n\r\n",
            DEF_HTTP_FIELD_NAME[ENUM_HTTP_CONTENT_LENGTH], body_len);
        if (len < buffer->m_size) { 
            buffer->m_token += len;
            buffer->m_size -= len; 
        } else {
            break;
        }

        if (0 < body_len  && body_len < buffer->m_size) {
            memcpy(buffer->m_token, body->m_token, body_len);
            buffer->m_token[body_len] = DEF_NULL_CHAR;
            
            buffer->m_token += body_len;
            buffer->m_size -= body_len;
        }

        return max - buffer->m_size;
    } while (0);

    buffer->m_token[0] = DEF_NULL_CHAR;
    return -1;
}


