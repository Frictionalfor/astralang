#pragma once
#include "../ast/ast.h"
#include "opt_level.h"

namespace astra {

/**
 * AST-level optimizer pass.
 * Runs before IR codegen. Mutates the AST in place.
 *
 * O1 passes:
 *   1. Constant folding   — evaluate compile-time constant expressions
 *   2. Dead code elim     — remove unreachable branches (if true/false literals)
 *   3. Expression simplify — identity/zero rules (x+0, x*1, x*0, etc.)
 *
 * O2 adds:
 *   4. Dead loop elim     — remove while(false) loops
 *   5. Unary folding      — -5, !true evaluated at compile time
 *   6. Bool short-circuit — x && false => false, x || true => true
 */
class Optimizer {
public:
    explicit Optimizer(OptLevel level = OptLevel::O2) : level_(level) {}
    void run(AstraModule &mod);

private:
    OptLevel level_;

    void opt_stmt(StmtPtr &s);
    void opt_block(BlockStmt &b);
    ExprPtr opt_expr(ExprPtr e);

    ExprPtr fold_binop(BinopExpr &b, int line, int col);
    ExprPtr simplify_binop(BinopExpr &b, int line, int col);

    static bool is_int_lit(const ExprPtr &e, int64_t &out);
    static bool is_float_lit(const ExprPtr &e, double &out);
    static bool is_bool_lit(const ExprPtr &e, bool &out);

    static ExprPtr make_int(int64_t v, int line, int col);
    static ExprPtr make_float(double v, int line, int col);
    static ExprPtr make_bool(bool v, int line, int col);
};

} // namespace astra
