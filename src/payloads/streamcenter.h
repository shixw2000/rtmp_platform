#ifndef __STREAMCENTER_H__
#define __STREAMCENTER_H__
#include<map>
#include<string>
#include"globaltype.h"
#include"datatype.h"


struct RtmpNode;
struct Cache;
class RtmpProto;

static const Byte CODEC_AUDIO_AAC = 0xa;
static const Byte CODEC_AUDIO_SEQ = 0x0;
static const Byte CODEC_VIDEO_KEY_FRAME = 0x1;
static const Byte CODEC_VIDEO_AVC_H264 = 0x17;
static const Byte CODEC_VIDEO_AVC_SEQ = 0x0;

typedef std::string typeString; 
typedef std::map<Uint32, RtmpUint*> typeMapUnit;
typedef typeMapUnit::iterator typeMapUnitItr;
typedef typeMapUnit::const_iterator typeMapUnitItrConst;

enum EnumAvcType { 
    ENUM_AVC_AUDIO,
    ENUM_AVC_VEDIO,
    ENUM_AVC_META_DATA,

    ENUM_AVC_END
};
 

typedef std::map<typeString, RtmpStream*> typeMapStream;
typedef typeMapStream::iterator typeMapStreamItr;
typedef typeMapStream::const_iterator typeMapStreamItrConst;

class RtmpHandler;

class StreamCenter {
public:
    explicit StreamCenter(RtmpProto* proto_center);
    
    static Bool chkAudioSeq(Byte data[]);
    static Bool chkVideoSeq(Byte data[]);
    static Bool chkVideoKeyFrame(Byte data[]);
    static Void toString(const Chunk* chunk, typeString& str);

    static RtmpUint* creatUnit(Uint32 sid, Rtmp* rtmp);
    static Void freeUnit(RtmpUint* unit);

    RtmpStream* findStream(const typeString& key);
    Void delStream(const typeString& key);
    
    Void playBaseAvc(RtmpUint* unit);
    Void cacheAvc(RtmpUint* unit, EnumAvcType type, Cache* cache); 
    
    Void clearAvc(RtmpStream* stream);
  
    /* the stream publish the msg to all of its players, 
        * then the players may start to deal it */
    Void publishCmd(RtmpStream* stream, Int32 msg_type, Int64 val);
    Void publish(RtmpStream* stream, Int32 msg_type, Cache* cache); 

    Int32 sendFlv(RtmpUint* unit, Cache* cache);

    Int32 sendInvoke(RtmpUint* unit, Cache* cache);

    Int32 regPlayer(RtmpUint* unit);
    Int32 unregPlayer(RtmpUint* unit);
    Int32 regPublisher(RtmpUint* unit);
    Int32 unregPublisher(RtmpUint* unit);

    Void publishFC(RtmpStream* stream, Chunk* name);
    Void unpublishFC(RtmpStream* stream, Chunk* name); 

    Void stopPublisher(RtmpStream* stream);
    
    /* stop the stream */
    Void stopUnit(Rtmp* rtmp, Uint32 sid, Bool bSnd); 

private:
    RtmpStream* creatStream(const typeString& key);
    Void freeStream(RtmpStream* stream);
    RtmpStream* getStream(const typeString& key);

    Void updateOutTs(RtmpUint* unit);

private:
    RtmpProto* m_proto_center;
    Uint32 m_avc_id;
    typeMapStream m_map;
};

#endif

