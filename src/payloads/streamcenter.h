#ifndef __STREAMCENTER_H__
#define __STREAMCENTER_H__
#include<map>
#include<string>
#include"globaltype.h"
#include"datatype.h"


struct RtmpNode;
struct RtmpStream;
struct Cache;

static const Byte CODEC_AUDIO_AAC = 0xa;
static const Byte CODEC_AUDIO_SEQ = 0x0;
static const Byte CODEC_VIDEO_KEY_FRAME = 0x1;
static const Byte CODEC_VIDEO_AVC_H264 = 0x17;
static const Byte CODEC_VIDEO_AVC_SEQ = 0x0;

typedef std::string typeString;

struct RtmpUint {
    RtmpNode* m_entity;
    RtmpStream* m_ctx;
    
    Uint32 m_user_id;
    Uint32 m_encoding;
    Uint32 m_stream_id;
    Uint32 m_max_out_epoch;     // max epoch of this rtmp
    Uint32 m_base_out_ts; // max offset of this rtmp
    Bool  m_is_pause; 
    Bool m_is_publisher;
    
    Chunk m_app;
    Chunk m_stream_name;
    Chunk m_tcUrl;
    Chunk m_stream_type;
    Chunk m_key; 
};

typedef std::map<Uint32, RtmpUint*> typeMapUnit;
typedef typeMapUnit::iterator typeMapUnitItr;
typedef typeMapUnit::const_iterator typeMapUnitItrConst;

enum EnumAvcType { 
    ENUM_AVC_AUDIO,
    ENUM_AVC_VEDIO,
    ENUM_AVC_META_DATA,

    ENUM_AVC_END
};

struct RtmpStream {
    RtmpUint* m_publisher;
    Cache* m_avc_cache[ENUM_AVC_END];
  
    Uint32 m_avc_id;
    typeString m_key; 
    typeMapUnit m_players;
}; 

typedef std::map<typeString, RtmpStream*> typeMapStream;
typedef typeMapStream::iterator typeMapStreamItr;
typedef typeMapStream::const_iterator typeMapStreamItrConst;

class StreamCenter {
public:
    static Void toString(const Chunk* chunk, typeString& str);
    
    Uint32 genUserId() ;

    Void playCache(RtmpUint* unit);
    Void cacheAvc(RtmpStream* stream, EnumAvcType type, Cache* cache);
    Void updateAvc(RtmpStream* stream, EnumAvcType type, Cache* cache);
    
    Bool chkAudioSeq(Byte data[]);
    Bool chkVideoSeq(Byte data[]);
    Bool chkVideoKeyFrame(Byte data[]);
    
    RtmpStream* findStream(const typeString& key);
    RtmpStream* creatStream(const typeString& key);
    Void delStream(const typeString& key);

    RtmpUint* findPlayer(RtmpStream* stream, Uint32 id);
    Bool delPlayer(RtmpStream* stream, Uint32 id);

    Void notifyEnd(RtmpStream* stream);
    
    Void publish(RtmpStream* stream, Int32 msg_type, Cache* cache);

    Bool regPlayer(RtmpUint* unit);
    Bool regPublisher(RtmpUint* unit);

    Bool unregPlayer(RtmpUint* unit);
    Bool unregPublisher(RtmpUint* unit); 

private:
    Uint32 m_user_id;
    Uint32 m_avc_id;
    typeMapStream m_map;
};

#endif

