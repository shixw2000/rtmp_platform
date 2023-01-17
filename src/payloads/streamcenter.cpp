#include"streamcenter.h"
#include"rtmpnode.h"
#include"cache.h"
#include"msgtype.h"
#include"payload.h"
#include"rtmpproto.h"


struct RtmpStream {
    RtmpUint* m_publisher;
    Cache* m_avc_cache[ENUM_AVC_END];
  
    Uint32 m_avc_id;
    Uint32 m_out_delta_ts;
    typeString m_key; 
    typeMapUnit m_players;
};

StreamCenter::StreamCenter(RtmpProto* proto_center)
    : m_proto_center(proto_center) {
    m_avc_id = 0;
}

RtmpStream* StreamCenter::findStream(const typeString& key) {
    RtmpStream* stream = NULL;
    typeMapStreamItrConst itr;

    itr = m_map.find(key);
    if (m_map.end() != itr) {
        stream = itr->second;
        
        return stream;
    } else {
        return NULL;
    } 
}

RtmpStream* StreamCenter::getStream(const typeString& key) {
    RtmpStream* stream = NULL;

    stream = findStream(key);
    if (NULL != stream) {
        return stream;
    } else {
        stream = creatStream(key);

        return stream;
    }
}

RtmpStream* StreamCenter::creatStream(const typeString& key) {
    RtmpStream* stream = NULL;

    I_NEW(RtmpStream, stream);
  
    stream->m_avc_id = ++m_avc_id;
    stream->m_out_delta_ts = 0;
    stream->m_key = key;
    stream->m_publisher = NULL;

    for (int i=0; i<ENUM_AVC_END; ++i) {
        stream->m_avc_cache[i] = NULL;
    } 
    
    m_map[key] = stream;
    return stream;
}

Void StreamCenter::freeStream(RtmpStream* stream) {
    RtmpUint* player = NULL;
    typeMapUnitItr itr;
    typeMapUnit& units = stream->m_players;
        
    /* delete publish */
    if (NULL != stream->m_publisher) { 
        unregPublisher(stream->m_publisher);
    }

    /* delete all of players */
    if (!units.empty()) {
        for (itr=units.begin(); itr != units.end(); ++itr) {
            player = itr->second;

            player->m_ctx = NULL;
            player->m_oper_auth = ENUM_OPER_END;
        }
        
        stream->m_players.clear(); 
    }

    for (int i=0; i<ENUM_AVC_END; ++i) {
        if (NULL != stream->m_avc_cache[i]) {
            CacheCenter::del(stream->m_avc_cache[i]);
            
            stream->m_avc_cache[i] = NULL;
        }
    } 

    I_FREE(stream);
}

Void StreamCenter::delStream(const typeString& key) {
    RtmpStream* stream = NULL;
    typeMapStreamItr itr;

    itr = m_map.find(key);
    if (m_map.end() != itr) {
        stream = itr->second;
        
        m_map.erase(itr);

        freeStream(stream);
    } 
}

Bool StreamCenter::chkAudioSeq(Byte data[]) {
    if (CODEC_AUDIO_AAC == (data[0] >> 4) && CODEC_AUDIO_SEQ == data[1]) {
        return TRUE;
    } else {
        return FALSE;
    }
}

Bool StreamCenter::chkVideoSeq(Byte data[]) {
    if (CODEC_VIDEO_AVC_H264 == data[0] && CODEC_VIDEO_AVC_SEQ == data[1]) {
        return TRUE;
    } else {
        return FALSE;
    }
}

Bool StreamCenter::chkVideoKeyFrame(Byte data[]) {
    if (CODEC_VIDEO_KEY_FRAME == (data[0]>>4)) {
        return TRUE;
    } else {
        return FALSE;
    }
}

Void StreamCenter::cacheAvc(RtmpUint* unit,
    EnumAvcType type, Cache* cache) {
    RtmpStream* stream = unit->m_ctx;

    if (NULL != stream) {
        if (NULL != stream->m_avc_cache[type]) {
            CacheCenter::del(stream->m_avc_cache[type]);
            
            stream->m_avc_cache[type] = CacheCenter::ref(cache);
        } else {
            stream->m_avc_cache[type] = CacheCenter::ref(cache);
        }
    }
}

Void StreamCenter::playBaseAvc(RtmpUint* unit) {
    RtmpStream* stream = unit->m_ctx;

    if (NULL != stream) {
        if (NULL != stream->m_avc_cache[ENUM_AVC_META_DATA]) {
            sendFlv(unit, stream->m_avc_cache[ENUM_AVC_META_DATA]);
        }

        if (NULL != stream->m_avc_cache[ENUM_AVC_VEDIO]) {
            sendFlv(unit, stream->m_avc_cache[ENUM_AVC_VEDIO]);
        }

        if (NULL != stream->m_avc_cache[ENUM_AVC_AUDIO]) {
            sendFlv(unit, stream->m_avc_cache[ENUM_AVC_AUDIO]);
        }
    }
}

Void StreamCenter::toString(const Chunk* chunk, typeString& str) {
    if (0 < chunk->m_size) {
        str.assign((Char*)chunk->m_data, chunk->m_size);
    }
}

Void StreamCenter::publish(RtmpStream* stream, Int32 msg_type, Cache* cache) {
    RtmpUint* unit = NULL;
    typeMapUnitItrConst itr;
    typeMapUnit& units = stream->m_players;

    for (itr=units.begin(); itr != units.end(); ++itr) {
        unit = itr->second;

        /* set sid of the unit */
        m_proto_center->recv(unit, msg_type, cache);
    }
}

Void StreamCenter::publishCmd(RtmpStream* stream, 
    Int32 msg_type, Int64 val) {
    RtmpUint* unit = NULL;
    typeMapUnitItrConst itr;
    typeMapUnit& units = stream->m_players;

    for (itr=units.begin(); itr != units.end(); ++itr) {
        unit = itr->second;

        m_proto_center->recvCmd(unit->m_parent, msg_type, val);
    }
} 

Int32 StreamCenter::regPlayer(RtmpUint* unit) {
    Int32 ret = 0;
    Uint32 user_id = 0;
    typeString strKey;
    RtmpStream* stream = NULL;
    typeMapUnitItr itr;

    user_id = unit->m_parent->m_user_id;
    toString(&unit->m_key, strKey);
    
    do {
        stream = getStream(strKey);
        if (NULL == stream) {
            ret = -1;
            break;
        } 

        itr = stream->m_players.find(user_id);
        if (stream->m_players.end() == itr) {
            stream->m_players[user_id] = unit;
            unit->m_ctx = stream;
        } else {
            /* already exists */
            ret = -2;
            break;
        }

        unit->m_oper_auth = ENUM_OPER_PLAYER;
    } while (0);
    
    return ret;
}

Int32 StreamCenter::unregPlayer(RtmpUint* unit) {
    Uint32 user_id = 0;
    RtmpStream* stream = NULL;
    typeMapUnitItr itr;

    user_id = unit->m_parent->m_user_id;
    
    stream = unit->m_ctx;
    if (NULL != stream) {
        stream->m_players.erase(user_id);

        unit->m_ctx = NULL;
    }
    
    unit->m_oper_auth = ENUM_OPER_END;
    return 0;
}

Int32 StreamCenter::regPublisher(RtmpUint* unit) {
    Int32 ret = 0;
    typeString strKey;
    RtmpStream* stream = NULL;

    toString(&unit->m_key, strKey);
    
    do {
        stream = getStream(strKey);
        if (NULL == stream) {
            ret = -1;
            break;
        }

        if (NULL == stream->m_publisher) {
            stream->m_publisher = unit;
            unit->m_ctx = stream;
        } else {
            ret = -2;
            break;
        }
        
        unit->m_oper_auth = ENUM_OPER_PUBLISHER;
    } while (0);
    
    return ret;
}

Int32 StreamCenter::unregPublisher(RtmpUint* unit) {

    if (NULL != unit->m_ctx) {
        unit->m_ctx->m_publisher = NULL;
        unit->m_ctx = NULL;
    }
    
    unit->m_oper_auth = ENUM_OPER_END;
    return 0;
}

RtmpUint* StreamCenter::creatUnit(Uint32 sid, Rtmp* rtmp) {
    RtmpUint* unit = NULL;
    
    I_NEW(RtmpUint, unit);

    CacheCenter::zero(unit, sizeof(RtmpUint));

    unit->m_sid = sid;
    unit->m_parent = rtmp; 
    return unit; 
}

Void StreamCenter::freeUnit(RtmpUint* unit) {
    if (NULL != unit) {
        freeAV(&unit->m_stream_name);
        freeAV(&unit->m_stream_type);
        freeAV(&unit->m_key);
        
        I_FREE(unit);
    }
}

Void StreamCenter::stopUnit(Rtmp* rtmp, Uint32 sid, Bool bSnd) {
    RtmpUint* unit = NULL;
    
    if (0 < sid && sid < RTMP_MAX_SESS_CNT && NULL != rtmp->m_unit[sid]) {
        unit = rtmp->m_unit[sid];
        
        if (ENUM_OPER_PUBLISHER == unit->m_oper_auth) {
            if (bSnd) {
                m_proto_center->notifyStop(rtmp, sid,
                    &av_status_play_stop,
                    RTMP_STREAM_STOP_DESC); 
            }
            
            unregPublisher(unit);
        } else if (ENUM_OPER_PLAYER == unit->m_oper_auth) {
            if (bSnd) {
                m_proto_center->notifyStop(rtmp, sid,
                    &av_status_play_stop,
                    RTMP_STREAM_STOP_DESC); 
            }
            
            unregPlayer(unit);
        } else {
            /* do nothing */
        }
        
        freeUnit(unit);
        rtmp->m_unit[sid] = NULL;
    } 
}

Void StreamCenter::unpublishFC(RtmpStream* stream, Chunk* name) {
    Cache* cache = 0;
    RtmpUint* unit = NULL;
    typeMapUnitItrConst itr;
    typeMapUnit& units = stream->m_players;

    cache = m_proto_center->genStatus(&av_status_play_end, name, RTMP_STREAM_END_DESC);
    //cache = m_proto_center->genStatus(&av_netstream_unpublish_notify, name, RTMP_STREAM_UNPUBLISH_DESC);

    for (itr=units.begin(); itr != units.end(); ++itr) {
        unit = itr->second;

        m_proto_center->sendUsrCtrl(unit->m_parent, ENUM_USR_CTRL_STREAM_END, unit->m_sid);
        
        sendInvoke(unit, cache);
    }

    CacheCenter::del(cache);
}

Int32 StreamCenter::sendFlv(RtmpUint* unit, Cache* cache) {
    Int32 ret = 0;

    ret = m_proto_center->send(unit, unit->m_ctx->m_out_delta_ts, cache);
    return ret;
}

Int32 StreamCenter::sendInvoke(RtmpUint* unit, Cache* cache) {
    Int32 ret = 0;

    ret = m_proto_center->send(unit, 0, cache);
    return ret;
}

Void StreamCenter::updateOutTs(RtmpUint* unit) {
    Uint32 cid = 0;
    ChnOutput* chn = NULL;
    RtmpStream* ctx = unit->m_ctx;
    Rtmp* rtmp = unit->m_parent;

    /* check audio out chn */
    cid = toCid(ENUM_MSG_RTMP_TYPE_AUDIO, unit->m_sid);
    chn = rtmp->m_chnnOut[cid];
    if (NULL != chn && ctx->m_out_delta_ts < chn->m_epoch) {
        ctx->m_out_delta_ts = chn->m_epoch;
    }

    /* check vidio out chn */
    cid = toCid(ENUM_MSG_RTMP_TYPE_VIDEO, unit->m_sid);
    chn = rtmp->m_chnnOut[cid];
    if (NULL != chn && ctx->m_out_delta_ts < chn->m_epoch) {
        ctx->m_out_delta_ts = chn->m_epoch;
    }
}

Void StreamCenter::publishFC(RtmpStream* stream, Chunk* name) {
    Cache* cache = 0;
    RtmpUint* unit = NULL;
    typeMapUnitItrConst itr;
    typeMapUnit& units = stream->m_players;

    cache = m_proto_center->genStatus(&av_netstream_publish_notify, name,
        RTMP_STREAM_PUBLISH_DESC);
    
    for (itr=units.begin(); itr != units.end(); ++itr) {
        unit = itr->second;

        updateOutTs(unit);

        m_proto_center->sendUsrCtrl(unit->m_parent, 
            ENUM_USR_CTRL_STREAM_BEGIN, unit->m_sid);
        
        sendInvoke(unit, cache);
    }

    CacheCenter::del(cache);
}

Void StreamCenter::stopPublisher(RtmpStream* stream) {
    RtmpUint* unit = stream->m_publisher;
    
    if (NULL != unit) {
        stopUnit(unit->m_parent, unit->m_sid, TRUE);
    }
}

