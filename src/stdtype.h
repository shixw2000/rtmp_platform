#ifndef __STDTYPE_H__
#define __STDTYPE_H__
#include<string>
#include<map>
#include<vector>
#include"sysoper.h"


typedef std::string typeString;
typedef std::size_t typeSize;
typedef std::vector<typeString> typeVec;


typedef std::map<typeString, typeString> typeFields;
typedef typeFields::iterator typeItr;
typedef typeFields::const_iterator typeItrConst;


static const typeSize DEF_STR_END = typeString::npos;
static const typeString DEF_RTSP_EMPTY_STR = "";
static const typeString DEF_BLANK_TOKEN = " \t";
static const typeString DEF_QUATION_TOKEN = "=";


/*** begin: these definitions are used by rtmp */
struct RtmpUint;
struct RtmpStream;

typedef std::map<Uint32, RtmpUint*> typeMapUnit;
typedef typeMapUnit::iterator typeMapUnitItr;
typedef typeMapUnit::const_iterator typeMapUnitItrConst;


typedef std::map<typeString, RtmpStream*> typeMapStream;
typedef typeMapStream::iterator typeMapStreamItr;
typedef typeMapStream::const_iterator typeMapStreamItrConst;

/*** end ***/


/*** Begin: these definitions are used by rtsp */
struct Aggregate;
struct Presentation;
struct MediaInfo;
struct Session;
struct IpInfo;
struct TransInfo;

struct StrCmp {
    bool operator()(const Char* str1, const Char* str2) const;
};

typedef std::map<const Char*, Presentation*, StrCmp> typeMapRes;
typedef typeMapRes::iterator typeResItr;
typedef typeMapRes::const_iterator typeResItrConst;

typedef std::map<const Char*, Aggregate*, StrCmp> typeMapAggr;
typedef typeMapAggr::iterator typeAggrItr;
typedef typeMapAggr::const_iterator typeAggrItrConst;

typedef std::map<const Char*, Session*, StrCmp> typeMapSess;
typedef typeMapSess::iterator typeSessItr;
typedef typeMapSess::const_iterator typeSessItrConst;

struct TransCmp {
    bool operator()(const IpInfo* ip1, const IpInfo* ip2) const;
};

typedef std::map<const IpInfo*, TransInfo*, TransCmp> typeMapTrans;
typedef typeMapTrans::iterator typeTransItr;
typedef typeMapTrans::const_iterator typeTransItrConst;


struct TokenCmp {
    bool operator()(const Token* token1, const Token* token2) const;
};

typedef std::map<const Token*, const Token*, TokenCmp> typeMapToken;
typedef typeMapToken::iterator typeTokenItr;
typedef typeMapToken::const_iterator typeTokenItrConst;


/*** end ***/

#endif

