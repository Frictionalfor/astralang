/**
 * AstraLang Bytecode Disassembler
 * Usage: astradis <file.asc>
 * Prints a human-readable listing of every instruction in the module.
 */
#include <cstdio>
#include <cstdint>
#include <cstring>
#include <cstdlib>

extern "C" {
#include "../include/astra_ir.h"
}

// ---- file helpers ----
static FILE *gf = nullptr;

static uint32_t ru32() {
    uint32_t v; fread(&v, 4, 1, gf); return v;
}
static int64_t ri64() {
    int64_t v; fread(&v, 8, 1, gf); return v;
}
static void rpad(int n) {
    uint8_t buf[16]; fread(buf, 1, n, gf);
}

static const char *opname(uint32_t op) {
    switch ((OpCode)op) {
        case OP_PUSH_I64:    return "PUSH_I64";
        case OP_PUSH_F64:    return "PUSH_F64";
        case OP_PUSH_BOOL:   return "PUSH_BOOL";
        case OP_PUSH_NULL:   return "PUSH_NULL";
        case OP_PUSH_STR:    return "PUSH_STR";
        case OP_POP:         return "POP";
        case OP_DUP:         return "DUP";
        case OP_LOAD_LOCAL:  return "LOAD_LOCAL";
        case OP_STORE_LOCAL: return "STORE_LOCAL";
        case OP_LOAD_GLOBAL: return "LOAD_GLOBAL";
        case OP_STORE_GLOBAL:return "STORE_GLOBAL";
        case OP_IADD:        return "IADD";
        case OP_ISUB:        return "ISUB";
        case OP_IMUL:        return "IMUL";
        case OP_IDIV:        return "IDIV";
        case OP_IMOD:        return "IMOD";
        case OP_INEG:        return "INEG";
        case OP_FADD:        return "FADD";
        case OP_FSUB:        return "FSUB";
        case OP_FMUL:        return "FMUL";
        case OP_FDIV:        return "FDIV";
        case OP_FNEG:        return "FNEG";
        case OP_IEQ:         return "IEQ";
        case OP_INEQ:        return "INEQ";
        case OP_ILT:         return "ILT";
        case OP_IGT:         return "IGT";
        case OP_ILTE:        return "ILTE";
        case OP_IGTE:        return "IGTE";
        case OP_AND:         return "AND";
        case OP_OR:          return "OR";
        case OP_NOT:         return "NOT";
        case OP_BAND:        return "BAND";
        case OP_BOR:         return "BOR";
        case OP_BXOR:        return "BXOR";
        case OP_BNOT:        return "BNOT";
        case OP_LSHIFT:      return "LSHIFT";
        case OP_RSHIFT:      return "RSHIFT";
        case OP_JMP:         return "JMP";
        case OP_JMP_IF:      return "JMP_IF";
        case OP_JMP_IFNOT:   return "JMP_IFNOT";
        case OP_CALL:        return "CALL";
        case OP_CALL_EXTERN: return "CALL_EXTERN";
        case OP_RETURN:      return "RETURN";
        case OP_RETURN_VOID: return "RETURN_VOID";
        case OP_ALLOC:       return "ALLOC";
        case OP_FREE:        return "FREE";
        case OP_LOAD_PTR:    return "LOAD_PTR";
        case OP_STORE_PTR:   return "STORE_PTR";
        case OP_STR_CONCAT:  return "STR_CONCAT";
        case OP_I2F:         return "I2F";
        case OP_F2I:         return "F2I";
        case OP_PRINT:       return "PRINT";
        case OP_PRINTLN:     return "PRINTLN";
        case OP_HALT:        return "HALT";
        default:             return "???";
    }
}

// ops that use operand_a
static int has_operand_a(uint32_t op) {
    switch ((OpCode)op) {
        case OP_PUSH_I64: case OP_PUSH_F64: case OP_PUSH_BOOL:
        case OP_PUSH_STR:
        case OP_LOAD_LOCAL: case OP_STORE_LOCAL:
        case OP_LOAD_GLOBAL: case OP_STORE_GLOBAL:
        case OP_JMP: case OP_JMP_IF: case OP_JMP_IFNOT:
        case OP_CALL: case OP_ALLOC:
            return 1;
        default: return 0;
    }
}

int main(int argc, char **argv) {
    if (argc < 2) {
        fprintf(stderr, "usage: astradis <file.asc>\n");
        return 1;
    }
    gf = fopen(argv[1], "rb");
    if (!gf) { perror(argv[1]); return 1; }

    // magic + version + flags
    char magic[5] = {0};
    fread(magic, 1, 4, gf);
    uint32_t ver   = ru32();
    uint32_t flags = ru32();
    printf("=== AstraLang Bytecode Disassembly ===\n");
    printf("File   : %s\n", argv[1]);
    printf("Magic  : %s\n", magic);
    printf("Version: %u.%u\n", ver >> 16, ver & 0xffff);
    printf("Flags  : 0x%02x%s%s%s\n", flags,
           (flags & 0x01) ? " [DEBUG]" : "",
           (flags & 0x02) ? " [OPTIMIZED]" : "",
           (flags & 0x04) ? " [STDLIB]" : "");
    printf("\n");

    // string pool
    uint32_t str_count = ru32();
    char **strings = (char **)calloc(str_count, sizeof(char *));
    printf("--- String Pool (%u entries) ---\n", str_count);
    for (uint32_t i = 0; i < str_count; i++) {
        uint32_t len = ru32();
        strings[i] = (char *)malloc(len + 1);
        fread(strings[i], 1, len, gf);
        strings[i][len] = '\0';
        printf("  [%u] \"%s\"\n", i, strings[i]);
    }
    printf("\n");

    // functions
    uint32_t func_count = ru32();
    printf("--- Functions (%u) ---\n\n", func_count);

    for (uint32_t fi = 0; fi < func_count; fi++) {
        char name[129] = {0};
        fread(name, 1, 128, gf);
        uint32_t params  = ru32();
        uint32_t locals  = ru32();
        uint32_t n_instr = ru32();

        printf("fn %s  (params=%u, locals=%u)\n", name, params, locals);
        printf("  %-6s  %-16s  %s\n", "IDX", "OPCODE", "OPERANDS");
        printf("  %-6s  %-16s  %s\n", "---", "------", "--------");

        for (uint32_t ii = 0; ii < n_instr; ii++) {
            uint32_t op_raw = ru32();
            rpad(4); // reserved field (was padding, now explicit)
            int64_t  a = ri64();
            int64_t  b = ri64();

            if (has_operand_a(op_raw)) {
                if ((OpCode)op_raw == OP_CALL) {
                    printf("  %-6u  %-16s  fn=%lld args=%lld\n",
                           ii, opname(op_raw), (long long)a, (long long)b);
                } else if ((OpCode)op_raw == OP_PUSH_STR) {
                    const char *s = (a < (int64_t)str_count) ? strings[a] : "?";
                    printf("  %-6u  %-16s  [%lld] \"%s\"\n",
                           ii, opname(op_raw), (long long)a, s);
                } else if ((OpCode)op_raw == OP_PUSH_F64) {
                    double f; memcpy(&f, &a, 8);
                    printf("  %-6u  %-16s  %g\n", ii, opname(op_raw), f);
                } else {
                    printf("  %-6u  %-16s  %lld\n",
                           ii, opname(op_raw), (long long)a);
                }
            } else {
                printf("  %-6u  %s\n", ii, opname(op_raw));
            }
        }
        printf("\n");
    }

    uint32_t entry = ru32();
    printf("Entry function index: %u\n", entry);

    fclose(gf);
    for (uint32_t i = 0; i < str_count; i++) free(strings[i]);
    free(strings);
    return 0;
}
