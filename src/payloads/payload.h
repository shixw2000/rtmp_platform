#ifndef __PAYLOAD_H__
#define __PAYLOAD_H__
#include"sysoper.h"
#include"datatype.h"


typedef enum {
    AMF_NUMBER = 0, 
    AMF_BOOLEAN, 
    AMF_STRING, 
    AMF_OBJECT,
    AMF_MOVIECLIP,		/* reserved, not used */
    AMF_NULL, 
    AMF_UNDEFINED,
    AMF_REFERENCE, 
    AMF_ECMA_ARRAY, 
    AMF_OBJECT_END,
    AMF_STRICT_ARRAY, 
    AMF_DATE, 
    AMF_LONG_STRING, 
    AMF_UNSUPPORTED,
    AMF_RECORDSET,		/* reserved, not used */
    AMF_XML_DOC, 
    AMF_TYPED_OBJECT,
    AMF_AVMPLUS,		/* switch to AMF3 */
    AMF_INVALID = 0xff
} AMFDataType;

struct AMFObjectProperty;

typedef struct AMFObject {
    int m_num;
    struct AMFObjectProperty* m_props;
} AMFObject;

typedef struct AMFObjectProperty {
    Chunk m_name;
    AMFDataType m_type;
    
    union {
        Int32 m_n;
        double m_number;
        Chunk m_val;
        AMFObject m_object;
    } p_vu;
} AMFObjectProperty;

static const AMFObjectProperty AMF_PROP_INVALID = { {0, 0}, AMF_INVALID, {0} };
static const AMFObject AMF_OBJ_INVALID = { 0, NULL };
static const Chunk AMF_CHUNK_EMPTY = { NULL, 0 };
static const Int32 AMF_GROWTH_PROP_SIZE = 0x1 << 4;
static const Int32 AMF_GROWTH_PROP_MASK = AMF_GROWTH_PROP_SIZE - 1;
static const double AMF_ZERO_DOUBLE = 0.;
static const Byte DEF_CHUNK_DELIM = 0x1;
static const Byte NULL_CHUNK_DELIM = 0x0;

#define AVC(str)	{(Byte*)str, sizeof(str)-1}
#define SAVC(x) static const Chunk av_##x = AVC(#x)
#define STRAVC(x,str) static const Chunk av_##x = AVC(str)

SAVC(app);
SAVC(connect);
SAVC(close);
SAVC(flashVer);
SAVC(swfUrl);
SAVC(pageUrl);
SAVC(tcUrl);
SAVC(fpad);
SAVC(capabilities);
SAVC(audioCodecs);
SAVC(videoCodecs);
SAVC(videoFunction);
SAVC(objectEncoding);
SAVC(_result);
SAVC(createStream);
SAVC(getStreamLength);
SAVC(play);
SAVC(fmsVer);
SAVC(mode);
SAVC(level);
SAVC(code);
SAVC(description);
SAVC(secureToken);
SAVC(onBWDone);
SAVC(onStatus);
SAVC(details);
SAVC(clientid);
SAVC(closeStream);
SAVC(releaseStream);
SAVC(pause);
SAVC(publish);
SAVC(FCPublish);
SAVC(FCUnpublish);
SAVC(onFCPublish);
SAVC(_checkbw);
SAVC(deleteStream);


STRAVC(def_fmsVer, "FMS/3,5,1,525");
STRAVC(status_level_ok, "status");
STRAVC(status_level_err, "error");

STRAVC(status_conn_ok, "NetConnection.Connect.Success");
STRAVC(status_conn_des, "Connection succeeded.");
STRAVC(status_conn_reject, "NetConnection.Connect.Rejected");

STRAVC(fms_version, "version");
STRAVC(fms_data, "data");
STRAVC(def_fms_version, "3,5,1,525");
STRAVC(def_clientid, "ASAICiss");

STRAVC(status_play_reset, "NetStream.Play.Reset");
STRAVC(status_play_start, "NetStream.Play.Start");
STRAVC(status_play_stop, "NetStream.Play.Stop");
STRAVC(status_play_end, "NetStream.Play.Complete");
STRAVC(netstream_pause_notify, "NetStream.Pause.Notify");
STRAVC(netstream_unpause_notify, "NetStream.Unpause.Notify");
STRAVC(netstream_publish_start, "NetStream.Publish.Start");
STRAVC(netstream_publish_notify, "NetStream.Play.PublishNotify");
STRAVC(netstream_unpublish_notify, "NetStream.Play.UnpublishNotify");
STRAVC(meta_prefix_data, "@setDataFrame");


static const Char RTMP_STREAM_START_DESC[] = "NetStream is now started";
static const Char RTMP_STREAM_STOP_DESC[] = "NetStream is now stoped";
static const Char RTMP_STREAM_END_DESC[] = "NetStream is now completed";
static const Char RTMP_STREAM_PAUSE_DESC[] = "NetStream is now paused";
static const Char RTMP_STREAM_UNPAUSE_DESC[] = "NetStream is now unpaused";
static const Char RTMP_STREAM_PUBLISH_DESC[] = "NetStream is now published";
static const Char RTMP_STREAM_UNPUBLISH_DESC[] = "NetStream is now unpublished";

extern Bool matchAV(const Chunk* o1, const Chunk* o2);
extern Bool creatAV(const Chunk* src, Chunk* dst);
extern Bool joinAV(const Chunk* src1, const Chunk* src2, 
    Chunk* dst, Byte delim);
extern Void freeAV(Chunk* chunk);

class Parser;
class Builder;

class AmfPayload {
public:
    template<typename T>
    T to_value(const Void* p) {
        const T* pt = (const T*)p;
        return *pt;
    }
    
    Void dump(const Char* promt, const AMFObject* obj, Int32 indent = 0);

    Bool encode(AMFObject* obj, Chunk* chunk);
    Bool encodeObj(Builder* builder, AMFObject* obj);
    Bool encodeEcmaArray(Builder* builder, AMFObject* obj);
    Bool encodeStrictArray(Builder* builder, AMFObject* obj);
    Bool encodeProp(Builder* builder, AMFObjectProperty *prop, 
        Bool hasName);
    Bool encodeArray(Builder* builder, Uint32 cnt,
        AMFObjectProperty* props, Bool hasName);
    
    Bool decode(AMFObject* obj, Byte* payload, Int32 len);
    Bool decodeObj(Parser* parser, AMFObject* obj);
    Bool decodeEcmaArray(Parser* parser, AMFObject* obj);
    Bool decodeStrictArray(Parser* parser, AMFObject* obj);
    Bool decodeArray(Parser* parser, AMFObject* obj, Uint32 cnt);
    Bool decodeProp(Parser* parser, AMFObjectProperty *prop, Bool hasName);

    Void addProp(AMFObject* obj, const AMFObjectProperty *prop);
    Void addPropAny(AMFObject* obj, const Chunk* name,
        AMFDataType type, const Void* any);

    Bool delProp(AMFObject* obj, Int32 index);
    
    inline Int32 countProp(AMFObject* obj) {
        return obj->m_num;
    }

    AMFObjectProperty* findByIndex(AMFObject* obj, Int32 index);
    AMFObjectProperty* findByName(AMFObject* obj, const Chunk* name);

    Chunk* getName(AMFObjectProperty* prop);
    double* getNum(AMFObjectProperty* prop);
    Chunk* getString(AMFObjectProperty* prop);
    Int32* getBool(AMFObjectProperty* prop);
    AMFObject* getObj(AMFObjectProperty* prop);

    Chunk* indexPropName(AMFObject* obj, Int32 index);
    double* indexPropNum(AMFObject* obj, Int32 index);
    Chunk* indexPropString(AMFObject* obj, Int32 index);
    Int32* indexPropBool(AMFObject* obj, Int32 index);
    AMFObject* indexPropObj(AMFObject* obj, Int32 index);

    Chunk* findPropName(AMFObject* obj, const Chunk* name);
    double* findPropNum(AMFObject* obj, const Chunk* name);
    Chunk* findPropString(AMFObject* obj, const Chunk* name);
    Int32* findPropBool(AMFObject* obj, const Chunk* name);
    AMFObject* findPropObj(AMFObject* obj, const Chunk* name);

    /* just free the current level properties */
    Void resetObj(AMFObject* obj);

    /* recursively free the whole properties of obj */
    Void freeObj(AMFObject* obj);

    /* recursively free the whole property if type is obj */
    Void freeProp(AMFObjectProperty* prop);

    Int32 peekString(Byte* data, Int32 len, Chunk* chunk);

private:
    static const Int32 MAX_AMF_BUFFER_SIZE = 0x10000;
    Byte m_buffer[MAX_AMF_BUFFER_SIZE];
};

#endif

