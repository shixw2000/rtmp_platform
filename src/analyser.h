#ifndef __ANALYSER_H__
#define __ANALYSER_H__
#include"globaltype.h"
#include"datatype.h"


class Parser {
public:
    Parser();
    Parser(Byte* data, Int32 len);
    explicit Parser(const Chunk* chunk);
    
    Void reset(const Chunk* chunk);
    Void reset(Byte* data, Int32 len);
    
    Int32 available();
    Int32 used();

    Bool eoo();
    Void skip(Int32 n);

    Bool parseBool(Int32* output);
    Bool parse8(Byte* output);
    Bool parse16(Uint16* output);
    Bool parse24(Uint32* output);
    Bool parse32(Uint32* output);
    Bool parse32LE(Uint32* output);
    Bool parseNum(double* output);
    
    Bool parseString(Chunk*);
    Bool parseLString(Chunk* chunk);

private:
    Void read(Byte* val);

private:
    Chunk m_chunk;
    Int32 m_upto;
};

class Builder {
public: 
    Builder();
    Builder(Byte* data, Int32 len);
    explicit Builder(const Chunk* chunk);

    Void reset(const Chunk* chunk);
    Void reset(Byte* data, Int32 len);

    Int32 available();
    Int32 used();

    Void skip(Int32 n);
    
    Bool buildBool(Int32 val);
    Bool build8(const Byte* input);
    Bool build16(const Uint16* input); 
    Bool build24(const Uint32* input); 
    Bool build32(const Uint32* input); 
    Bool build32LE(const Uint32* input); 
    Bool buildNum(const double* input);
    Bool buildBytes(const Void* input, Uint32 len);

    Bool buildString(const Chunk* chunk);
    Bool buildLString(const Chunk* chunk);

private:
    Void write(const Byte* input);

private:
    Chunk m_chunk;  
    Int32 m_upto;
};

#endif

