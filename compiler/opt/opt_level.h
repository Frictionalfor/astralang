#pragma once

namespace astra {

/**
 * Optimization levels — mirrors GCC/Clang convention.
 *
 * O0 — no optimization. Fastest compile, best for debugging.
 *      All LINE_INFO and BREAKPOINT opcodes preserved.
 *
 * O1 — safe optimizations only.
 *      Constant folding, dead code elimination on trivially false branches,
 *      expression simplification. No reordering. Debug info preserved.
 *
 * O2 — full optimization (default for release builds).
 *      All O1 passes plus: dead loop elimination, identity simplification,
 *      NOP removal. Debug info stripped unless --debug also set.
 *
 * Determinism guarantee:
 *      All optimization passes are deterministic. Given the same source,
 *      the same bytecode is always produced regardless of system state.
 *
 * Safety guarantee:
 *      No optimization may change observable program behavior.
 *      Division by zero, null dereference, and overflow behavior
 *      are never optimized away.
 */
enum class OptLevel {
    O0 = 0,  // debug — no optimization
    O1 = 1,  // safe  — constant folding + dead code
    O2 = 2,  // full  — all passes (default)
};

inline OptLevel opt_level_from_string(const char *s) {
    if (s[0] == '0') return OptLevel::O0;
    if (s[0] == '1') return OptLevel::O1;
    return OptLevel::O2;
}

inline const char *opt_level_name(OptLevel l) {
    switch (l) {
        case OptLevel::O0: return "O0 (none)";
        case OptLevel::O1: return "O1 (safe)";
        case OptLevel::O2: return "O2 (full)";
    }
    return "O2";
}

} // namespace astra
