#include "optimizer.h"
#include <algorithm>
#include <cmath>

namespace astra {

// ---- helpers ----
bool Optimizer::is_int_lit(const ExprPtr &e, int64_t &out) {
    if (auto *v = std::get_if<IntLitExpr>(&e->data)) { out = v->value; return true; }
    return false;
}
bool Optimizer::is_float_lit(const ExprPtr &e, double &out) {
    if (auto *v = std::get_if<FloatLitExpr>(&e->data)) { out = v->value; return true; }
    return false;
}
bool Optimizer::is_bool_lit(const ExprPtr &e, bool &out) {
    if (auto *v = std::get_if<BoolLitExpr>(&e->data)) { out = v->value; return true; }
    return false;
}

ExprPtr Optimizer::make_int(int64_t v, int line, int col) {
    auto e = std::make_shared<Expr>(); e->line = line; e->col = col;
    e->data = IntLitExpr{v}; return e;
}
ExprPtr Optimizer::make_float(double v, int line, int col) {
    auto e = std::make_shared<Expr>(); e->line = line; e->col = col;
    e->data = FloatLitExpr{v}; return e;
}
ExprPtr Optimizer::make_bool(bool v, int line, int col) {
    auto e = std::make_shared<Expr>(); e->line = line; e->col = col;
    e->data = BoolLitExpr{v}; return e;
}

// ---- entry ----
void Optimizer::run(AstraModule &mod) {
    for (auto &item : mod.items) opt_stmt(item);
}

void Optimizer::opt_stmt(StmtPtr &s) {
    std::visit([&](auto &node) {
        using T = std::decay_t<decltype(node)>;

        if constexpr (std::is_same_v<T, FnDeclStmt>) {
            if (node.body) opt_stmt(node.body);
        }
        else if constexpr (std::is_same_v<T, BlockStmt>) {
            opt_block(node);
        }
        else if constexpr (std::is_same_v<T, VarDeclStmt>) {
            if (node.init) node.init = opt_expr(node.init);
        }
        else if constexpr (std::is_same_v<T, ExprStmt>) {
            node.expr = opt_expr(node.expr);
        }
        else if constexpr (std::is_same_v<T, ReturnStmt>) {
            if (node.value) *node.value = opt_expr(*node.value);
        }
        else if constexpr (std::is_same_v<T, IfStmt>) {
            node.cond = opt_expr(node.cond);
            // dead code elimination on constant condition
            bool bval;
            if (is_bool_lit(node.cond, bval)) {
                if (bval) {
                    // always true: replace if with then block
                    opt_stmt(node.then_block);
                    s->data = std::get<BlockStmt>(node.then_block->data);
                } else {
                    // always false: replace with else block or empty block
                    if (node.else_block) {
                        opt_stmt(*node.else_block);
                        s->data = std::get<BlockStmt>((*node.else_block)->data);
                    } else {
                        s->data = BlockStmt{{}};
                    }
                }
                return;
            }
            opt_stmt(node.then_block);
            if (node.else_block) opt_stmt(*node.else_block);
        }
        else if constexpr (std::is_same_v<T, WhileStmt>) {
            node.cond = opt_expr(node.cond);
            // dead loop elimination: while false { ... }
            bool bval;
            if (is_bool_lit(node.cond, bval) && !bval) {
                s->data = BlockStmt{{}};
                return;
            }
            opt_stmt(node.body);
        }
        else if constexpr (std::is_same_v<T, ModDeclStmt>) {
            for (auto &i : node.items) opt_stmt(i);
        }
    }, s->data);
}

void Optimizer::opt_block(BlockStmt &b) {
    for (auto &s : b.stmts) opt_stmt(s);

    // remove empty block stmts produced by dead code elim
    b.stmts.erase(
        std::remove_if(b.stmts.begin(), b.stmts.end(), [](const StmtPtr &s) {
            if (auto *blk = std::get_if<BlockStmt>(&s->data))
                return blk->stmts.empty();
            return false;
        }),
        b.stmts.end()
    );
}

ExprPtr Optimizer::opt_expr(ExprPtr e) {
    std::visit([&](auto &node) {
        using T = std::decay_t<decltype(node)>;
        if constexpr (std::is_same_v<T, BinopExpr>) {
            node.lhs = opt_expr(node.lhs);
            node.rhs = opt_expr(node.rhs);
            ExprPtr folded = fold_binop(node, e->line, e->col);
            if (folded) { e = folded; return; }
            ExprPtr simplified = simplify_binop(node, e->line, e->col);
            if (simplified) { e = simplified; return; }
        }
        else if constexpr (std::is_same_v<T, UnopExpr>) {
            node.operand = opt_expr(node.operand);
            // fold -<int>
            int64_t iv; double fv; bool bv;
            if (node.op == "-" && is_int_lit(node.operand, iv)) {
                e = make_int(-iv, e->line, e->col); return;
            }
            if (node.op == "-" && is_float_lit(node.operand, fv)) {
                e = make_float(-fv, e->line, e->col); return;
            }
            if (node.op == "!" && is_bool_lit(node.operand, bv)) {
                e = make_bool(!bv, e->line, e->col); return;
            }
        }
        else if constexpr (std::is_same_v<T, CallExpr>) {
            for (auto &arg : node.args) arg = opt_expr(arg);
        }
        else if constexpr (std::is_same_v<T, AssignExpr>) {
            node.rhs = opt_expr(node.rhs);
        }
    }, e->data);
    return e;
}

ExprPtr Optimizer::fold_binop(BinopExpr &b, int line, int col) {
    int64_t li, ri; double lf, rf; bool lb, rb;

    // integer folding
    if (is_int_lit(b.lhs, li) && is_int_lit(b.rhs, ri)) {
        if (b.op == "+")  return make_int(li + ri, line, col);
        if (b.op == "-")  return make_int(li - ri, line, col);
        if (b.op == "*")  return make_int(li * ri, line, col);
        if (b.op == "/" && ri != 0) return make_int(li / ri, line, col);
        if (b.op == "%" && ri != 0) return make_int(li % ri, line, col);
        if (b.op == "==") return make_bool(li == ri, line, col);
        if (b.op == "!=") return make_bool(li != ri, line, col);
        if (b.op == "<")  return make_bool(li <  ri, line, col);
        if (b.op == ">")  return make_bool(li >  ri, line, col);
        if (b.op == "<=") return make_bool(li <= ri, line, col);
        if (b.op == ">=") return make_bool(li >= ri, line, col);
        if (b.op == "&")  return make_int(li & ri, line, col);
        if (b.op == "|")  return make_int(li | ri, line, col);
        if (b.op == "^")  return make_int(li ^ ri, line, col);
        if (b.op == "<<") return make_int(li << ri, line, col);
        if (b.op == ">>") return make_int(li >> ri, line, col);
    }

    // float folding
    if (is_float_lit(b.lhs, lf) && is_float_lit(b.rhs, rf)) {
        if (b.op == "+")  return make_float(lf + rf, line, col);
        if (b.op == "-")  return make_float(lf - rf, line, col);
        if (b.op == "*")  return make_float(lf * rf, line, col);
        if (b.op == "/" && rf != 0.0) return make_float(lf / rf, line, col);
        if (b.op == "==") return make_bool(lf == rf, line, col);
        if (b.op == "!=") return make_bool(lf != rf, line, col);
        if (b.op == "<")  return make_bool(lf <  rf, line, col);
        if (b.op == ">")  return make_bool(lf >  rf, line, col);
    }

    // bool folding
    if (is_bool_lit(b.lhs, lb) && is_bool_lit(b.rhs, rb)) {
        if (b.op == "&&") return make_bool(lb && rb, line, col);
        if (b.op == "||") return make_bool(lb || rb, line, col);
        if (b.op == "==") return make_bool(lb == rb, line, col);
        if (b.op == "!=") return make_bool(lb != rb, line, col);
    }

    return nullptr;
}

ExprPtr Optimizer::simplify_binop(BinopExpr &b, int line, int col) {
    int64_t iv; bool bv;

    // x + 0 => x,  0 + x => x
    if (b.op == "+" && is_int_lit(b.rhs, iv) && iv == 0) return b.lhs;
    if (b.op == "+" && is_int_lit(b.lhs, iv) && iv == 0) return b.rhs;

    // x - 0 => x
    if (b.op == "-" && is_int_lit(b.rhs, iv) && iv == 0) return b.lhs;

    // x * 1 => x,  1 * x => x
    if (b.op == "*" && is_int_lit(b.rhs, iv) && iv == 1) return b.lhs;
    if (b.op == "*" && is_int_lit(b.lhs, iv) && iv == 1) return b.rhs;

    // x * 0 => 0,  0 * x => 0
    if (b.op == "*" && is_int_lit(b.rhs, iv) && iv == 0) return make_int(0, line, col);
    if (b.op == "*" && is_int_lit(b.lhs, iv) && iv == 0) return make_int(0, line, col);

    // x / 1 => x
    if (b.op == "/" && is_int_lit(b.rhs, iv) && iv == 1) return b.lhs;

    // x && true => x,  true && x => x
    if (b.op == "&&" && is_bool_lit(b.rhs, bv) && bv)  return b.lhs;
    if (b.op == "&&" && is_bool_lit(b.lhs, bv) && bv)  return b.rhs;

    // x && false => false
    if (b.op == "&&" && is_bool_lit(b.rhs, bv) && !bv) return make_bool(false, line, col);
    if (b.op == "&&" && is_bool_lit(b.lhs, bv) && !bv) return make_bool(false, line, col);

    // x || false => x,  false || x => x
    if (b.op == "||" && is_bool_lit(b.rhs, bv) && !bv) return b.lhs;
    if (b.op == "||" && is_bool_lit(b.lhs, bv) && !bv) return b.rhs;

    // x || true => true
    if (b.op == "||" && is_bool_lit(b.rhs, bv) && bv)  return make_bool(true, line, col);
    if (b.op == "||" && is_bool_lit(b.lhs, bv) && bv)  return make_bool(true, line, col);

    return nullptr;
}

} // namespace astra
