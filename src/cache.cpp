#include"globaltype.h"
#include"cache.h"
#include"datatype.h"
#include"msgtype.h"


/* these are internal msgs communicate with threads */
struct _MsgExch {
    Int64 m_val;
};

struct Cache {
    Int32 m_ref;
    Int32 m_cap;
    Byte  m_buf[0];
};

/* this structure is aligned by 8 bytes */
struct CacheHdr {
    list_node m_node; 
    Cache* m_cache;
    Int32 m_type;
    Int32 m_buff_size; 
    Byte m_buff[0]; // buffer allocates      
};

CacheHdr* CacheCenter::creat(Int32 type, Int32 buff_size) { 
    Int32 total = 0;
    Byte* ptr = NULL;
    CacheHdr* hdr = NULL;

    total = (Int32)(sizeof(CacheHdr) + buff_size + 1);
    ARR_NEW(Byte, total, ptr);
    if (NULL != ptr) {
        zero(ptr, sizeof(CacheHdr));

        hdr = (CacheHdr*)ptr;
        INIT_LIST_NODE(&hdr->m_node); 
        hdr->m_buff_size = buff_size;
        hdr->m_type = type;
        hdr->m_buff[buff_size] = DEF_DELIM_END_VALID; 
        
        return hdr; 
    } else { 
        return NULL;
    }
}

Void CacheCenter::free(CacheHdr* hdr) { 
    Byte* ptr = (Byte*)hdr;
    
    if (NULL != ptr) {
        if (DEF_DELIM_END_VALID == hdr->m_buff[hdr->m_buff_size]) {
            hdr->m_buff[hdr->m_buff_size] = DEF_DELIM_END_INVALID;

            if (NULL != hdr->m_cache) {
                del(hdr->m_cache);
                hdr->m_cache= NULL;
            }
            
            ARR_FREE(ptr);
        } else {
            LOG_ERROR("*******del_cachehdr| hdr=%p| size=%d| type=%d|"
                " msg=cachehdr invalid|",
                hdr, hdr->m_buff_size, hdr->m_type);
        }
    }
}

CacheHdr* CacheCenter::cast(list_node* node) {
    CacheHdr* hdr = NULL;

    hdr = list_entry(node, CacheHdr, m_node);
    return hdr;
}

void CacheCenter::queue(CacheHdr* hdr, list_head* list) { 
    list_add_back(&hdr->m_node, list); 
}

Byte* CacheCenter::buffer(CacheHdr* hdr) {
    return hdr->m_buff;
} 

Int32 CacheCenter::getType(CacheHdr* hdr) {
    return hdr->m_type;
}

Void CacheCenter::setType(CacheHdr* hdr, Int32 type) {
    hdr->m_type = type;
}

Cache* CacheCenter::getCache(CacheHdr* hdr) {
    return hdr->m_cache;
}

Void CacheCenter::setCache(CacheHdr* hdr, Cache* cache) {
    hdr->m_cache = cache;
}

Cache* CacheCenter::alloc(Int32 capacity) {
    Int32 total = sizeof(Cache) + capacity + 1;
    Byte* ptr = NULL;
    Cache* cache = NULL;

    ARR_NEW(Byte, total, ptr);
    if (NULL != ptr) {  
        zero(ptr, sizeof(Cache));
        
        cache = (Cache*)ptr;
        cache->m_ref = 1;
        cache->m_cap = capacity; 
        cache->m_buf[capacity] = DEF_DELIM_END_VALID;

        return cache;
    } else {
        return NULL;
    } 
}

void CacheCenter::del(Cache* cache) {
    Int32 ref = 0;
    Byte* ptr = NULL;
    
    if (NULL != cache) {
        if (DEF_DELIM_END_VALID == cache->m_buf[cache->m_cap]) {
            ref = ATOMIC_DEC(&cache->m_ref);
            if (0 < ref) {
                return;
            } else {
                cache->m_buf[cache->m_cap] = DEF_DELIM_END_INVALID;
                
                ptr = (Byte*)cache;
                ARR_FREE(ptr);
            }
        } else {
            LOG_ERROR("*******del_cache| cache=%p| ref=%d| cap=%d|"
                " delim=0x%x| msg=cache invalid|",
                cache, cache->m_ref, cache->m_cap, 
                cache->m_buf[cache->m_cap]);
        }
    }
}

Cache* CacheCenter::ref(Cache* cache) {
    ATOMIC_INC(&cache->m_ref);

    return cache;
} 

Byte* CacheCenter::data(Cache* cache) {
    return cache->m_buf;
}

Void CacheCenter::zero(Void* data, Int32 len) {
    memset(data, 0, len);
}

Void CacheCenter::copy(Void* dst, const Void* src, Int32 len) {
    memcpy(dst, src, len);
}

CacheHdr* MsgCenter::creatExchMsg(Int32 type, Int64 val) {
    CacheHdr* hdr = NULL;
    _MsgExch* msg = NULL;

    hdr = creatMsg<_MsgExch>(type);
    msg = body<_MsgExch>(hdr);
    msg->m_val = val;
    
    return hdr; 
} 

Int64 MsgCenter::getExchMsgVal(CacheHdr* hdr) {
    _MsgExch* msg = NULL;

    msg = body<_MsgExch>(hdr); 
    return msg->m_val;
}

Bool MsgCenter::chkSysMsg(CacheHdr* hdr) {
    return hdr->m_type < ENUM_MSG_TYPE_SYSTEM_MAX;
}

