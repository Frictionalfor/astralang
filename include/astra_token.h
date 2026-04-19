#ifndef ASTRA_TOKEN_H
#define ASTRA_TOKEN_H

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    // Literals
    TOK_INT_LIT,
    TOK_FLOAT_LIT,
    TOK_STRING_LIT,
    TOK_BOOL_LIT,

    // Identifiers & Keywords
    TOK_IDENT,
    TOK_LET,
    TOK_MUT,
    TOK_FN,
    TOK_RETURN,
    TOK_IF,
    TOK_ELSE,
    TOK_WHILE,
    TOK_FOR,
    TOK_IN,
    TOK_STRUCT,
    TOK_MOD,
    TOK_USE,
    TOK_PUB,
    TOK_EXTERN,
    TOK_TRUE,
    TOK_FALSE,
    TOK_NULL,
    TOK_AS,
    TOK_ASYNC,
    TOK_AWAIT,

    // Primitive types
    TOK_TYPE_I8,
    TOK_TYPE_I16,
    TOK_TYPE_I32,
    TOK_TYPE_I64,
    TOK_TYPE_U8,
    TOK_TYPE_U16,
    TOK_TYPE_U32,
    TOK_TYPE_U64,
    TOK_TYPE_F32,
    TOK_TYPE_F64,
    TOK_TYPE_BOOL,
    TOK_TYPE_STR,
    TOK_TYPE_VOID,
    TOK_TYPE_PTR,

    // Operators
    TOK_PLUS,
    TOK_MINUS,
    TOK_STAR,
    TOK_SLASH,
    TOK_PERCENT,
    TOK_AMP,
    TOK_PIPE,
    TOK_CARET,
    TOK_TILDE,
    TOK_BANG,
    TOK_EQ,
    TOK_EQEQ,
    TOK_NEQ,
    TOK_LT,
    TOK_GT,
    TOK_LTE,
    TOK_GTE,
    TOK_AND,
    TOK_OR,
    TOK_LSHIFT,
    TOK_RSHIFT,
    TOK_ARROW,      // ->
    TOK_FAT_ARROW,  // =>
    TOK_DOT,
    TOK_DOTDOT,
    TOK_COLON,
    TOK_COLONCOLON,
    TOK_SEMICOLON,
    TOK_COMMA,
    TOK_PLUS_EQ,
    TOK_MINUS_EQ,
    TOK_STAR_EQ,
    TOK_SLASH_EQ,

    // Delimiters
    TOK_LPAREN,
    TOK_RPAREN,
    TOK_LBRACE,
    TOK_RBRACE,
    TOK_LBRACKET,
    TOK_RBRACKET,

    // Special
    TOK_EOF,
    TOK_ERROR,
    TOK_NEWLINE,
} TokenType;

typedef struct {
    TokenType   type;
    const char *start;   // pointer into source buffer
    int         length;
    int         line;
    int         col;
} Token;

// Lexer opaque handle
typedef struct Lexer Lexer;

Lexer *lexer_create(const char *source, int length);
void   lexer_destroy(Lexer *lexer);
Token  lexer_next(Lexer *lexer);
Token  lexer_peek(Lexer *lexer);
const char *token_type_name(TokenType t);

#ifdef __cplusplus
}
#endif

#endif /* ASTRA_TOKEN_H */
