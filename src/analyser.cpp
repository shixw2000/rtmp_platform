#include<arpa/inet.h>
#include<stdlib.h>
#include"analyser.h"
#include"cache.h"


Parser::Parser() {
    reset(NULL);
}

Parser::Parser(Byte* data, Int32 len) {
    reset(data, len);
}

Parser::Parser(const Chunk* chunk) {
    reset(chunk);
}

Void Parser::reset(const Chunk* chunk) {
    if (NULL != chunk) {
        m_chunk.m_data = chunk->m_data;
        m_chunk.m_size = chunk->m_size;
    } else {
        m_chunk.m_data = NULL;
        m_chunk.m_size = 0;
    }

    m_upto = 0;
}

Void Parser::reset(Byte* data, Int32 len) {
    m_chunk.m_data = data;
    m_chunk.m_size = len;
    m_upto = 0;
}

Int32 Parser::available() {
    if (0 < m_chunk.m_size) {
        return m_chunk.m_size;
    } else {
        return 0;
    }
}

Int32 Parser::used() {
    return m_upto;
}

Bool Parser::eoo() {
    if (0 == m_chunk.m_data[0] && 0 == m_chunk.m_data[1]
        && AMF_END_OF_OBJ == m_chunk.m_data[2]) {
        return TRUE;
    } else {
        return FALSE;
    }
}

Void Parser::skip(Int32 n) {
    m_chunk.m_data += n;
    m_chunk.m_size -= n;

    m_upto += n;
}

Bool Parser::parseBool(Int32* output) {  
    Byte ch = 0;
    
    if (1 <= available()) {
        read(&ch);

        *output = ch ? TRUE : FALSE;
        return TRUE;
    } else {
        return FALSE;
    }
}

Bool Parser::parse8(Byte* output) {       
    if (1 <= available()) {
        read(output);
        
        return TRUE;
    } else {
        return FALSE;
    }
}

Bool Parser::parse16(Uint16* output) {   
    Byte* p = (Byte*)output;
    
    if (2 <= available()) {
        read(&p[1]);
        read(&p[0]);
        
        return TRUE;
    } else {
        return FALSE;
    }
}

Bool Parser::parse24(Uint32* output) {  
    Byte* p = (Byte*)output;
    
    if (3 <= available()) {
        p[3] = 0;
        read(&p[2]);
        read(&p[1]);
        read(&p[0]);
        
        return TRUE;
    } else {
        return FALSE;
    }
}

Bool Parser::parse32(Uint32* output) {
    Byte* p = (Byte*)output;
    
    if (4 <= available()) {
        read(&p[3]);
        read(&p[2]);
        read(&p[1]);
        read(&p[0]);
        
        return TRUE;
    } else {
        return FALSE;
    }
}

Bool Parser::parse32LE(Uint32* output) {
    Byte* p = (Byte*)output;
    
    if (4 <= available()) {
        read(&p[0]);
        read(&p[1]);
        read(&p[2]);
        read(&p[3]);
        
        return TRUE;
    } else {
        return FALSE;
    }
}

Bool Parser::parseNum(double* output) {
    Byte* p = (Byte*)output;
    
    if (8 <= available()) {
        read(&p[7]);
        read(&p[6]);
        read(&p[5]);
        read(&p[4]);
        read(&p[3]);
        read(&p[2]);
        read(&p[1]);
        read(&p[0]);
        
        return TRUE;
    } else {
        return FALSE;
    }
}

Bool Parser::parseString(Chunk* chunk) {
    union {
        Uint16 u16;
        Byte b[2];
    } v;
    
    if (2 <= available()) {
        v.b[1] = m_chunk.m_data[0];
        v.b[0] = m_chunk.m_data[1];

        if (2 + v.u16 <= available()) {
            chunk->m_size = v.u16;
            chunk->m_data = &m_chunk.m_data[2];

            skip(2 + v.u16);
            return TRUE; 
        } else {
            return FALSE;
        }
    } else {
        return FALSE;
    }
}

Bool Parser::parseLString(Chunk* chunk) {
    union {
        Uint32 u32;
        Byte b[4];
    } v;
    
    if (4 <= available()) {
        v.b[3] = m_chunk.m_data[0];
        v.b[2] = m_chunk.m_data[1];
        v.b[1] = m_chunk.m_data[2];
        v.b[0] = m_chunk.m_data[3];

        if (4 + v.u32 <= (Uint32)available()) {
            chunk->m_size = v.u32;
            chunk->m_data = &m_chunk.m_data[4];

            skip(4 + v.u32);
            return TRUE; 
        } else {
            return FALSE;
        }
    } else {
        return FALSE;
    }
}

Void Parser::read(Byte* val) {
    *val = m_chunk.m_data[0];
    
    skip(1);
}


Builder::Builder() {
    reset(NULL);
}

Builder::Builder(Byte* data, Int32 len) {
    reset(data, len);
}

Builder::Builder(const Chunk* chunk) {
    reset(chunk);
}

Void Builder::reset(const Chunk* chunk) {
    if (NULL != chunk) {
        m_chunk.m_data = chunk->m_data;
        m_chunk.m_size = chunk->m_size;
    } else {
        m_chunk.m_data = NULL;
        m_chunk.m_size = 0;
    }

    m_upto = 0;
}

Void Builder::reset(Byte* data, Int32 len) {
    m_chunk.m_data = data;
    m_chunk.m_size = len;
    m_upto = 0;
}

Int32 Builder::available() {
    if (0 < m_chunk.m_size) {
        return m_chunk.m_size;
    } else {
        return 0;
    }
}

Int32 Builder::used() {
    return m_upto;
}

Void Builder::skip(Int32 n) {
    m_chunk.m_data += n;
    m_chunk.m_size -= n;

    m_upto += n;
}

Bool Builder::buildBool(Int32 val) {
    Byte ch = val ? TRUE : FALSE;
    
    if (1 <= available()) {
        write(&ch);

        return TRUE;
    } else {
        return FALSE;
    }
}

Bool Builder::build8(const Byte* input) {
    if (1 <= available()) {
        write(input);

        return TRUE;
    } else {
        return FALSE;
    }
}

Bool Builder::build16(const Uint16* input) {
    const Byte* p = (const Byte*)input;
    
    if (2 <= available()) {
        write(&p[1]);
        write(&p[0]);

        return TRUE;
    } else {
        return FALSE;
    }
}

Bool Builder::build24(const Uint32* input) {  
    const Byte* p = (const Byte*)input;

    if (3 <= available()) {
        write(&p[2]);
        write(&p[1]);
        write(&p[0]);
    
        return TRUE;
    } else {
        return FALSE;
    }
}

Bool Builder::build32(const Uint32* input) {
    const Byte* p = (const Byte*)input;

    if (4 <= available()) {
        write(&p[3]);
        write(&p[2]);
        write(&p[1]);
        write(&p[0]);
    
        return TRUE;
    } else {
        return FALSE;
    }
}

Bool Builder::build32LE(const Uint32* input) {
    const Byte* p = (const Byte*)input;

    if (4 <= available()) {
        write(&p[0]);
        write(&p[1]);
        write(&p[2]);
        write(&p[3]);
    
        return TRUE;
    } else {
        return FALSE;
    }
}

Bool Builder::buildNum(const double* input) {
    const Byte* p = (const Byte*)input;

    if (8 <= available()) {
        write(&p[7]);
        write(&p[6]);
        write(&p[5]);
        write(&p[4]);
        write(&p[3]);
        write(&p[2]);
        write(&p[1]);
        write(&p[0]);
    
        return TRUE;
    } else {
        return FALSE;
    }
}

Bool Builder::buildBytes(const Void* input, Uint32 len) {    
    if (len <= (Uint32)available()) {
        memcpy(m_chunk.m_data, input, len);
    
        skip(len);
        return TRUE;
    } else {
        return FALSE;
    }
}

Void Builder::write(const Byte* input) {
    m_chunk.m_data[0] = *input;

    skip(1);
}

Bool Builder::buildString(const Chunk* chunk) {      
    Uint16 u16 = chunk->m_size;

    if (2 + u16 < (Int32)available()) {
        build16(&u16);
        buildBytes(chunk->m_data, chunk->m_size);

        return TRUE;
    } else {
        return FALSE;
    }
}

Bool Builder::buildLString(const Chunk* chunk) {      
    Uint32 u32 = chunk->m_size;

    if (u32 + 4 < (Uint32)available()) {
        build32(&u32);
        buildBytes(chunk->m_data, u32);

        return TRUE;
    } else {
        return FALSE;
    }
}


