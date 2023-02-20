#include<string.h>
#include"resmanage.h"
#include"tokenutil.h"
#include"httpdec.h"
#include"rtspmsg.h"
#include"rtpmsg.h"
#include"datatype.h"


static const Char* DEF_CODEC_TYPE_NAME[ENUM_MEDIA_TYPE_END] = {
    "video", "audio", "application"
};

Presentation* ResCenter::findRes(const Char* path) {
    typeResItr itr;

    itr = m_mapRes.find(path);
    if (m_mapRes.end() != itr) {
        return itr->second;
    } else {
        return NULL;
    }
}

Void ResCenter::addRes(Presentation* res) {
    if (DEF_NULL_CHAR != res->m_uri[0]) {
        m_mapRes[res->m_uri] = res;
    }
}

Void ResCenter::delRes(const Char* path) {
    typeResItr itr;

    itr = m_mapRes.find(path);
    if (m_mapRes.end() != itr) {
        m_mapRes.erase(itr);
    } 
}


Aggregate* ResCenter::findAggr(const Char* path) {
    typeAggrItr itr;

    itr = m_mapAggr.find(path);
    if (m_mapAggr.end() != itr) {
        return itr->second;
    } else {
        return NULL;
    }
}

Void ResCenter::delAggr(const Char* path) {
    typeAggrItr itr;

    itr = m_mapAggr.find(path);
    if (m_mapAggr.end() != itr) {
        m_mapAggr.erase(itr);
    } 
}

Void ResCenter::addGroup(Aggregate* aggr) {
    RtspStream* stream = NULL;

    for (Int32 i=0; i<aggr->m_stream_cnt; ++i) {
        stream = &aggr->m_stream[i];

        if (DEF_NULL_CHAR != stream->m_path[0]) {
            m_mapAggr[stream->m_path] = aggr;
        }
    }

    if (DEF_NULL_CHAR != aggr->m_path[0]) {
        m_mapAggr[aggr->m_path] = aggr;
    }
}

Void ResCenter::delGroup(Aggregate* aggr) {
    RtspStream* stream = NULL;

    for (int i=0; i<aggr->m_stream_cnt; ++i) {
        stream = &aggr->m_stream[i];
        
        delAggr(stream->m_path);
    }

    delAggr(aggr->m_path);
}

Presentation* ResTool::creatPresent() {
    Presentation* present = NULL;

    I_NEW(Presentation, present);

    resetPresent(present);
    
    return present;
}

Void ResTool::resetPresent(Presentation* present) { 
    memset(present, 0, sizeof(*present));
}

Void ResTool::resetMedia(MediaInfo* media) { 
    memset(media, 0, sizeof(*media));

    media->m_codec_type = ENUM_MEDIA_TYPE_END;
}

Void ResTool::freePresent(Presentation* present) {    
    if (NULL != present) {
        resetPresent(present);
        
        I_FREE(present);
    }
}

Bool ResTool::parseSDP(const Token* orig, Presentation* sdp) {
    Bool bOk = FALSE;
    Char letter = DEF_NULL_CHAR;
    Token t = DEF_EMPTY_TOKEN;
    TokenUtil util;

    util.reset(orig);
    
    do { 
        bOk = util.nextToken(DEF_NEWLINE_STR, &t);
        if (!bOk) {
            break;
        }
        
        if (2 >= t.m_size || DEF_URL_EQUATION_CHAR != t.m_token[1]) {
            continue;
        }
        
        letter = t.m_token[0];
        t.m_size -= 2;
        t.m_token += 2;

        if ('m' == letter) {
            bOk = parseMedia(&t, sdp);
        } else if ('c' == letter) {
            bOk = parseConn(&t, sdp);
        } else if ('a' == letter) {
            bOk = parseAttr(&t, sdp);
        } else if ('s' == letter) {
            TokenUtil::getToken(&t, sdp->m_title, sizeof(sdp->m_title));
        } else if ('i' == letter) {
            TokenUtil::getToken(&t, sdp->m_detail, sizeof(sdp->m_detail)); 
        } else {
        }
    } while (bOk && !util.isEol());

    if (bOk) {
        encCtx(sdp);
    }

    return bOk;
}

Bool ResTool::parseConn(const Token* orig, Presentation* sdp) {
    Int32 ttl = 0;
    Bool bOk = FALSE;
    MediaInfo* media = NULL;
    Token t = DEF_EMPTY_TOKEN;
    Token t1 = DEF_EMPTY_TOKEN;
    TokenUtil util;

    util.reset(orig); 

    do {
        if (0 < sdp->m_media_size) {
            media = &sdp->m_media[sdp->m_media_size-1];
        }
        
        bOk = util.nextWord(&t);
        if (!bOk) {
            break;
        }

        if (0 != TokenUtil::cmpStr(&t, "IN")) {
            /* noly support tcp/ip */
            break;
        }

        bOk = util.nextWord(&t);
        if (!bOk) {
            break;
        }

        if (0 != TokenUtil::cmpStr(&t, "IP4")
            && 0 != TokenUtil::cmpStr(&t, "IP6")) {
            break;
        }

        bOk = util.nextWord(&t);
        if (bOk) {
            /* get ip, may with ttl : ip/ttl*/
            util.reset(&t);
        } else {
            break;
        }

        bOk = util.findChar(DEF_URL_SEP_CHAR, &t);
        if (!bOk) {
            util.copy(&t1);
        } else {
            util.cutAfter(&t, &t1);
            ttl = TokenUtil::toNum(&t1);
            
            util.cutBefore(&t, &t1); 
        } 

        (Void)ttl;
        (Void)media;
        return 0;
    } while (0);

    return -1;
}

Bool ResTool::parseMedia(const Token* orig, Presentation* sdp) {
    Bool bOk = FALSE;
    MediaInfo* media = NULL;
    Token t = DEF_EMPTY_TOKEN;
    TokenUtil util;

    util.reset(orig); 

    media = &sdp->m_media[sdp->m_media_size]; 
    resetMedia(media);

    do {        
        resetMedia(media);
        
        bOk = util.nextWord(&t);
        if (!bOk) {
            break;
        }

        if (0 == TokenUtil::cmpStr(&t, 
            DEF_CODEC_TYPE_NAME[ENUM_MEDIA_TYPE_AUDIO])) {
            media->m_codec_type = ENUM_MEDIA_TYPE_AUDIO;
        } else if (0 == TokenUtil::cmpStr(&t, 
            DEF_CODEC_TYPE_NAME[ENUM_MEDIA_TYPE_VIDEO])) {
            media->m_codec_type = ENUM_MEDIA_TYPE_VIDEO;
        } else if (0 == TokenUtil::cmpStr(&t, 
            DEF_CODEC_TYPE_NAME[ENUM_MEDIA_TYPE_DATA])) {
            media->m_codec_type = ENUM_MEDIA_TYPE_DATA;
        } else {
            break;
        }

        /* port */
        bOk = util.nextWord(&t);
        if (!bOk) {
            break;
        }

        /* protocol rtp/avp */
        bOk = util.nextWord(&t);
        if (!bOk) {
            break;
        }

        if (0 != TokenUtil::cmpStr(&t, "RTP/AVP")) {
            break;
        }

        /* format list */
        bOk = util.nextWord(&t);
        if (!bOk) {
            break;
        }

        media->m_sdp_pt = TokenUtil::toNum(&t); 
        
        ++sdp->m_media_size;
        return TRUE;
    } while (0);

    return FALSE;
}

Bool ResTool::parseAttr(const Token* orig, Presentation* sdp) {
    Bool bOk = FALSE;
    Token t = DEF_EMPTY_TOKEN;
    Token t1 = DEF_EMPTY_TOKEN;
    TokenUtil util;

    util.reset(orig); 

    do {
        bOk = util.nextToken(DEF_COLON_CHAR, &t);
        if (!bOk) {
            break;
        }

        util.copy(&t1);

        if (0 == TokenUtil::cmpStr(&t, "control")) { 
            bOk = parseControl(&t1, sdp);
        } else if (0 == TokenUtil::cmpStr(&t, "rtpmap")) {
            bOk = parseRtpmap(&t1, sdp);
        } else if (0 == TokenUtil::cmpStr(&t, "fmtp")) {
            bOk = parseFmtp(&t1, sdp);
        } else {
        }

        if (!bOk) {
            break;
        }

        return TRUE;
    } while (0);

    return FALSE;
}

Bool ResTool::parseControl(const Token* orig, Presentation* sdp) {
    Int32 len = 0;
    Int32 left = 0;
    MediaInfo* media = NULL;
    Token path = DEF_EMPTY_TOKEN;

    do {
        if (0 < sdp->m_media_size) {
            media = &sdp->m_media[sdp->m_media_size-1];
        } 

        HttpTool::splitUri(orig, NULL, NULL, NULL, NULL, &path);
        if (0 == path.m_size) {
            break;
        }

        if (DEF_URL_SEP_CHAR == path.m_token[0]) {
            if (NULL != media) {
                TokenUtil::getToken(&path, media->m_uri, sizeof(media->m_uri));
            } else {
                TokenUtil::getToken(&path, sdp->m_uri, sizeof(sdp->m_uri));
            }
        } else {
            /* just add relative uri to media */
            if (NULL != media) {
                len = 0;
                left = (Int32)sizeof(media->m_uri);
                
                if (DEF_NULL_CHAR != sdp->m_uri[0]) {
                    len = snprintf(media->m_uri, left, "%s/", sdp->m_uri);
                    if (len < left) {
                        left -= len; 
                    } else {
                        media->m_uri[0] = DEF_NULL_CHAR;
                        break;
                    }
                }

                TokenUtil::getToken(&path, &media->m_uri[len], left);
            }
        }

        return TRUE;
    } while (0);

    return FALSE;
}

Bool ResTool::parseRtpmap(const Token* orig, Presentation* sdp) {
    Int32 pt = 0;
    Bool bOk = FALSE;
    MediaInfo* media = NULL;
    Token t = DEF_EMPTY_TOKEN;
    TokenUtil util;

    util.reset(orig); 

    do {
        if (0 < sdp->m_media_size) {
            media = &sdp->m_media[sdp->m_media_size-1];
        } else {
            break;
        } 

        bOk = util.nextWord(&t);
        if (!bOk) {
            break;
        }

        pt = TokenUtil::toNum(&t);
        if (pt != media->m_sdp_pt) {
            break;
        }

        bOk = util.nextToken(DEF_URL_SEP_CHAR, &t);
        if (bOk) {
            TokenUtil::getToken(&t, media->m_enc_name, 
                sizeof(media->m_enc_name));
        } else {
            break;
        }

        bOk = util.nextToken("/ ", &t);
        if (bOk) {
            media->m_sample_rate = TokenUtil::toNum(&t);
        } else {
            break;
        }

        bOk = util.nextWord(&t);
        if (bOk) {
            media->m_channel_num = TokenUtil::toNum(&t);
        }

        return TRUE;
    } while (FALSE);

    return FALSE;
}

Bool ResTool::parseFmtp(const Token* orig, Presentation* sdp) {
    MediaInfo* media = NULL;

    if (0 < sdp->m_media_size) {
        media = &sdp->m_media[sdp->m_media_size-1];

        TokenUtil::getToken(orig, media->m_fmtp, sizeof(media->m_fmtp));

        return TRUE;
    } else {
        return FALSE;
    }
}

Bool ResTool::encCtx(Presentation* sdp) {
    Bool bOk = FALSE;
    Int32 max = sizeof(sdp->m_ctx);
    Int32 len = 0;
    Token t = DEF_EMPTY_TOKEN;

    TokenUtil::setToken(&t, sdp->m_ctx, max); 

    do {
        len = snprintf(t.m_token, t.m_size,
            "v=0\r\n"
            "o=- 0 0 IN IP4 0.0.0.0\r\n"
            "s=%s\r\n"
            "i=%s\r\n"
            "c=IN IP4 0.0.0.0\r\n"
            "t=0 0\r\n"
            "a=tool:libavformat 57.25.100\r\n",
            sdp->m_title,
            sdp->m_detail);
        if (len < t.m_size) {
            t.m_token += len;
            t.m_size -= len;
        } else {
            break;
        }

        for (Int32 i=0; i<sdp->m_media_size; ++i) {
            bOk = encMedia(&sdp->m_media[i], &t);
            if (!bOk) {
                break;
            }
        }

        if (!bOk) {
            break;
        } 

        sdp->m_ctx_size = max - t.m_size;
        return TRUE;
    } while (0);
    
    sdp->m_ctx_size = 0;
    t.m_token[0] = DEF_NULL_CHAR;
    return FALSE;
}

Bool ResTool::encMedia(MediaInfo* media, Token* orig) {
    Int32 len = 0;
    Token t = *orig;

    do { 
        len = snprintf(t.m_token, t.m_size, "m=%s 0 RTP/AVP %d\r\n",
            DEF_CODEC_TYPE_NAME[media->m_codec_type],
            media->m_sdp_pt);
        if (len < t.m_size) {
            t.m_token += len;
            t.m_size -= len;
        } else {
            break;
        }

        len = snprintf(t.m_token, t.m_size, "c=IN IP4 0.0.0.0");
        if (len < t.m_size) {
            t.m_token += len;
            t.m_size -= len;
        } else {
            break;
        }

        len = snprintf(t.m_token, t.m_size, "a=rtpmap:%d %s/%d\r\n",
            media->m_sdp_pt,
            media->m_enc_name,
            media->m_sample_rate);
        if (len < t.m_size) {
            t.m_token += len;
            t.m_size -= len;
        } else {
            break;
        }

        len = snprintf(t.m_token, t.m_size, "a=fmtp:%d %s\r\n",
            media->m_sdp_pt,
            media->m_fmtp);
        if (len < t.m_size) {
            t.m_token += len;
            t.m_size -= len;
        } else {
            break;
        }

        len = snprintf(t.m_token, t.m_size, "a=control:%s\r\n",
            media->m_uri);
        if (len < t.m_size) {
            t.m_token += len;
            t.m_size -= len;
        } else {
            break;
        }
        
        return TRUE;
    } while (0);

    t.m_token[0] = DEF_NULL_CHAR;
    return FALSE;
}

Aggregate* ResTool::genAggr(Presentation* present) {
    Aggregate* aggr = NULL;
    RtspStream* stream = NULL;
    MediaInfo* media = NULL;

    I_NEW(Aggregate, aggr);
    memset(aggr, 0, sizeof(*aggr));

    aggr->m_path = present->m_uri;
    aggr->m_res = present;
    aggr->m_stream_cnt = present->m_media_size;

    for (Int32 i=0; i<present->m_media_size; ++i) {
        media = &present->m_media[i];
        stream = &aggr->m_stream[i];

        stream->m_path = media->m_uri;
        stream->m_media = media;
    }

    return aggr;
}

MediaInfo* ResTool::findMedia(Presentation* res, const Char* path) {
    MediaInfo* media = NULL;

    for (Int32 i=0; i<res->m_media_size; ++i) {
        media = &res->m_media[i];
        
        if (0 == strcmp(media->m_uri, path)) {
            return media;
        }
    }

    return NULL;
}

RtspStream* ResTool::findStream(Aggregate* aggr, const Char* path) {
    RtspStream* stream = NULL;

    for (Int32 i=0; i<aggr->m_stream_cnt; ++i) {
        stream = &aggr->m_stream[i];
        
        if (0 == strcmp(stream->m_path, path)) {
            return stream;
        }
    }

    return NULL;
}

