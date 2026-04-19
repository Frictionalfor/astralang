#ifndef ASTRA_IR_H
#define ASTRA_IR_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

/* -----------------------------------------------------------------------
 * AstraLang Bytecode IR — .asc format
 *
 * File layout:
 *   [4]  magic   "ASTR"
 *   [4]  version u32  (ASTRA_BYTECODE_VERSION)
 *   [4]  flags   u32  (ASTRA_FLAG_*)
 *   [4]  str_count
 *   ...  string pool entries: [u32 len][bytes]
 *   [4]  func_count
 *   ...  function entries (see IRFunction)
 *   [4]  entry_func_index
 *
 * Versioning policy:
 *   - MAJOR bump: breaking opcode changes, incompatible format
 *   - MINOR bump: new opcodes added (backward compatible)
 *   - Patch: metadata/debug info changes only
 *
 * Instruction layout (24 bytes, C struct with alignment):
 *   [4]  OpCode  op
 *   [4]  padding (reserved, must be 0)
 *   [8]  int64   operand_a
 *   [8]  int64   operand_b
 * ----------------------------------------------------------------------- */

#define ASTRA_BYTECODE_MAJOR   1
#define ASTRA_BYTECODE_MINOR   2
#define ASTRA_BYTECODE_VERSION ((ASTRA_BYTECODE_MAJOR << 16) | ASTRA_BYTECODE_MINOR)

/* File flags */
#define ASTRA_FLAG_NONE        0x00
#define ASTRA_FLAG_DEBUG       0x01   /* debug info present */
#define ASTRA_FLAG_OPTIMIZED   0x02   /* compiled with optimizer */
#define ASTRA_FLAG_STDLIB      0x04   /* stdlib module */

typedef enum {
    /* ---- Stack ops ---- */
    OP_PUSH_I64   = 0,
    OP_PUSH_F64   = 1,
    OP_PUSH_BOOL  = 2,
    OP_PUSH_NULL  = 3,
    OP_PUSH_STR   = 4,   /* operand_a: string pool index */
    OP_POP        = 5,
    OP_DUP        = 6,
    OP_SWAP       = 7,   /* swap top two stack values */

    /* ---- Locals / Globals ---- */
    OP_LOAD_LOCAL   = 10,  /* operand_a: slot */
    OP_STORE_LOCAL  = 11,
    OP_LOAD_GLOBAL  = 12,  /* operand_a: global index */
    OP_STORE_GLOBAL = 13,

    /* ---- Integer arithmetic ---- */
    OP_IADD = 20,
    OP_ISUB = 21,
    OP_IMUL = 22,
    OP_IDIV = 23,
    OP_IMOD = 24,
    OP_INEG = 25,

    /* ---- Float arithmetic ---- */
    OP_FADD = 30,
    OP_FSUB = 31,
    OP_FMUL = 32,
    OP_FDIV = 33,
    OP_FNEG = 34,

    /* ---- Integer comparison ---- */
    OP_IEQ  = 40,
    OP_INEQ = 41,
    OP_ILT  = 42,
    OP_IGT  = 43,
    OP_ILTE = 44,
    OP_IGTE = 45,

    /* ---- Float comparison ---- */
    OP_FEQ  = 50,
    OP_FNEQ = 51,
    OP_FLT  = 52,
    OP_FGT  = 53,
    OP_FLTE = 54,
    OP_FGTE = 55,

    /* ---- Logic ---- */
    OP_AND = 60,
    OP_OR  = 61,
    OP_NOT = 62,

    /* ---- Bitwise ---- */
    OP_BAND   = 70,
    OP_BOR    = 71,
    OP_BXOR   = 72,
    OP_BNOT   = 73,
    OP_LSHIFT = 74,
    OP_RSHIFT = 75,

    /* ---- Control flow ---- */
    OP_JMP       = 80,  /* operand_a: absolute instruction index */
    OP_JMP_IF    = 81,
    OP_JMP_IFNOT = 82,

    /* ---- Functions ---- */
    OP_CALL        = 90,  /* operand_a: func index, operand_b: arg count */
    OP_CALL_EXTERN = 91,  /* operand_a: extern symbol index */
    OP_RETURN      = 92,
    OP_RETURN_VOID = 93,
    OP_TAIL_CALL   = 94,  /* tail-call optimization (v1.2+) */

    /* ---- Memory ---- */
    OP_ALLOC     = 100,  /* operand_a: size in bytes */
    OP_FREE      = 101,
    OP_LOAD_PTR  = 102,
    OP_STORE_PTR = 103,

    /* ---- Strings ---- */
    OP_STR_CONCAT = 110,
    OP_STR_LEN    = 111,

    /* ---- Type casts ---- */
    OP_I2F = 120,
    OP_F2I = 121,
    OP_I2B = 122,  /* int to bool */
    OP_B2I = 123,  /* bool to int */

    /* ---- I/O (stdlib bridge) ---- */
    OP_PRINT   = 130,
    OP_PRINTLN = 131,
    OP_EPRINT  = 132,  /* print to stderr */

    /* ---- Debug / meta ---- */
    OP_BREAKPOINT = 200,  /* triggers debugger if attached */
    OP_NOP        = 201,  /* no-op, used by optimizer */
    OP_LINE_INFO  = 202,  /* operand_a: line, operand_b: col — debug only */

    OP_HALT = 255,

    /* Sentinel — keep last */
    OP_MAX_OPCODE = 256
} OpCode;

/* A single IR instruction — 24 bytes (aligned) */
typedef struct {
    OpCode  op;          /* 4 bytes */
    int32_t reserved;    /* 4 bytes — must be 0, reserved for flags */
    int64_t operand_a;   /* 8 bytes */
    int64_t operand_b;   /* 8 bytes */
} IRInstr;

/* Debug source map entry */
typedef struct {
    int32_t instr_index;
    int32_t line;
    int32_t col;
} IRSourceMap;

/* A compiled function */
typedef struct {
    char          name[128];
    IRInstr      *instrs;
    int           instr_count;
    int           local_count;
    int           param_count;
    IRSourceMap  *source_map;   /* nullable — present if ASTRA_FLAG_DEBUG */
    int           source_map_count;
} IRFunction;

/* The full module emitted by the compiler */
typedef struct {
    IRFunction  *functions;
    int          func_count;
    char       **string_pool;
    int          string_count;
    int          entry_func_index;
    uint32_t     version;        /* ASTRA_BYTECODE_VERSION at compile time */
    uint32_t     flags;          /* ASTRA_FLAG_* */
} IRModule;

/* Version compatibility check */
static inline int astra_version_compatible(uint32_t file_ver) {
    uint16_t file_major = (uint16_t)(file_ver >> 16);
    uint16_t cur_major  = (uint16_t)(ASTRA_BYTECODE_VERSION >> 16);
    return file_major == cur_major;
}

#ifdef __cplusplus
}
#endif

#endif /* ASTRA_IR_H */
