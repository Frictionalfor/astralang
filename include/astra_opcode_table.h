#ifndef ASTRA_OPCODE_TABLE_H
#define ASTRA_OPCODE_TABLE_H

#include "astra_version.h"
#include "astra_ir.h"

#ifdef __cplusplus
extern "C" {
#endif

/* -----------------------------------------------------------------------
 * Formal opcode registry.
 * Every opcode in the instruction set is listed here with:
 *   - its numeric value
 *   - its mnemonic name
 *   - stack effect: net change to stack depth (negative = pops, positive = pushes)
 *   - operand count (0, 1, or 2)
 *   - lifecycle state
 *   - version introduced (minor)
 * ----------------------------------------------------------------------- */

typedef struct {
    unsigned int      opcode;
    const char       *name;
    int               stack_effect;  /* net stack depth change */
    int               operand_count; /* number of operands used */
    AstraOpLifecycle  lifecycle;
    unsigned int      since_minor;   /* bytecode minor version when added */
} AstraOpcodeInfo;

/* Returns info for a given opcode, or NULL if unknown */
const AstraOpcodeInfo *astra_opcode_info(unsigned int op);

/* Validate that an opcode is safe to emit (not reserved/deprecated) */
int astra_opcode_emittable(unsigned int op);

/* Print the full opcode table to stdout */
void astra_opcode_table_print(void);

#ifdef __cplusplus
}
#endif

#endif /* ASTRA_OPCODE_TABLE_H */
