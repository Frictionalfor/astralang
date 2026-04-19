#ifndef ASTRA_VERSION_H
#define ASTRA_VERSION_H

/* -----------------------------------------------------------------------
 * AstraLang version constants — single source of truth for all components
 *
 * Versioning rules:
 *   MAJOR — breaking changes to bytecode format or language semantics
 *   MINOR — backward-compatible additions (new opcodes, new stdlib modules)
 *   PATCH — bug fixes, no behavioral changes
 *
 * Bytecode compatibility:
 *   A runtime with major version N can execute bytecode with major version N.
 *   Minor version mismatches are allowed (runtime >= file minor).
 *   Patch version is never encoded in bytecode.
 *
 * Deprecation policy:
 *   An opcode is marked DEPRECATED for one full minor version cycle.
 *   It is removed only on the next MAJOR version bump.
 *   Reserved opcode slots must never be reused for different semantics.
 * ----------------------------------------------------------------------- */

/* Compiler toolchain version */
#define ASTRA_COMPILER_MAJOR   2
#define ASTRA_COMPILER_MINOR   0
#define ASTRA_COMPILER_PATCH   0
#define ASTRA_COMPILER_VERSION "2.0.0"

/* Bytecode format version */
#define ASTRA_BYTECODE_MAJOR   1
#define ASTRA_BYTECODE_MINOR   2
#define ASTRA_BYTECODE_VERSION ((ASTRA_BYTECODE_MAJOR << 16) | ASTRA_BYTECODE_MINOR)

/* Standard library version */
#define ASTRA_STDLIB_MAJOR     1
#define ASTRA_STDLIB_MINOR     0
#define ASTRA_STDLIB_VERSION   "1.0.0"

/* Runtime version (kept in sync with Cargo.toml) */
#define ASTRA_RUNTIME_VERSION  "2.0.0"

/* Minimum supported bytecode major version */
#define ASTRA_MIN_BYTECODE_MAJOR 1

/* Compatibility check: returns 1 if file_ver is loadable by this runtime */
static inline int astra_bytecode_compatible(unsigned int file_ver) {
    unsigned int file_major = file_ver >> 16;
    return (file_major >= ASTRA_MIN_BYTECODE_MAJOR &&
            file_major <= ASTRA_BYTECODE_MAJOR);
}

/* Opcode lifecycle states — stored in opcode table metadata */
typedef enum {
    ASTRA_OP_STABLE     = 0,  /* fully supported, no planned changes */
    ASTRA_OP_ADDED      = 1,  /* added in current minor version */
    ASTRA_OP_DEPRECATED = 2,  /* will be removed in next major version */
    ASTRA_OP_RESERVED   = 3,  /* slot reserved, must not be emitted */
} AstraOpLifecycle;

#endif /* ASTRA_VERSION_H */
