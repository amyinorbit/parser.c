/*===--------------------------------------------------------------------------------------------===
 * parser.c
 *
 * Created by Amy Parent <amy@amyparent.com>
 * Copyright (c) 2022 Amy Parent. All rights reserved
 *
 * Licensed under the MIT License
 *===--------------------------------------------------------------------------------------------===
*/
#include "parser.h"
#include <ctype.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <math.h>

// We use a simple recursive descent lexer/parser


void parse_init(parser_t *parser, const char *src, size_t len) {
    PARSE_ASSERT(parser != NULL);
    PARSE_ASSERT(src != NULL);
    PARSE_ASSERT(len > 0);
    
    memset(parser, 0, sizeof(*parser));
    parser->owns_src = false;
    parser->src = src;
    parser->end = src + len;
    parser->ptr = src;
    
    parser->line = 0;
    parser->column = 1;
    
    parser->error = NULL;
    parser->tok.kind = TOK_INVALID;
    lex(parser);
}

void parse_init_file(parser_t *parser, FILE *f) {
    PARSE_ASSERT(parser != NULL);
    PARSE_ASSERT(f != NULL);
    memset(parser, 0, sizeof(*parser));
    
    fseek(f, 0, SEEK_END);
    size_t size = ftell(f);
    fseek(f, 0, SEEK_SET);
    
    char *src = PARSE_CALLOC(size+1, sizeof(char));
    PARSE_ASSERT(src);
    fread(src, sizeof(char), size, f);
    src[size] = '\0';
    
    parse_init(parser, src, size);
    parser->owns_src = true;
}

void parse_init_path(parser_t *parser, const char *path) {
    PARSE_ASSERT(parser != NULL);
    PARSE_ASSERT(path != NULL);
    memset(parser, 0, sizeof(*parser));
    
    FILE *f = fopen(path, "rb");
    if(!f) {
        parse_fail(parser, "can't open '%s' (%s)", path, strerror(errno));
        return;
    }
    parse_init_file(parser, f);
    fclose(f);
}

void parse_fini(parser_t *parser) {
    if(parser->src && parser->owns_src) free((char *)parser->src);
    if(parser->error) free(parser->error);
    memset(parser, 0, sizeof(*parser));
}

static char *vsprintf_alloc(const char *fmt, va_list args) {
    va_list args2;
    va_copy(args2, args);
    size_t len = vsnprintf(NULL, 0, fmt, args2);
    va_end(args2);
    
    PARSE_ASSERT(len > 0);
    char *out = PARSE_CALLOC(len+1, 1);
    (void)vsnprintf(out, len+1, fmt, args);
    return out;
}

void parse_fail(parser_t *parser, const char *fmt, ...) {
    if(parser->error) return;
    
    va_list args;
    va_start(args, fmt);
    parser->error = vsprintf_alloc(fmt, args);
    va_end(args);
}

static int advance(parser_t *parser) {
    if(parser->error) return 0;
    if(parser->ptr == parser->end) return EOF;
    int current = *parser->ptr++;
    
    if(*parser->ptr == '\n') {
        parser->line += 1;
        parser->column = 0;
    } else {
        parser->column += 1;
    }
    return current;
}

static int peek(parser_t *parser) {
    if(parser->error) return 0;
    if(parser->ptr == parser->end) return EOF;
    return *parser->ptr;
}

static void skip_whitespace(parser_t *parser) {
    for(;;) {
        switch(peek(parser)) {    
        case '\t':
        case '\r':
        case ' ':
        case '\n':
            advance(parser);
            break;
        case '#':
            while(peek(parser) != '\n' && peek(parser) != EOF) {
                advance(parser);
            }
            break;
        default: return;
        }
    }
}

static inline bool is_tok_char(char c) {
    return isalnum(c) || c == '.' || c == '+' || c == '-';
}

static const tok_t *make_token(parser_t *parser) {
    size_t len = 1;
    
    enum {
        STATE_INT,
        STATE_FLOAT,
        STATE_EXP_SIGN,
        STATE_EXP,
        STATE_TEXT,
    };
    
    int state = STATE_INT; // We start with the most restrictive possible kind.
    advance(parser);
    for(;;) {
        int c = advance(parser);
        if(!is_tok_char(c)) break;
        len += 1;
        switch(state) {
        case STATE_INT:
            if(c == '.') state = STATE_FLOAT;
            else if(!isdigit(c)) state = STATE_TEXT;
            break;
            
        case STATE_FLOAT:
            if(c == 'E' || c == 'e') state = STATE_EXP_SIGN;
            else if(!isdigit(c)) state = STATE_TEXT;
            break;
            
        case STATE_EXP_SIGN:
            if(c == '+' || c == '-') state = STATE_EXP;
            else state = STATE_TEXT;
            break;
            
        case STATE_EXP:
            if(!isdigit(c)) state = STATE_TEXT;
            break;
            
        case STATE_TEXT:
            break;
            
            default: break;
        }
    }
    
    switch(state) {
    case STATE_INT: parser->tok.kind = TOK_INT; break;
    case STATE_FLOAT:
    case STATE_EXP: parser->tok.kind = TOK_FLOAT; break;
    default: parser->tok.kind = TOK_TEXT; break;
    }
    parser->tok.len = len;
    
    if(parser->tok.kind == TOK_INT) {
        parser->tok.i64 = atol(parser->tok.start);
    } else if(parser->tok.kind == TOK_FLOAT) {
        parser->tok.f64 = atof(parser->tok.start);
    }
    
    return &parser->tok;
}

const tok_t *lex(parser_t *parser) {
    if(parser->error) return &parser->tok;
    
    skip_whitespace(parser);
    parser->tok.start = parser->ptr;
    parser->tok.line = parser->line;
    parser->tok.column = parser->column;
    
    int c = peek(parser);
    
    if(c == EOF) {
        parser->tok.kind = TOK_EOF;
        return &parser->tok;
    }
    
    if(is_tok_char(c)) {
        return make_token(parser);
    }
    
    parser->tok.kind = TOK_INVALID;
    return &parser->tok;
}

bool have(parser_t *parser, tok_kind_t kind) {
    PARSE_ASSERT(parser != NULL);
    return parser->tok.kind == kind;
}

bool match(parser_t *parser, tok_kind_t kind) {
    PARSE_ASSERT(parser != NULL);
    if(!have(parser, kind)) return false;
    (void)lex(parser);
    return true;
}

static const char *tok_name(tok_kind_t kind) {
    switch(kind) {
        case TOK_INVALID: return "an invalid token";
        case TOK_EOF: return "the end of file";
        case TOK_INT: return "an integer";
        case TOK_FLOAT: return "a number";
        case TOK_TEXT: return "a word";
    }
    return "<bad token kind>";
}

void syntax_error(parser_t *parser, tok_kind_t kind) {
    PARSE_ASSERT(parser != NULL);
    if(parser->error) return;
    
    parse_fail(parser, "found %s, but needed %s",
        tok_name(parser->tok.kind), tok_name(kind));
}

void expect(parser_t *parser, tok_kind_t kind) {
    PARSE_ASSERT(parser != NULL);
    if(parser->error) return;
    if(!have(parser, kind)) {
        syntax_error(parser, kind);
        return;
    }
    lex(parser);
}

int64_t parse_int(parser_t *parser) {
    PARSE_ASSERT(parser != NULL);
    if(parser->error) return 0;
    if(!have(parser, TOK_INT)) {
        syntax_error(parser, TOK_INT);
    }
    int64_t val = parser->tok.i64;
    lex(parser);
    return val;
}

double parse_float(parser_t *parser) {
    PARSE_ASSERT(parser != NULL);
    if(parser->error) return 0;
    if(!have(parser, TOK_INT) && !have(parser, TOK_FLOAT)) {
        syntax_error(parser, TOK_FLOAT);
        return NAN;
    }
    double val = parser->tok.kind == TOK_FLOAT ? parser->tok.f64 : parser->tok.i64;
    lex(parser);
    return val;
}

size_t parse_text(parser_t *parser, char *out, size_t cap) {
    PARSE_ASSERT(parser != NULL);
    if(parser->error) return 0;
    
    if(!have(parser, TOK_TEXT)) {
        syntax_error(parser, TOK_TEXT);
        return 0;
    }
    
    size_t len = parser->tok.len;
    const char *src = parser->tok.start;
    lex(parser);
    
    if(!out) return len;
    if(cap < len) {
        len = cap;
    }
    strncpy(out, src, cap);
    return len;
}
