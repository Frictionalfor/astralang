#ifndef ASTRA_ERROR_H
#define ASTRA_ERROR_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

/* -----------------------------------------------------------------------
 * AstraLang unified error system
 * Used by lexer, compiler, and runtime to emit structured diagnostics.
 * ----------------------------------------------------------------------- */

typedef enum {
    ASTRA_ERR_NONE = 0,

    /* Lexer errors (1xx) */
    ASTRA_ERR_UNEXPECTED_CHAR  = 100,
    ASTRA_ERR_UNTERMINATED_STR = 101,
    ASTRA_ERR_INVALID_NUMBER   = 102,

    /* Parser errors (2xx) */
    ASTRA_ERR_UNEXPECTED_TOKEN = 200,
    ASTRA_ERR_MISSING_SEMICOLON= 201,
    ASTRA_ERR_MISSING_BRACE    = 202,
    ASTRA_ERR_MISSING_PAREN    = 203,
    ASTRA_ERR_INVALID_EXPR     = 204,

    /* Semantic errors (3xx) */
    ASTRA_ERR_UNDEFINED_SYMBOL = 300,
    ASTRA_ERR_TYPE_MISMATCH    = 301,
    ASTRA_ERR_REDEFINED_SYMBOL = 302,
    ASTRA_ERR_IMMUTABLE_ASSIGN = 303,
    ASTRA_ERR_WRONG_ARG_COUNT  = 304,
    ASTRA_ERR_RETURN_TYPE      = 305,

    /* Codegen errors (4xx) */
    ASTRA_ERR_CODEGEN          = 400,

    /* Runtime errors (5xx) */
    ASTRA_ERR_STACK_OVERFLOW   = 500,
    ASTRA_ERR_STACK_UNDERFLOW  = 501,
    ASTRA_ERR_DIV_BY_ZERO      = 502,
    ASTRA_ERR_NULL_DEREF       = 503,
    ASTRA_ERR_OUT_OF_BOUNDS    = 504,
    ASTRA_ERR_INVALID_OPCODE   = 505,
    ASTRA_ERR_CALL_INVALID_FN  = 506,
    ASTRA_ERR_TYPE_ERROR       = 507,

    /* Loader errors (6xx) */
    ASTRA_ERR_BAD_MAGIC        = 600,
    ASTRA_ERR_VERSION_MISMATCH = 601,
    ASTRA_ERR_CORRUPT_MODULE   = 602,
    ASTRA_ERR_FILE_NOT_FOUND   = 603,
} AstraErrorCode;

typedef enum {
    ASTRA_SEVERITY_ERROR   = 0,
    ASTRA_SEVERITY_WARNING = 1,
    ASTRA_SEVERITY_NOTE    = 2,
} AstraSeverity;

typedef struct {
    AstraErrorCode code;
    AstraSeverity  severity;
    int            line;
    int            col;
    const char    *file;     /* source file path, may be NULL */
    const char    *message;  /* human-readable message */
} AstraDiagnostic;

/* Print a diagnostic to stderr in structured format:
 *   [ERROR 300] file.as:12:5 — undefined symbol 'foo'
 */
void astra_print_diagnostic(const AstraDiagnostic *d);

/* Return a static string name for an error code */
const char *astra_error_name(AstraErrorCode code);

#ifdef __cplusplus
}
#endif

#endif /* ASTRA_ERROR_H */
