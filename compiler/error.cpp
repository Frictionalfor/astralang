#include "../include/astra_error.h"
#include <stdio.h>
#include <string.h>

static const char *severity_label(AstraSeverity s) {
    switch (s) {
        case ASTRA_SEVERITY_ERROR:   return "\033[1;31mERROR\033[0m";
        case ASTRA_SEVERITY_WARNING: return "\033[1;33mWARN\033[0m";
        case ASTRA_SEVERITY_NOTE:    return "\033[1;36mNOTE\033[0m";
        default:                     return "INFO";
    }
}

void astra_print_diagnostic(const AstraDiagnostic *d) {
    const char *file = d->file ? d->file : "<unknown>";
    fprintf(stderr, "%s [%d] %s:%d:%d — %s\n",
            severity_label(d->severity),
            (int)d->code,
            file,
            d->line,
            d->col,
            d->message ? d->message : "");
}

const char *astra_error_name(AstraErrorCode code) {
    switch (code) {
        case ASTRA_ERR_NONE:              return "NONE";
        case ASTRA_ERR_UNEXPECTED_CHAR:   return "UNEXPECTED_CHAR";
        case ASTRA_ERR_UNTERMINATED_STR:  return "UNTERMINATED_STR";
        case ASTRA_ERR_INVALID_NUMBER:    return "INVALID_NUMBER";
        case ASTRA_ERR_UNEXPECTED_TOKEN:  return "UNEXPECTED_TOKEN";
        case ASTRA_ERR_MISSING_SEMICOLON: return "MISSING_SEMICOLON";
        case ASTRA_ERR_MISSING_BRACE:     return "MISSING_BRACE";
        case ASTRA_ERR_MISSING_PAREN:     return "MISSING_PAREN";
        case ASTRA_ERR_INVALID_EXPR:      return "INVALID_EXPR";
        case ASTRA_ERR_UNDEFINED_SYMBOL:  return "UNDEFINED_SYMBOL";
        case ASTRA_ERR_TYPE_MISMATCH:     return "TYPE_MISMATCH";
        case ASTRA_ERR_REDEFINED_SYMBOL:  return "REDEFINED_SYMBOL";
        case ASTRA_ERR_IMMUTABLE_ASSIGN:  return "IMMUTABLE_ASSIGN";
        case ASTRA_ERR_WRONG_ARG_COUNT:   return "WRONG_ARG_COUNT";
        case ASTRA_ERR_RETURN_TYPE:       return "RETURN_TYPE";
        case ASTRA_ERR_CODEGEN:           return "CODEGEN";
        case ASTRA_ERR_STACK_OVERFLOW:    return "STACK_OVERFLOW";
        case ASTRA_ERR_STACK_UNDERFLOW:   return "STACK_UNDERFLOW";
        case ASTRA_ERR_DIV_BY_ZERO:       return "DIV_BY_ZERO";
        case ASTRA_ERR_NULL_DEREF:        return "NULL_DEREF";
        case ASTRA_ERR_OUT_OF_BOUNDS:     return "OUT_OF_BOUNDS";
        case ASTRA_ERR_INVALID_OPCODE:    return "INVALID_OPCODE";
        case ASTRA_ERR_CALL_INVALID_FN:   return "CALL_INVALID_FN";
        case ASTRA_ERR_TYPE_ERROR:        return "TYPE_ERROR";
        case ASTRA_ERR_BAD_MAGIC:         return "BAD_MAGIC";
        case ASTRA_ERR_VERSION_MISMATCH:  return "VERSION_MISMATCH";
        case ASTRA_ERR_CORRUPT_MODULE:    return "CORRUPT_MODULE";
        case ASTRA_ERR_FILE_NOT_FOUND:    return "FILE_NOT_FOUND";
        default:                          return "UNKNOWN";
    }
}
