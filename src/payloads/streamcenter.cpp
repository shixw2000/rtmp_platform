#include"streamcenter.h"
#include"rtmpnode.h"
#include"cache.h"
#include"msgtype.h"


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

RtmpStream* StreamCenter::creatStream(const typeString& key) {
    RtmpStream* stream = NULL;

    I_NEW(RtmpStream, stream);
  
    stream->m_avc_id = ++m_avc_id;
    stream->m_key = key;
    stream->m_publisher = NULL;

    for (int i=0; i<ENUM_AVC_END; ++i) {
        stream->m_avc_cache[i] = NULL;
    } 
    
    m_map[key] = stream;
    return stream;
}

Void StreamCenter::delStream(const typeString& key) {
    RtmpStream* stream = NULL;
    typeMapStreamItr itr;

    itr = m_map.find(key);
    if (m_map.end() != itr) {
        stream = itr->second;
        
        /* delete publish */
        stream->m_publisher = NULL;

        /* delete all of players */
        stream->m_players.clear();

        I_FREE(stream);

        m_map.erase(itr); 
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

Void StreamCenter::cacheAvc(RtmpStream* stream,
    EnumAvcType type, Cache* cache) {
    
    if (NULL != stream->m_avc_cache[type]) {
        CacheCenter::del(stream->m_avc_cache[type]);
        
        stream->m_avc_cache[type] = CacheCenter::ref(cache);
    } else {
        stream->m_avc_cache[type] = CacheCenter::ref(cache);
    }
}

Void StreamCenter::updateAvc(RtmpStream* stream,
    EnumAvcType type, Cache* cache) {
    RtmpPkg* pkg_old = NULL;
    RtmpPkg* pkg_new = NULL; 

    pkg_new = MsgCenter::cast<RtmpPkg>(cache); 
    
    if (NULL != stream->m_avc_cache[type]) {
        pkg_old = MsgCenter::cast<RtmpPkg>(stream->m_avc_cache[type]); 
        
        if (pkg_old->m_epoch < pkg_new->m_epoch) {
            pkg_old->m_epoch = pkg_new->m_epoch;
        }
    } 
}

Void StreamCenter::playCache(RtmpUint* unit) {
    RtmpStream* stream = unit->m_ctx;

    if (NULL != stream->m_avc_cache[ENUM_AVC_META_DATA]) {
        unit->m_entity->onRecv(unit->m_entity, 
            ENUM_MSG_RTMP_TYPE_META_INFO,
            stream->m_avc_cache[ENUM_AVC_META_DATA]);
    }

    if (NULL != stream->m_avc_cache[ENUM_AVC_VEDIO]) {
        unit->m_entity->onRecv(unit->m_entity, 
            ENUM_MSG_RTMP_TYPE_VIDEO,
            stream->m_avc_cache[ENUM_AVC_VEDIO]);
    }

    if (NULL != stream->m_avc_cache[ENUM_AVC_AUDIO]) {
        unit->m_entity->onRecv(unit->m_entity, 
            ENUM_MSG_RTMP_TYPE_AUDIO,
            stream->m_avc_cache[ENUM_AVC_AUDIO]);
    }
}

Void StreamCenter::notifyEnd(RtmpStream* stream) {
    CacheHdr* hdr = NULL;
    RtmpUint* unit = NULL;
    typeMapUnitItrConst itr;
    typeMapUnit& units = stream->m_players;

    for (itr=units.begin(); itr != units.end(); ++itr) {
        unit = itr->second;

        hdr = MsgCenter::creatExchMsg(
            ENUM_MSG_CUSTOMER_NOTIFY_END_STREAM,
            unit->m_stream_id);
        
        unit->m_entity->dispatch(unit->m_entity, hdr);
    } 
}

Void StreamCenter::publish(RtmpStream* stream, Int32 msg_type, Cache* cache) {
    RtmpUint* unit = NULL;
    typeMapUnitItrConst itr;
    typeMapUnit& units = stream->m_players;

    for (itr=units.begin(); itr != units.end(); ++itr) {
        unit = itr->second;

        unit->m_entity->onRecv(unit->m_entity, msg_type, cache);
    }
}

Bool StreamCenter::regPlayer(RtmpUint* unit) {
    Bool bOk = FALSE;
    RtmpUint* old = NULL;
    RtmpStream* stream = NULL;
    typeString key;

    toString(&unit->m_key, key);
    
    stream = findStream(key);
    if (NULL == stream) {
        stream = creatStream(key);
    } 
    
    old = findPlayer(stream, unit->m_user_id);
    if (NULL == old) {
        stream->m_players[unit->m_user_id] = unit;
        unit->m_ctx = stream;
        
        bOk = TRUE;
    } else {
        /* already exists */
        bOk = FALSE;
    }

    return bOk;
}

Bool StreamCenter::unregPlayer(RtmpUint* unit) {
    Bool bOk = FALSE;
    RtmpUint* old = NULL;
    RtmpStream* stream = NULL;
    typeString key;

    toString(&unit->m_key, key);

    stream = findStream(key);
    if (NULL != stream) {
        old = findPlayer(stream, unit->m_user_id);
        if (NULL != old) {
            if (old == unit) {
                delPlayer(stream, unit->m_user_id);
                unit->m_ctx = NULL;
                
                bOk = TRUE;
            } else {
                /* there has another play with the same id */
                unit->m_ctx = NULL;
                bOk = FALSE;
            }
        } else {
            /* it is not a player of the stream */
            unit->m_ctx = NULL;
            bOk = TRUE;
        }
    } else {
        /* the stream doesnot exists */
        unit->m_ctx = NULL;
        bOk = TRUE;
    }

    return bOk;
}

Bool StreamCenter::regPublisher(RtmpUint* unit) {
    Bool bOk = FALSE;
    RtmpStream* stream = NULL;
    typeString key;

    toString(&unit->m_key, key);

    stream = findStream(key);
    if (NULL == stream) {
        stream = creatStream(key);
    } 
    
    if (NULL == stream->m_publisher) {
        stream->m_publisher = unit;
        unit->m_ctx = stream;
        bOk = TRUE;
    } else {
        /* already has publisher */
        bOk = FALSE;
    }

    return bOk;
}

Bool StreamCenter::unregPublisher(RtmpUint* unit) {
    Bool bOk = FALSE;
    RtmpStream* stream = NULL;
    typeString key;

    toString(&unit->m_key, key);

    stream = findStream(key);
    if (NULL != stream) {
        if (unit == stream->m_publisher) {
            stream->m_publisher = NULL;
            unit->m_ctx = NULL;
            bOk = TRUE;
        } else {
            /* it is not the publisher of the stream */
            unit->m_ctx = NULL;
            bOk = FALSE;
        }
    } else {
        /* the stream doesnot exists */
        unit->m_ctx = NULL;
        bOk = TRUE;
    }

    return bOk;
}

RtmpUint* StreamCenter::findPlayer(RtmpStream* stream, Uint32 id) {
    RtmpUint* unit = NULL;
    typeMapUnitItrConst itr;

    itr = stream->m_players.find(id);
    if (stream->m_players.end() != itr) {
        unit = itr->second;
        
        return unit;
    } else {
        return NULL;
    }
}

Bool StreamCenter::delPlayer(RtmpStream* stream, Uint32 id) {
    typeMapUnitItr itr;

    itr = stream->m_players.find(id);
    if (stream->m_players.end() != itr) {
        stream->m_players.erase(itr);
        
        return TRUE;
    } else {
        return FALSE;
    }
}

Uint32 StreamCenter::genUserId() {
    return ++m_user_id;
}

Void StreamCenter::toString(const Chunk* chunk, typeString& str) {
    if (0 < chunk->m_size) {
        str.assign((Char*)chunk->m_data, chunk->m_size);
    }
}

