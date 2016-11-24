
#include "parse.h"

void parse_init(Parser* p)
{
    memset(p, 0, sizeof(Parser));
    
    str_init(&p->accum);
    array_init(&p->content, ManifestEntry);
}

static void parse_deinit_each_value(void* ptr)
{
    String* str = (String*)ptr;
    
    str_deinit(str);
}

void parse_deinit_each_patch_entry(void* ptr)
{
    ManifestEntry* me = (ManifestEntry*)ptr;
    
    sstr_destroy(me->name);
    tbl_deinit(&me->content, parse_deinit_each_value);
}

void parse_deinit(Parser* p)
{
    str_deinit(&p->accum);
    array_deinit(&p->content, parse_deinit_each_patch_entry);
}

static int parse_skip_until(Lexer* lex, int c)
{
    for (;;)
    {
        int tk = lex_adv(lex);
        
        if (tk == Token_End)
            break;
        
        if (tk == c)
            return true;
    }
    
    return false;
}

static int parse_tag(Parser* p, Lexer* lex)
{
    const char* str = NULL;
    uint32_t len    = 0;
    ManifestEntry me;
    ManifestEntry* mePtr;
    
    for (;;)
    {
        int tk = lex_adv(lex);
        
        switch (tk)
        {
        case Token_End:
            return ERR_Done;
        
        case Token_String:
            if (!str)
                str = lex_str(lex);
            
            len += lex_len(lex);
            break;
            
        case ']':
            goto finish;
        
        default:
            len++;
            break;
        }
    }
    
finish:
    if (!str) return ERR_Done;
    
    me.name = sstr_create(str, len);
    
    if (!me.name) return ERR_OutOfMemory;
    
    tbl_init(&me.content, String);
    
    mePtr = array_push_back_type(&p->content, &me, ManifestEntry);
    
    if (!mePtr) return ERR_OutOfMemory;
    
    p->curTbl   = &mePtr->content;
    p->state    = Parse_Label;
    
    return ERR_None;
}

static int parse_label(Parser* p, Lexer* lex)
{
    for (;;)
    {
        int tk = lex_adv(lex);
        
        switch (tk)
        {
        case Token_End:
            return ERR_Done;
        
        case Token_String:
            p->key      = lex_str(lex);
            p->len      = lex_len(lex);
            p->state    = Parse_Equals;
            return ERR_None;
            
        case '[':
            return parse_tag(p, lex);
        
        default:
            break;
        }
    }
    
    return ERR_None;
}

static int parse_equals(Parser* p, Lexer* lex)
{
    for (;;)
    {
        int tk = lex_adv(lex);
        
        switch (tk)
        {
        case Token_End:
            return ERR_Done;
        
        case '=':
            p->state = Parse_Value;
            return ERR_None;
        
        default:
            if (isspace(tk))
                break;
            
            return ERR_Done;
        }
    }
    
    return ERR_None;
}

static int parse_value(Parser* p, Lexer* lex)
{
    int rc;
    int skipNewline = false;
    int readString  = false;
    
    str_clear(&p->accum);
    
    for (;;)
    {
        int tk = lex_adv(lex);
        
        switch (tk)
        {
        case Token_End:
            return ERR_Done;
        
        case Token_String:
            skipNewline = false;
            readString  = true;
            rc          = str_append(&p->accum, lex_str(lex), lex_len(lex));
        
            if (rc) return rc;
        
            break;
        
        case '\n':
            if (skipNewline)
            {
                skipNewline = false;
                break;
            }
            
            goto finish;
        
        case '\\':
            skipNewline = true;
            break;
        
        default:
            if (isspace(tk) && !readString)
                break;
            
            rc = str_append_char(&p->accum, tk);
            
            if (rc) return rc;
            
            break;
        }
    }
    
finish:
    /* Insert the accumulated string under the awaiting key */
    
    if (!p->key || !p->curTbl) return ERR_Invalid;
    
    rc = tbl_set_str(p->curTbl, p->key, p->len, &p->accum);
    
    if (rc) return rc;
    
    str_init(&p->accum);
    
    p->state = Parse_Label;
    
    return ERR_None;
}

int parse_file(Parser* p, const char* str, uint32_t len)
{
    Lexer* lex  = &p->lex;
    int rc      = ERR_None;
    
    lex_init(&p->lex, str, len);
    
    for (;;)
    {
        switch (p->state)
        {
        case Parse_Start:
            if (!parse_skip_until(lex, '['))
                goto finish;
            
            if ((rc = parse_tag(p, lex)))
                goto finish;
            
            break;
            
        case Parse_Label:
            if ((rc = parse_label(p, lex)))
                goto finish;
            
            break;
            
        case Parse_Equals:
            if ((rc = parse_equals(p, lex)))
                goto finish;
            
            break;
            
        case Parse_Value:
            if ((rc = parse_value(p, lex)))
                goto finish;
            
            break;
        }
    }
    
finish:
    return (rc == ERR_Done) ? ERR_None : rc;
}

Array* parse_get_manifests(Parser* p)
{
    return &p->content;
}
