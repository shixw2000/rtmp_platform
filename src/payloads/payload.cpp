#include<stdlib.h>
#include<string.h>
#include"globaltype.h"
#include"payload.h"
#include"analyser.h"
#include"cache.h"


Bool matchAV(const Chunk* o1, const Chunk* o2) {
    if (o1->m_size == o2->m_size 
        && 0 == memcmp(o1->m_data, o2->m_data, o1->m_size)) {
        return TRUE;
    } else {
        return FALSE;
    }
}

Bool creatAV(const Chunk* src, Chunk* dst) {
    if (0 < src->m_size) {
        dst->m_data = (Byte*)malloc(src->m_size);
        if (NULL != dst->m_data) {
            memcpy(dst->m_data, src->m_data, src->m_size);
            dst->m_data[src->m_size] = '\0';
            dst->m_size = src->m_size;
            
            return TRUE;
        } else {
            dst->m_size = 0;
            
            return FALSE;
        }
    } else {
        dst->m_size = 0;
        dst->m_data = NULL;

        return TRUE;
    }
}

Bool joinAV(const Chunk* src1, const Chunk* src2, Chunk* dst, Byte delim) {
    Byte* ptr = NULL;
    Int32 total = 0;

    if (NULL_CHUNK_DELIM != delim) {
        total = src1->m_size + src2->m_size + 1;
    } else {
        total = src1->m_size + src2->m_size;
    }
    
    dst->m_data = (Byte*)malloc(total);
    if (NULL != dst->m_data) {
        ptr = dst->m_data;
        
        if (0 < src1->m_size) {
            memcpy(ptr, src1->m_data, src1->m_size);

            ptr += src1->m_size;
        }

        if (NULL_CHUNK_DELIM != delim) {
            *(ptr++) = delim;;
        }

        if (0 < src2->m_size) {
            memcpy(ptr, src2->m_data, src2->m_size);
        } 
        
        dst->m_size = total;
        
        return TRUE;
    } else {
        dst->m_size = 0;
        
        return FALSE;
    }
}

Void freeAV(Chunk* chunk) {
    if (0 < chunk->m_size) {
        chunk->m_size = 0;

        free(chunk->m_data);
        chunk->m_data = NULL;
    }
}

Bool AmfPayload::decode(AMFObject* obj, Byte* payload, Int32 len) {
    Bool bOk = FALSE;
    AMFObjectProperty prop;
    Parser parser(payload, len); 
    
    while (0 < parser.available()) {
        memset(&prop, 0, sizeof(prop));
        
        bOk = decodeProp(&parser, &prop, FALSE);
        if (bOk) {
            addProp(obj, &prop);
        } else {
            break;
        }
    }

    return bOk;
}

Void AmfPayload::dump(const Char* promt, const AMFObject* obj, Int32 indent) {
    const Char DEF_SPACE[] = "";
    const Int32 nspace = 2 * indent;
    const Char* name = NULL;
    const AMFObjectProperty* prop = NULL;
    
    LOG_INFO("%*s====%s: obj_begin| num=%d|", 
        nspace, DEF_SPACE, promt, obj->m_num);

    for (int i=0; i<obj->m_num; ++i) {
        prop = &obj->m_props[i];

        if (0 < prop->m_name.m_size) {
            name = (const Char*)prop->m_name.m_data;
        } else {
            name = "";
        }
        
        switch (prop->m_type) {
        case AMF_NUMBER:
            LOG_INFO("%*s[%d]property| name=%.*s| number=%.2f|", 
                nspace, DEF_SPACE, i, 
                prop->m_name.m_size, name, prop->p_vu.m_number);
            break;

        case AMF_BOOLEAN:
            LOG_INFO("%*s[%d]property| name=%.*s| boolean=%s|", 
                nspace, DEF_SPACE, i, 
                prop->m_name.m_size, name, 
                prop->p_vu.m_n ? "TRUE" : "FALSE");
            break;

        case AMF_STRING:
            LOG_INFO("%*s[%d]property| name=%.*s| string=(%d)%.*s|", 
                nspace, DEF_SPACE, i, 
                prop->m_name.m_size, name, 
                prop->p_vu.m_val.m_size,
                prop->p_vu.m_val.m_size,
                (const Char*)prop->p_vu.m_val.m_data);
            break;

        case AMF_OBJECT:
            LOG_INFO("%*s[%d]property| name=%.*s| object|", 
                nspace, DEF_SPACE, i, 
                prop->m_name.m_size, name);
            dump(">", &prop->p_vu.m_object, ++indent);
            break;

        case AMF_ECMA_ARRAY:
            LOG_INFO("%*s[%d]property| name=%.*s| ecma_array|", 
                nspace, DEF_SPACE, i, 
                prop->m_name.m_size, name);
            dump(">", &prop->p_vu.m_object, ++indent);
            break;

        case AMF_STRICT_ARRAY:
            LOG_INFO("%*s[%d]property| name=%.*s| strict_array|", 
                nspace, DEF_SPACE, i, 
                prop->m_name.m_size, name);
            dump(">", &prop->p_vu.m_object, ++indent);
            break;
  
        case AMF_NULL:
            LOG_INFO("%*s[%d]property| name=%.*s| type=NULL|", 
                nspace, DEF_SPACE, i, 
                prop->m_name.m_size, name);
            break;

        default:
            LOG_INFO("%*s[%d]property| name=%.*s| type=0x%x|", 
                nspace, DEF_SPACE, i, 
                prop->m_name.m_size, name, 
                prop->m_type);
            break;
        }
    }
    
    LOG_INFO("%*sobj_end|", nspace, DEF_SPACE);
}

Bool AmfPayload::decodeObj(Parser* parser, AMFObject* obj) {
    Bool bOk = FALSE;
    Bool bEnd = FALSE; 
    AMFObjectProperty prop;

    while (0 < parser->available()) {
        bEnd = parser->eoo();
        if (!bEnd) {
            memset(&prop, 0, sizeof(prop));
            
            bOk = decodeProp(parser, &prop, TRUE);
            if (bOk) {
                addProp(obj, &prop);
            } else {
                break;
            }
        } else {
            /* skip eoo */
            parser->skip(3);
            bOk = TRUE;
            break;
        }
    }

    return bOk;
}

Bool AmfPayload::decodeEcmaArray(Parser* parser, AMFObject* obj) {
    Bool bOk = FALSE;
    Uint32 cnt = 0;

    bOk = parser->parse32(&cnt);
    if (bOk) { 
        bOk = decodeObj(parser, obj);
    } 

    return bOk;
}

Bool AmfPayload::decodeStrictArray(Parser* parser, AMFObject* obj) {
    Bool bOk = FALSE;
    Uint32 cnt = 0;

    bOk = parser->parse32(&cnt);
    if (bOk) { 
        bOk = decodeArray(parser, obj, cnt); 
    } 

    return bOk;
}

Bool AmfPayload::decodeArray(Parser* parser, AMFObject* obj, Uint32 cnt) {
    Bool bOk = TRUE; // here must be TRUE for cnt == 0
    AMFObjectProperty prop;
   
    while (0 < cnt--) {
        memset(&prop, 0, sizeof(prop));
        
        bOk = decodeProp(parser, &prop, FALSE);
        if (bOk) {
            addProp(obj, &prop);
        } else {
            break;
        }
    } 

    return bOk;
}

Bool AmfPayload::decodeProp(Parser* parser,
    AMFObjectProperty *prop, Bool hasName) {
    Bool bOk = FALSE;
    Byte ch = 0;   

    prop->m_type = AMF_INVALID;
    
    if (hasName) {
        bOk = parser->parseString(&prop->m_name);
        if (!bOk || !prop->m_name.m_size) {
            /* name must has len > 0 */
            return FALSE;
        }
    } else {
        prop->m_name.m_size = 0;
    }

    bOk = parser->parse8(&ch);
    if (!bOk) { 
        return bOk;
    }

    switch (ch) {
    case AMF_NUMBER:
        prop->m_type = AMF_NUMBER;
        bOk = parser->parseNum(&prop->p_vu.m_number);
        break;

    case AMF_BOOLEAN:
        prop->m_type = AMF_BOOLEAN;
        bOk = parser->parseBool(&prop->p_vu.m_n);
        break;

    case AMF_STRING:
        prop->m_type = AMF_STRING;
        bOk = parser->parseString(&prop->p_vu.m_val);
        break; 

    case AMF_LONG_STRING:
        prop->m_type = AMF_STRING;
        bOk = parser->parseLString(&prop->p_vu.m_val); 
        break;

    case AMF_OBJECT:
        prop->m_type = AMF_OBJECT;
        bOk = decodeObj(parser, &prop->p_vu.m_object);
        break;

    case AMF_ECMA_ARRAY:
        prop->m_type = AMF_OBJECT;
        bOk = decodeEcmaArray(parser, &prop->p_vu.m_object);
        break;

    case AMF_STRICT_ARRAY:
        prop->m_type = AMF_OBJECT;
        bOk = decodeStrictArray(parser, &prop->p_vu.m_object);
        break;

    case AMF_NULL:
        prop->m_type = AMF_NULL;
        break; 
        
    default:
        prop->m_type = AMF_INVALID;
        bOk = FALSE;
        break;
    }

    return bOk;
}

Void AmfPayload::addProp(AMFObject* obj, const AMFObjectProperty *prop) {
    if (!(obj->m_num & AMF_GROWTH_PROP_MASK)) {
        obj->m_props = (AMFObjectProperty*)realloc(obj->m_props,
            (obj->m_num + AMF_GROWTH_PROP_SIZE) * sizeof(AMFObjectProperty)); 
    } 
    
    memcpy(&obj->m_props[obj->m_num++], prop, sizeof(AMFObjectProperty)); 
}

template<AMFDataType type, typename T>
Void AmfPayload::addPropAny(AMFObject* obj, 
    const Chunk* name, const T* any) {
    AMFObjectProperty prop = AMF_PROP_INVALID;
    
    CacheCenter::zero(&prop, sizeof(prop));
    
    if (NULL != name) {
        prop.m_name = *name;
    } else {
        prop.m_name.m_size = 0;
        prop.m_name.m_data = NULL;
    }

    prop.m_type = type;
    CacheCenter::copy(&prop.p_vu, any, sizeof(T));
    addProp(obj, &prop);
}

template Void AmfPayload::addPropAny<AMF_NUMBER, double>(AMFObject* obj,
    const Chunk* name, const double* any);
template Void AmfPayload::addPropAny<AMF_BOOLEAN, Int32>(AMFObject* obj,
    const Chunk* name, const Int32* any);
template Void AmfPayload::addPropAny<AMF_STRING, Chunk>(AMFObject* obj,
    const Chunk* name, const Chunk* any);
template Void AmfPayload::addPropAny<AMF_OBJECT, AMFObject>(AMFObject* obj,
    const Chunk* name, const AMFObject* any);

template<>
Void AmfPayload::addPropAny<AMF_NULL, Void>(AMFObject* obj, 
    const Chunk* name, const Void*) {
    AMFObjectProperty prop = AMF_PROP_INVALID;
    
    CacheCenter::zero(&prop, sizeof(prop));
    
    if (NULL != name) {
        prop.m_name = *name;
    } else {
        prop.m_name.m_size = 0;
        prop.m_name.m_data = NULL;
    }

    prop.m_type = AMF_NULL;
    addProp(obj, &prop);
}

template<>
Void AmfPayload::addPropAny<AMF_INVALID, Void>(AMFObject*, 
    const Chunk*, const Void* ) {
    /* do nothing */
}

Bool AmfPayload::delProp(AMFObject* obj, Int32 index) {
    Int32 n = 0;
    
    if (obj->m_num > index && 0 <= index) {
        /* release the resource of prop */
        freeProp(&obj->m_props[index]);
        
        --obj->m_num;
        if (index < obj->m_num) {
            n = obj->m_num - index;
            memmove(&obj->m_props[index], &obj->m_props[index+1], n); 
        } 
        
        /* clear the end */
        memset(&obj->m_props[obj->m_num], 0, sizeof(AMFObjectProperty));

        return TRUE;
    } else {
        return FALSE;
    }
} 

AMFObjectProperty* AmfPayload::findByIndex(AMFObject* obj,
    Int32 index) {
    if (obj->m_num > index) {
        return &obj->m_props[index];
    } else {
        return NULL;
    }
}

AMFObjectProperty* AmfPayload::findByName(AMFObject* obj, 
    const Chunk* name) {
    Bool found = FALSE;
    
    for (int i=0; i<obj->m_num; ++i) {
        found = matchAV(name, &obj->m_props[i].m_name);
        if (found) {
            return &obj->m_props[i];
        }
    }

    return NULL;
}

Chunk* AmfPayload::getName(AMFObjectProperty* prop) {
    if (NULL != prop) {
        return &prop->m_name;
    } else {
        return NULL;
    }
}

double* AmfPayload::getNum(AMFObjectProperty* prop) {
    if (NULL != prop && AMF_NUMBER == prop->m_type) {
        return &prop->p_vu.m_number;
    } else {
        return NULL;
    }
}

Chunk* AmfPayload::getString(AMFObjectProperty* prop) {
    if (NULL != prop && AMF_STRING == prop->m_type) {
        return &prop->p_vu.m_val;;
    } else {
        return NULL;
    }
}

Int32* AmfPayload::getBool(AMFObjectProperty* prop) {
    if (NULL != prop && AMF_BOOLEAN == prop->m_type) {
        return &prop->p_vu.m_n;
    } else {
        return NULL;
    }
}

AMFObject* AmfPayload::getObj(AMFObjectProperty* prop) {
    if (NULL != prop && AMF_OBJECT == prop->m_type) {
        return &prop->p_vu.m_object;
    } else {
        return NULL;
    }
}

Chunk* AmfPayload::indexPropName(AMFObject* obj, Int32 index) {
    AMFObjectProperty* prop = NULL;

    prop = findByIndex(obj, index);
    return getName(prop);
}

double* AmfPayload::indexPropNum(AMFObject* obj, Int32 index) {
    AMFObjectProperty* prop = NULL;

    prop = findByIndex(obj, index);
    return getNum(prop);
}

Chunk* AmfPayload::indexPropString(AMFObject* obj, Int32 index) {
    AMFObjectProperty* prop = NULL;

    prop = findByIndex(obj, index);
    return getString(prop);
}

Int32* AmfPayload::indexPropBool(AMFObject* obj, Int32 index) {
    AMFObjectProperty* prop = NULL;

    prop = findByIndex(obj, index);
    return getBool(prop);
}

AMFObject* AmfPayload::indexPropObj(AMFObject* obj, Int32 index) {
    AMFObjectProperty* prop = NULL;

    prop = findByIndex(obj, index);
    return getObj(prop);
}

Chunk* AmfPayload::findPropName(AMFObject* obj, const Chunk* name) {
    AMFObjectProperty* prop = NULL;

    prop = findByName(obj, name);
    return getName(prop);
}

double* AmfPayload::findPropNum(AMFObject* obj, const Chunk* name) {
    AMFObjectProperty* prop = NULL;

    prop = findByName(obj, name);
    return getNum(prop);
}

Chunk* AmfPayload::findPropString(AMFObject* obj, const Chunk* name) {
    AMFObjectProperty* prop = NULL;

    prop = findByName(obj, name);
    return getString(prop);
}

Int32* AmfPayload::findPropBool(AMFObject* obj, const Chunk* name) {
    AMFObjectProperty* prop = NULL;

    prop = findByName(obj, name);
    return getBool(prop);
}

AMFObject* AmfPayload::findPropObj(AMFObject* obj, const Chunk* name) {
    AMFObjectProperty* prop = NULL;

    prop = findByName(obj, name);
    return getObj(prop);
}

Void AmfPayload::resetObj(AMFObject* obj) {
    if (0 < obj->m_num) { 
        free(obj->m_props); 
        obj->m_props = NULL;
        obj->m_num = 0;
    }
}

Void AmfPayload::freeObj(AMFObject* obj) {
    if (0 < obj->m_num) {
        for (int i=0; i<obj->m_num; ++i) {
            freeProp(&obj->m_props[i]);
        }
        
        free(obj->m_props); 
        obj->m_props = NULL;
        obj->m_num = 0;
    }
}

Void AmfPayload::freeProp(AMFObjectProperty* prop) {
    if (AMF_OBJECT == prop->m_type || AMF_ECMA_ARRAY == prop->m_type
        || AMF_STRICT_ARRAY == prop->m_type) {
        freeObj(&prop->p_vu.m_object);
    } else {
        prop->p_vu.m_val.m_size = 0;
        prop->p_vu.m_val.m_data = NULL;
    }

    prop->m_type = AMF_INVALID;
}

Bool AmfPayload::encode(AMFObject* obj, Chunk* chunk) {
    Bool bOk = FALSE;
    Uint32 cnt = 0;
    Builder builder(m_buffer, MAX_AMF_BUFFER_SIZE);

    chunk->m_size = 0;
    chunk->m_data = NULL;
    
    if (0 < obj->m_num) {
        cnt = (Uint32)obj->m_num;
        
        bOk = encodeArray(&builder, cnt, obj->m_props, FALSE);
        if (bOk) {
            chunk->m_size = builder.used();
            chunk->m_data = m_buffer;
        } 
    }

    return bOk;
}

Bool AmfPayload::encodeObj(Builder* builder, AMFObject* obj) {
    Bool bOk = FALSE;
    Uint32 cnt = 0;
    
    if (0 <= obj->m_num) {
        cnt = (Uint32)obj->m_num;
        
        bOk = encodeArray(builder, cnt, obj->m_props, TRUE);
        if (bOk) {
            bOk = builder->build24(&AMF_END_OF_OBJ);
        }
    }
    
    return bOk;
}

Bool AmfPayload::encodeStrictArray(Builder* builder, AMFObject* obj) {
    Bool bOk = FALSE;
    Uint32 cnt = 0;

    if (0 <= obj->m_num) {
        cnt = (Uint32)obj->m_num;
        
        bOk = builder->build32(&cnt);
        if (bOk) { 
            bOk = encodeArray(builder, cnt, obj->m_props, FALSE);
        } 
    }

    return bOk;
}

Bool AmfPayload::encodeEcmaArray(Builder* builder, AMFObject* obj) {
    Bool bOk = FALSE;
    Uint32 cnt = 0;

    if (0 <= obj->m_num) {
        cnt = (Uint32)obj->m_num;
        
        bOk = builder->build32(&cnt);
        if (bOk) { 
            bOk = encodeObj(builder, obj);
        }
    } 

    return bOk;
}

Bool AmfPayload::encodeArray(Builder* builder, Uint32 cnt,
    AMFObjectProperty *props, Bool hasName) {
    Bool bOk = TRUE; // here must be TRUE, for cnt == 0

    for (Uint32 i=0; i<cnt; ++i) { 
        bOk = encodeProp(builder, &props[i], hasName);
    }

    return bOk;
}

Bool AmfPayload::encodeProp(Builder* builder, 
    AMFObjectProperty *prop, Bool hasName) {
    Bool bOk = FALSE;
    Byte ch = 0;

    if (hasName) {
        /* the name must has len > 0 */
        if (0 < prop->m_name.m_size) {
            builder->buildString(&prop->m_name);
        } else {
            return FALSE;
        }
    } 

    ch = (Byte)prop->m_type;
    switch (prop->m_type) {
    case AMF_NUMBER:
        builder->build8(&ch);
        bOk = builder->buildNum(&prop->p_vu.m_number);
        break;

    case AMF_BOOLEAN:
        builder->build8(&ch);
        bOk = builder->buildBool(prop->p_vu.m_n);
        break;

    case AMF_STRING:
        if (AMF_MAX_STRING_SIZE > prop->p_vu.m_val.m_size) {
            builder->build8(&ch);
            bOk = builder->buildString(&prop->p_vu.m_val);
        } else {
            ch = (Byte)AMF_LONG_STRING;
            builder->build8(&ch);
            bOk = builder->buildLString(&prop->p_vu.m_val);
        }
        break;

    case AMF_OBJECT:
        builder->build8(&ch);
        bOk = encodeObj(builder, &prop->p_vu.m_object);
        break;

    case AMF_ECMA_ARRAY:
        builder->build8(&ch);
        bOk = encodeEcmaArray(builder, &prop->p_vu.m_object);
        break;

    case AMF_STRICT_ARRAY:
        builder->build8(&ch);
        bOk = encodeStrictArray(builder, &prop->p_vu.m_object);
        break;

    case AMF_NULL:
    case AMF_UNDEFINED:
        bOk = builder->build8(&ch);
        break;

    default:
        bOk = FALSE;
        break;
    }

    return bOk;
}

Int32 AmfPayload::peekString(Byte* data, Int32 len, Chunk* chunk) {
    Int32 cnt = 0;
    Bool bOk = FALSE;
    Chunk* p = NULL;
    AMFObjectProperty prop = AMF_PROP_INVALID;
    Parser parser(data, len);

    bOk = decodeProp(&parser, &prop, FALSE);
    if (bOk) {
        p = getString(&prop);
        if (NULL != p) {
            *chunk = *p;
            cnt = parser.used();
        } else {
            cnt = -1;
        }
    } else {
        cnt = -1;
    }

    return cnt;
}
