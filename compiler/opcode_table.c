#include "../include/astra_opcode_table.h"
#include <stdio.h>
#include <stddef.h>

/* -----------------------------------------------------------------------
 * Opcode registry — every entry is the authoritative definition.
 * stack_effect: +1 = pushes one value, -1 = pops one, -2 = pops two, etc.
 * ----------------------------------------------------------------------- */
static const AstraOpcodeInfo OPCODE_TABLE[] = {
    /* op                  name             effect  operands  lifecycle              since */
    { OP_PUSH_I64,        "PUSH_I64",        +1,    1,  ASTRA_OP_STABLE,     0 },
    { OP_PUSH_F64,        "PUSH_F64",        +1,    1,  ASTRA_OP_STABLE,     0 },
    { OP_PUSH_BOOL,       "PUSH_BOOL",       +1,    1,  ASTRA_OP_STABLE,     0 },
    { OP_PUSH_NULL,       "PUSH_NULL",       +1,    0,  ASTRA_OP_STABLE,     0 },
    { OP_PUSH_STR,        "PUSH_STR",        +1,    1,  ASTRA_OP_STABLE,     0 },
    { OP_POP,             "POP",             -1,    0,  ASTRA_OP_STABLE,     0 },
    { OP_DUP,             "DUP",             +1,    0,  ASTRA_OP_STABLE,     0 },
    { OP_SWAP,            "SWAP",             0,    0,  ASTRA_OP_STABLE,     2 },
    { OP_LOAD_LOCAL,      "LOAD_LOCAL",      +1,    1,  ASTRA_OP_STABLE,     0 },
    { OP_STORE_LOCAL,     "STORE_LOCAL",     -1,    1,  ASTRA_OP_STABLE,     0 },
    { OP_LOAD_GLOBAL,     "LOAD_GLOBAL",     +1,    1,  ASTRA_OP_STABLE,     0 },
    { OP_STORE_GLOBAL,    "STORE_GLOBAL",    -1,    1,  ASTRA_OP_STABLE,     0 },
    { OP_IADD,            "IADD",            -1,    0,  ASTRA_OP_STABLE,     0 },
    { OP_ISUB,            "ISUB",            -1,    0,  ASTRA_OP_STABLE,     0 },
    { OP_IMUL,            "IMUL",            -1,    0,  ASTRA_OP_STABLE,     0 },
    { OP_IDIV,            "IDIV",            -1,    0,  ASTRA_OP_STABLE,     0 },
    { OP_IMOD,            "IMOD",            -1,    0,  ASTRA_OP_STABLE,     0 },
    { OP_INEG,            "INEG",             0,    0,  ASTRA_OP_STABLE,     0 },
    { OP_FADD,            "FADD",            -1,    0,  ASTRA_OP_STABLE,     0 },
    { OP_FSUB,            "FSUB",            -1,    0,  ASTRA_OP_STABLE,     0 },
    { OP_FMUL,            "FMUL",            -1,    0,  ASTRA_OP_STABLE,     0 },
    { OP_FDIV,            "FDIV",            -1,    0,  ASTRA_OP_STABLE,     0 },
    { OP_FNEG,            "FNEG",             0,    0,  ASTRA_OP_STABLE,     0 },
    { OP_IEQ,             "IEQ",             -1,    0,  ASTRA_OP_STABLE,     0 },
    { OP_INEQ,            "INEQ",            -1,    0,  ASTRA_OP_STABLE,     0 },
    { OP_ILT,             "ILT",             -1,    0,  ASTRA_OP_STABLE,     0 },
    { OP_IGT,             "IGT",             -1,    0,  ASTRA_OP_STABLE,     0 },
    { OP_ILTE,            "ILTE",            -1,    0,  ASTRA_OP_STABLE,     0 },
    { OP_IGTE,            "IGTE",            -1,    0,  ASTRA_OP_STABLE,     0 },
    { OP_FEQ,             "FEQ",             -1,    0,  ASTRA_OP_STABLE,     0 },
    { OP_FNEQ,            "FNEQ",            -1,    0,  ASTRA_OP_STABLE,     0 },
    { OP_FLT,             "FLT",             -1,    0,  ASTRA_OP_STABLE,     0 },
    { OP_FGT,             "FGT",             -1,    0,  ASTRA_OP_STABLE,     0 },
    { OP_FLTE,            "FLTE",            -1,    0,  ASTRA_OP_STABLE,     0 },
    { OP_FGTE,            "FGTE",            -1,    0,  ASTRA_OP_STABLE,     0 },
    { OP_AND,             "AND",             -1,    0,  ASTRA_OP_STABLE,     0 },
    { OP_OR,              "OR",              -1,    0,  ASTRA_OP_STABLE,     0 },
    { OP_NOT,             "NOT",              0,    0,  ASTRA_OP_STABLE,     0 },
    { OP_BAND,            "BAND",            -1,    0,  ASTRA_OP_STABLE,     0 },
    { OP_BOR,             "BOR",             -1,    0,  ASTRA_OP_STABLE,     0 },
    { OP_BXOR,            "BXOR",            -1,    0,  ASTRA_OP_STABLE,     0 },
    { OP_BNOT,            "BNOT",             0,    0,  ASTRA_OP_STABLE,     0 },
    { OP_LSHIFT,          "LSHIFT",          -1,    0,  ASTRA_OP_STABLE,     0 },
    { OP_RSHIFT,          "RSHIFT",          -1,    0,  ASTRA_OP_STABLE,     0 },
    { OP_JMP,             "JMP",              0,    1,  ASTRA_OP_STABLE,     0 },
    { OP_JMP_IF,          "JMP_IF",          -1,    1,  ASTRA_OP_STABLE,     0 },
    { OP_JMP_IFNOT,       "JMP_IFNOT",       -1,    1,  ASTRA_OP_STABLE,     0 },
    { OP_CALL,            "CALL",             0,    2,  ASTRA_OP_STABLE,     0 },
    { OP_CALL_EXTERN,     "CALL_EXTERN",      0,    2,  ASTRA_OP_STABLE,     0 },
    { OP_RETURN,          "RETURN",          -1,    0,  ASTRA_OP_STABLE,     0 },
    { OP_RETURN_VOID,     "RETURN_VOID",      0,    0,  ASTRA_OP_STABLE,     0 },
    { OP_TAIL_CALL,       "TAIL_CALL",        0,    2,  ASTRA_OP_ADDED,      2 },
    { OP_ALLOC,           "ALLOC",           +1,    1,  ASTRA_OP_STABLE,     0 },
    { OP_FREE,            "FREE",            -1,    0,  ASTRA_OP_STABLE,     0 },
    { OP_LOAD_PTR,        "LOAD_PTR",        -1,    0,  ASTRA_OP_STABLE,     0 },
    { OP_STORE_PTR,       "STORE_PTR",       -3,    0,  ASTRA_OP_STABLE,     0 },
    { OP_STR_CONCAT,      "STR_CONCAT",      -1,    0,  ASTRA_OP_STABLE,     0 },
    { OP_STR_LEN,         "STR_LEN",          0,    0,  ASTRA_OP_ADDED,      2 },
    { OP_I2F,             "I2F",              0,    0,  ASTRA_OP_STABLE,     0 },
    { OP_F2I,             "F2I",              0,    0,  ASTRA_OP_STABLE,     0 },
    { OP_I2B,             "I2B",              0,    0,  ASTRA_OP_ADDED,      2 },
    { OP_B2I,             "B2I",              0,    0,  ASTRA_OP_ADDED,      2 },
    { OP_PRINT,           "PRINT",           -1,    0,  ASTRA_OP_STABLE,     0 },
    { OP_PRINTLN,         "PRINTLN",         -1,    0,  ASTRA_OP_STABLE,     0 },
    { OP_EPRINT,          "EPRINT",          -1,    0,  ASTRA_OP_ADDED,      2 },
    { OP_BREAKPOINT,      "BREAKPOINT",       0,    0,  ASTRA_OP_ADDED,      2 },
    { OP_NOP,             "NOP",              0,    0,  ASTRA_OP_ADDED,      2 },
    { OP_LINE_INFO,       "LINE_INFO",        0,    2,  ASTRA_OP_ADDED,      2 },
    { OP_HALT,            "HALT",             0,    0,  ASTRA_OP_STABLE,     0 },
    { 0, NULL, 0, 0, ASTRA_OP_STABLE, 0 } /* sentinel */
};

const AstraOpcodeInfo *astra_opcode_info(unsigned int op) {
    for (int i = 0; OPCODE_TABLE[i].name != NULL; i++) {
        if (OPCODE_TABLE[i].opcode == op) return &OPCODE_TABLE[i];
    }
    return NULL;
}

int astra_opcode_emittable(unsigned int op) {
    const AstraOpcodeInfo *info = astra_opcode_info(op);
    if (!info) return 0;
    return info->lifecycle != ASTRA_OP_RESERVED &&
           info->lifecycle != ASTRA_OP_DEPRECATED;
}

void astra_opcode_table_print(void) {
    printf("%-16s  %-6s  %-8s  %-10s  %-12s  %s\n",
           "OPCODE", "VALUE", "EFFECT", "OPERANDS", "LIFECYCLE", "SINCE");
    printf("%-16s  %-6s  %-8s  %-10s  %-12s  %s\n",
           "------", "-----", "------", "--------", "---------", "-----");
    for (int i = 0; OPCODE_TABLE[i].name != NULL; i++) {
        const AstraOpcodeInfo *e = &OPCODE_TABLE[i];
        const char *lc = "stable";
        if (e->lifecycle == ASTRA_OP_ADDED)      lc = "added";
        if (e->lifecycle == ASTRA_OP_DEPRECATED)  lc = "deprecated";
        if (e->lifecycle == ASTRA_OP_RESERVED)    lc = "reserved";
        printf("%-16s  %-6u  %-8d  %-10d  %-12s  1.%u\n",
               e->name, e->opcode, e->stack_effect,
               e->operand_count, lc, e->since_minor);
    }
}
