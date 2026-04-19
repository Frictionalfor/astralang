#include "../include/astra_token.h"
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdio.h>

struct Lexer {
    const char *src;
    int         len;
    int         pos;
    int         line;
    int         col;
    Token       peeked;
    int         has_peeked;
};

/* ---- keyword table ---- */
typedef struct { const char *word; TokenType type; } KW;
static const KW KEYWORDS[] = {
    {"let",    TOK_LET},    {"mut",    TOK_MUT},
    {"fn",     TOK_FN},     {"return", TOK_RETURN},
    {"if",     TOK_IF},     {"else",   TOK_ELSE},
    {"while",  TOK_WHILE},  {"for",    TOK_FOR},
    {"in",     TOK_IN},     {"struct", TOK_STRUCT},
    {"mod",    TOK_MOD},    {"use",    TOK_USE},
    {"pub",    TOK_PUB},    {"extern", TOK_EXTERN},
    {"true",   TOK_TRUE},   {"false",  TOK_FALSE},
    {"null",   TOK_NULL},   {"as",     TOK_AS},
    {"async",  TOK_ASYNC},  {"await",  TOK_AWAIT},
    {"i8",     TOK_TYPE_I8},  {"i16",  TOK_TYPE_I16},
    {"i32",    TOK_TYPE_I32}, {"i64",  TOK_TYPE_I64},
    {"u8",     TOK_TYPE_U8},  {"u16",  TOK_TYPE_U16},
    {"u32",    TOK_TYPE_U32}, {"u64",  TOK_TYPE_U64},
    {"f32",    TOK_TYPE_F32}, {"f64",  TOK_TYPE_F64},
    {"bool",   TOK_TYPE_BOOL},{"str",  TOK_TYPE_STR},
    {"void",   TOK_TYPE_VOID},{"ptr",  TOK_TYPE_PTR},
    {NULL, 0}
};

Lexer *lexer_create(const char *source, int length) {
    Lexer *l = (Lexer *)malloc(sizeof(Lexer));
    l->src        = source;
    l->len        = length;
    l->pos        = 0;
    l->line       = 1;
    l->col        = 1;
    l->has_peeked = 0;
    return l;
}

void lexer_destroy(Lexer *l) { free(l); }

static char cur(Lexer *l)  { return l->pos < l->len ? l->src[l->pos] : '\0'; }
static char peek1(Lexer *l){ return (l->pos+1) < l->len ? l->src[l->pos+1] : '\0'; }

static void advance(Lexer *l) {
    if (l->pos < l->len) {
        if (l->src[l->pos] == '\n') { l->line++; l->col = 1; }
        else                         { l->col++; }
        l->pos++;
    }
}

static void skip_whitespace_and_comments(Lexer *l) {
    while (l->pos < l->len) {
        char c = cur(l);
        if (c == ' ' || c == '\t' || c == '\r' || c == '\n') {
            advance(l);
        } else if (c == '/' && peek1(l) == '/') {
            while (l->pos < l->len && cur(l) != '\n') advance(l);
        } else if (c == '/' && peek1(l) == '*') {
            advance(l); advance(l);
            while (l->pos < l->len) {
                if (cur(l) == '*' && peek1(l) == '/') { advance(l); advance(l); break; }
                advance(l);
            }
        } else {
            break;
        }
    }
}

static Token make_tok(Lexer *l, TokenType t, int start, int line, int col) {
    Token tok;
    tok.type   = t;
    tok.start  = l->src + start;
    tok.length = l->pos - start;
    tok.line   = line;
    tok.col    = col;
    return tok;
}

static Token lex_ident_or_kw(Lexer *l) {
    int start = l->pos, line = l->line, col = l->col;
    while (l->pos < l->len && (isalnum(cur(l)) || cur(l) == '_')) advance(l);
    int len = l->pos - start;
    for (int i = 0; KEYWORDS[i].word; i++) {
        if ((int)strlen(KEYWORDS[i].word) == len &&
            strncmp(l->src + start, KEYWORDS[i].word, len) == 0) {
            return make_tok(l, KEYWORDS[i].type, start, line, col);
        }
    }
    return make_tok(l, TOK_IDENT, start, line, col);
}

static Token lex_number(Lexer *l) {
    int start = l->pos, line = l->line, col = l->col;
    int is_float = 0;
    while (l->pos < l->len && isdigit(cur(l))) advance(l);
    if (cur(l) == '.' && isdigit(peek1(l))) {
        is_float = 1;
        advance(l);
        while (l->pos < l->len && isdigit(cur(l))) advance(l);
    }
    if (cur(l) == 'e' || cur(l) == 'E') {
        is_float = 1;
        advance(l);
        if (cur(l) == '+' || cur(l) == '-') advance(l);
        while (l->pos < l->len && isdigit(cur(l))) advance(l);
    }
    return make_tok(l, is_float ? TOK_FLOAT_LIT : TOK_INT_LIT, start, line, col);
}

static Token lex_string(Lexer *l) {
    int start = l->pos, line = l->line, col = l->col;
    advance(l); // consume opening "
    while (l->pos < l->len && cur(l) != '"') {
        if (cur(l) == '\\') advance(l); // skip escape
        advance(l);
    }
    if (cur(l) == '"') advance(l); // consume closing "
    return make_tok(l, TOK_STRING_LIT, start, line, col);
}

static Token lex_one(Lexer *l) {
    skip_whitespace_and_comments(l);
    if (l->pos >= l->len) {
        Token t; t.type = TOK_EOF; t.start = ""; t.length = 0;
        t.line = l->line; t.col = l->col;
        return t;
    }

    char c = cur(l);
    int  start = l->pos, line = l->line, col = l->col;

    if (isalpha(c) || c == '_') return lex_ident_or_kw(l);
    if (isdigit(c))              return lex_number(l);
    if (c == '"')                return lex_string(l);

    advance(l);
    switch (c) {
        case '+': if (cur(l)=='='){advance(l);return make_tok(l,TOK_PLUS_EQ,start,line,col);}
                  return make_tok(l, TOK_PLUS,  start, line, col);
        case '-': if (cur(l)=='>'){advance(l);return make_tok(l,TOK_ARROW,start,line,col);}
                  if (cur(l)=='='){advance(l);return make_tok(l,TOK_MINUS_EQ,start,line,col);}
                  return make_tok(l, TOK_MINUS, start, line, col);
        case '*': if (cur(l)=='='){advance(l);return make_tok(l,TOK_STAR_EQ,start,line,col);}
                  return make_tok(l, TOK_STAR,  start, line, col);
        case '/': if (cur(l)=='='){advance(l);return make_tok(l,TOK_SLASH_EQ,start,line,col);}
                  return make_tok(l, TOK_SLASH, start, line, col);
        case '%': return make_tok(l, TOK_PERCENT,   start, line, col);
        case '&': if (cur(l)=='&'){advance(l);return make_tok(l,TOK_AND,start,line,col);}
                  return make_tok(l, TOK_AMP,        start, line, col);
        case '|': if (cur(l)=='|'){advance(l);return make_tok(l,TOK_OR,start,line,col);}
                  return make_tok(l, TOK_PIPE,        start, line, col);
        case '^': return make_tok(l, TOK_CARET,      start, line, col);
        case '~': return make_tok(l, TOK_TILDE,      start, line, col);
        case '!': if (cur(l)=='='){advance(l);return make_tok(l,TOK_NEQ,start,line,col);}
                  return make_tok(l, TOK_BANG,        start, line, col);
        case '=': if (cur(l)=='='){advance(l);return make_tok(l,TOK_EQEQ,start,line,col);}
                  if (cur(l)=='>'){advance(l);return make_tok(l,TOK_FAT_ARROW,start,line,col);}
                  return make_tok(l, TOK_EQ,           start, line, col);
        case '<': if (cur(l)=='='){advance(l);return make_tok(l,TOK_LTE,start,line,col);}
                  if (cur(l)=='<'){advance(l);return make_tok(l,TOK_LSHIFT,start,line,col);}
                  return make_tok(l, TOK_LT,            start, line, col);
        case '>': if (cur(l)=='='){advance(l);return make_tok(l,TOK_GTE,start,line,col);}
                  if (cur(l)=='>'){advance(l);return make_tok(l,TOK_RSHIFT,start,line,col);}
                  return make_tok(l, TOK_GT,            start, line, col);
        case '.': if (cur(l)=='.'){advance(l);return make_tok(l,TOK_DOTDOT,start,line,col);}
                  return make_tok(l, TOK_DOT,           start, line, col);
        case ':': if (cur(l)==':'){advance(l);return make_tok(l,TOK_COLONCOLON,start,line,col);}
                  return make_tok(l, TOK_COLON,         start, line, col);
        case ';': return make_tok(l, TOK_SEMICOLON,     start, line, col);
        case ',': return make_tok(l, TOK_COMMA,         start, line, col);
        case '(': return make_tok(l, TOK_LPAREN,        start, line, col);
        case ')': return make_tok(l, TOK_RPAREN,        start, line, col);
        case '{': return make_tok(l, TOK_LBRACE,        start, line, col);
        case '}': return make_tok(l, TOK_RBRACE,        start, line, col);
        case '[': return make_tok(l, TOK_LBRACKET,      start, line, col);
        case ']': return make_tok(l, TOK_RBRACKET,      start, line, col);
        default:
            fprintf(stderr, "[lexer] unexpected character '%c' at %d:%d\n", c, line, col);
            return make_tok(l, TOK_ERROR, start, line, col);
    }
}

Token lexer_next(Lexer *l) {
    if (l->has_peeked) { l->has_peeked = 0; return l->peeked; }
    return lex_one(l);
}

Token lexer_peek(Lexer *l) {
    if (!l->has_peeked) { l->peeked = lex_one(l); l->has_peeked = 1; }
    return l->peeked;
}

const char *token_type_name(TokenType t) {
    switch (t) {
        case TOK_INT_LIT:    return "INT_LIT";
        case TOK_FLOAT_LIT:  return "FLOAT_LIT";
        case TOK_STRING_LIT: return "STRING_LIT";
        case TOK_BOOL_LIT:   return "BOOL_LIT";
        case TOK_IDENT:      return "IDENT";
        case TOK_LET:        return "let";
        case TOK_MUT:        return "mut";
        case TOK_FN:         return "fn";
        case TOK_RETURN:     return "return";
        case TOK_IF:         return "if";
        case TOK_ELSE:       return "else";
        case TOK_WHILE:      return "while";
        case TOK_FOR:        return "for";
        case TOK_STRUCT:     return "struct";
        case TOK_EOF:        return "EOF";
        case TOK_ERROR:      return "ERROR";
        default:             return "TOKEN";
    }
}
