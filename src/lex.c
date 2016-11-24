
#include "lex.h"

static void lex_get(Lexer* lex, LexCursor* lc);

void lex_init(Lexer* lex, SimpleString* str)
{
    lex->pos    = 0;
    lex->len    = sstr_length(str);
    lex->src    = sstr_data(str);
    
    lex->cur.token  = Token_Start;
    lex->cur.ptr    = NULL;
    lex->cur.len    = 0;
    
    lex->next.token = Token_Start;
    lex->next.ptr   = NULL;
    lex->next.len   = 0;
    
    lex->str = str;
    
    lex_get(lex, &lex->next);
}

void lex_deinit(Lexer* lex)
{
    if (lex->str)
    {
        sstr_destroy(lex->str);
        lex->str = NULL;
    }
}

static void lex_read(Lexer* lex, LexCursor* lc)
{
    uint32_t pos    = lex->pos;
    uint32_t last   = lex->len;
    const char* src = lex->src;
    uint32_t len    = 1;
    
    lc->token   = Token_String;
    lc->ptr     = src + pos - 1;
    
    while (pos < last)
    {
        int c = src[pos];
        
        if (!isalnum(c))
            break;
        
        pos++;
        len++;
    }
    
    lex->pos    = pos;
    lc->len     = len;
}

static void lex_get(Lexer* lex, LexCursor* lc)
{
    if (lex->pos < lex->len)
    {
        int c = lex->src[lex->pos++];
        
        if (isalnum(c))
            lex_read(lex, lc);
        else
            lc->token = c;
    }
    else
    {
        lc->token = Token_End;
    }
}

int lex_adv(Lexer* lex)
{
    memcpy(&lex->cur, &lex->next, sizeof(LexCursor));
    lex_get(lex, &lex->next);
    return lex->cur.token;
}

int lex_peek(Lexer* lex)
{
    return lex->next.token;
}

const char* lex_str(Lexer* lex)
{
    return lex->cur.ptr;
}

uint32_t lex_len(Lexer* lex)
{
    return lex->cur.len;
}
