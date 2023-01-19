#ifndef __CONFIG_H__
#define __CONFIG_H__
#include<map>
#include<string>
#include"sysoper.h"


class ConfParser {
    static const Int32 MAX_LINE_LEN = 128;
    
    typedef std::string typeStr;
    typedef std::map<typeStr, typeStr> typeMap;
    typedef typeMap::iterator typeMapItr;
    typedef std::map<typeStr, typeMap> typeConf;
    typedef typeConf::iterator typeConfItr; 
    
public:
    Int32 parse(const Char* path);
    Int32 getParamInt(const Char secName[], const Char key[], Int32* val);
    Int32 getParamStr(const Char secName[], const Char key[],
        Char val[], Int32 maxlen);

private: 
    Void strip(Char* text);
    Void stripComment(Char* text);
    int getKeyVal(const Char* line, Char* key, Int32 key_len,
        Char* val, Int32 val_len);
    Bool getSection(Char line[]);

    Int32 getKeyStr(typeMap& imap, const Char key[],
        Char val[], Int32 maxlen);
    
    Int32 getKeyInt(typeMap& imap, const Char key[], Int32* val);
    
private:
    typeConf m_conf; 
};

#endif

