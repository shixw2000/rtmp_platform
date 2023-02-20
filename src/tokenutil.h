#ifndef __TOKENUTIL_H__
#define __TOKENUTIL_H__
#include"sysoper.h"


class TokenUtil {
public:
    TokenUtil();

    static Void setToken(Token* token, const Char* start, Int32 size = -1);
    static Void getToken(const Token* token, Char* buf, Int32 max);
    static Int32 cmpToken(const Token* token1, const Token* token2);
    static Int32 caseCmpToken(const Token* token1, const Token* token2);
    static Int32 cmpStr(const Token* token, const Char* needle);
    static Int32 caseCmpStr(const Token* token, const Char* needle);
    static Void stripToken(Token* token);
    static Int32 toNum(const Token* token); 
    static Bool isFall(const Char* delim, Int32 delimLen, Char ch);
    static Bool isSpace(Char ch);
    static Bool isBlank(Char ch);

    Void reset(const Token* token);
    Void reset(const Char* data, Int32 size);
    Void reset(const Char* data);
    Void stripTail();
    Void stripHead();
    Void stripAll();

    Bool findFirstNotOf(const Char* reject, Token* entity) const;
    Bool findFirstOf(const Char* accept, Token* entity) const; 
    Bool findLastNotOf(const Char* accept, Token* entity) const;
    Bool findLastOf(const Char* accept, Token* entity) const;
    
    Bool findChar(Char ch, Token* entity) const;
    Bool findStr(const Char* str, Token* entity) const;
    Bool startWith(const Char* str, Token* entity) const;
    Bool startCaseWith(const Char* str, Token* entity) const;
    Bool startEqual(Char ch) const;
    Void cutBefore(const Token* marker, Token* entity) const;
    Void cutAfter(const Token* marker, Token* entity) const;
    Void cutFrom(const Token* marker, Token* entity) const;
    Void copy(Token* entity) const;
    Bool isEol() const;
    
    Void addHead(Int32 n);
    Void subTail(Int32 n);
    Void skipHead(const Char* sep);
    Void skipTail(const Char* sep);
    
    Void setEol(); 
    Void seek(const Token* marker);
    Void seekAfter(const Token* marker); 

    Void trancate(const Token* marker);
    Void trancateAfter(const Token* marker); 
        
    Void skipBlank();
    
    Bool nextWord(Token* next); 
    Bool nextToken(const Char* delim, Token* next);
    Bool nextToken(Char sep, Token* next);
    
private:
    const Char* m_start;
    const Char* m_end;
};

#endif

