/*===--------------------------------------------------------------------------------------------===
 * parser.h
 *
 * Basic recursive-descent parser so I don't have to rewrite it every time I create a new project.
 *
 * Created by Amy Parent <amy@amyparent.com>
 * Copyright (c) 2022 Amy Parent
 *
 * Licensed under the MIT License
 *===--------------------------------------------------------------------------------------------===
*/
#ifndef _PARSER_H_
#define _PARSER_H_

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif
    
#ifndef PARSE_ASSERT
#include <assert.h>
#define PARSE_ASSERT(x) assert(x)
#endif
    
#ifndef PARSE_CALLOC
#include <stdlib.h>
#define PARSE_CALLOC(n, size) calloc(n, size)
#endif
    
#ifndef PARSE_FREE
#include <stdlib.h>
#define PARSE_FREE(x) free(x)
#endif

typedef enum {
    TOK_INVALID,
    TOK_TEXT,
    TOK_INT,
    TOK_FLOAT,
    TOK_EOF
} tok_kind_t;

typedef struct {
    tok_kind_t  kind;
    const char  *start;
    size_t      len;

    int         line, column;

    union {
        double  f64;
        int64_t i64;
    };
} tok_t;

typedef struct {
    bool        owns_src;
    const char  *src;
    const char  *end;
    const char  *ptr;
    
    int         line, column;
    tok_t       tok;

    char        *error;
} parser_t;

// Bookkeeping
void parse_init(parser_t *parser, const char *src, size_t len);
void parse_init_file(parser_t *parser, FILE *f);
void parse_init_path(parser_t *parser, const char *path);
void parse_fini(parser_t *parser);
void parse_fail(parser_t *parser, const char *fmt, ...);

// Lexing
const tok_t *lex(parser_t *parser);

// Recursive Descent Primitives
bool have(parser_t *parser, tok_kind_t kind);
bool match(parser_t *parser, tok_kind_t kind);
void expect(parser_t *parser, tok_kind_t kind);

// Helpers
int64_t parse_int(parser_t *parser);
double parse_float(parser_t *parser);
size_t parse_text(parser_t *parser, char *out, size_t cap);

#ifdef __cplusplus
}
#endif


#endif /* ifndef _PARSER_H_ */


