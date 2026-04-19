#include "codegen.h"
#include <cstring>
#include <stdexcept>

namespace astra {

void IRCodegen::emit(OpCode op, int64_t a, int64_t b) {
    IRInstr instr;
    instr.op         = op;
    instr.reserved   = 0;
    instr.operand_a  = a;
    instr.operand_b  = b;
    instrs_.push_back(instr);
}

int IRCodegen::emit_placeholder(OpCode op) {
    int idx = current_pos();
    emit(op, 0);
    return idx;
}

void IRCodegen::patch(int idx, int64_t a) {
    instrs_[idx].operand_a = a;
}

int IRCodegen::intern_string(const std::string &s) {
    auto it = str_idx_.find(s);
    if (it != str_idx_.end()) return it->second;
    int idx = (int)str_idx_.size();
    str_idx_[s] = idx;
    return idx;
}

// Recursive helper to collect all function indices including inside mods
static void collect_fn_indices(
    const std::vector<StmtPtr> &items,
    std::unordered_map<std::string, int> &fn_idx,
    int &counter,
    const std::string &prefix = "")
{
    for (auto &item : items) {
        if (auto *fn = std::get_if<FnDeclStmt>(&item->data)) {
            std::string qname = prefix.empty() ? fn->name : prefix + "::" + fn->name;
            fn_idx[qname] = counter++;
        } else if (auto *mod = std::get_if<ModDeclStmt>(&item->data)) {
            std::string new_prefix = prefix.empty() ? mod->name : prefix + "::" + mod->name;
            collect_fn_indices(mod->items, fn_idx, counter, new_prefix);
        }
    }
}

IRModule IRCodegen::generate(const AstraModule &mod) {
    // First pass: assign function indices (including inside mods)
    int fi = 0;
    collect_fn_indices(mod.items, fn_idx_, fi);

    // Second pass: generate each function (including inside mods)
    gen_items(mod.items, "");

    // --- Flatten to C IRModule ---
    IRModule module;
    module.func_count       = (int)funcs_.size();
    module.functions        = new IRFunction[module.func_count];
    module.string_count     = (int)str_idx_.size();
    module.string_pool      = module.string_count > 0 ? new char*[module.string_count] : nullptr;
    module.entry_func_index = 0;

    // string pool
    for (auto &[s, i] : str_idx_) {
        module.string_pool[i] = new char[s.size() + 1];
        strcpy(module.string_pool[i], s.c_str());
    }

    // functions
    for (int i = 0; i < module.func_count; i++) {
        IRFunctionBuf &buf = funcs_[i];
        IRFunction    &dst = module.functions[i];
        strncpy(dst.name, buf.name.c_str(), 127);
        dst.name[127]   = '\0';
        dst.param_count = buf.param_count;
        dst.local_count = buf.local_count;
        dst.instr_count = (int)buf.instrs.size();
        dst.instrs      = new IRInstr[dst.instr_count];
        memcpy(dst.instrs, buf.instrs.data(), dst.instr_count * sizeof(IRInstr));
    }

    // entry point = "main"
    auto it = fn_idx_.find("main");
    if (it != fn_idx_.end()) module.entry_func_index = it->second;

    return module;
}

void IRCodegen::gen_items(const std::vector<StmtPtr> &items, const std::string &prefix) {
    for (auto &item : items) {
        if (auto *fn = std::get_if<FnDeclStmt>(&item->data)) {
            if (!fn->is_extern) {
                std::string qname = prefix.empty() ? fn->name : prefix + "::" + fn->name;
                gen_fn(*fn, qname);
            }
        } else if (auto *mod = std::get_if<ModDeclStmt>(&item->data)) {
            std::string new_prefix = prefix.empty() ? mod->name : prefix + "::" + mod->name;
            gen_items(mod->items, new_prefix);
        }
    }
}

void IRCodegen::gen_fn(const FnDeclStmt &fn, const std::string &qname) {
    instrs_.clear();
    locals_.clear();
    local_cnt_ = 0;

    // params occupy first slots
    for (auto &p : fn.params) {
        locals_[p.name] = local_cnt_++;
    }

    if (fn.body) gen_stmt(fn.body);

    // ensure function ends with a return
    if (instrs_.empty() || instrs_.back().op != OP_RETURN_VOID)
        emit(OP_RETURN_VOID);

    IRFunctionBuf buf;
    buf.name        = qname.empty() ? fn.name : qname;
    buf.instrs      = instrs_;
    buf.local_count = local_cnt_;
    buf.param_count = (int)fn.params.size();
    funcs_.push_back(std::move(buf));
}

void IRCodegen::gen_stmt(const StmtPtr &s) {
    std::visit([&](const auto &node) {
        using T = std::decay_t<decltype(node)>;
        if constexpr (std::is_same_v<T, BlockStmt>)       gen_block(node);
        else if constexpr (std::is_same_v<T, VarDeclStmt>) gen_var_decl(node);
        else if constexpr (std::is_same_v<T, IfStmt>)      gen_if(node);
        else if constexpr (std::is_same_v<T, WhileStmt>)   gen_while(node);
        else if constexpr (std::is_same_v<T, ReturnStmt>)  gen_return(node);
        else if constexpr (std::is_same_v<T, ExprStmt>) {
            // Determine if this expression leaves a value on the stack
            bool leaves_value = true;

            if (auto *call = std::get_if<CallExpr>(&node.expr->data)) {
                if (auto *id = std::get_if<IdentExpr>(&call->callee->data)) {
                    if (id->name == "println" || id->name == "print") leaves_value = false;
                    else if (fn_idx_.count(id->name))                 leaves_value = false;
                }
            } else if (std::get_if<AssignExpr>(&node.expr->data)) {
                // Assignment as statement: don't DUP, just store and discard
                const AssignExpr &assign = std::get<AssignExpr>(node.expr->data);
                gen_expr(assign.rhs);
                if (auto *ident = std::get_if<IdentExpr>(&assign.lhs->data)) {
                    auto it = locals_.find(ident->name);
                    if (it != locals_.end()) emit(OP_STORE_LOCAL, it->second);
                }
                return; // nothing left on stack
            }

            gen_expr(node.expr);
            if (leaves_value) emit(OP_POP);
        }
        else if constexpr (std::is_same_v<T, ForStmt>) {
            // minimal: evaluate iter, discard
            gen_expr(node.iter);
            emit(OP_POP);
        }
        else if constexpr (std::is_same_v<T, FnDeclStmt>) { /* handled at module level */ }
        else if constexpr (std::is_same_v<T, ModDeclStmt>) {
            // mod functions are generated at module level via gen_items
        }
    }, s->data);
}

void IRCodegen::gen_block(const BlockStmt &b) {
    for (auto &s : b.stmts) gen_stmt(s);
}

void IRCodegen::gen_var_decl(const VarDeclStmt &v) {
    if (v.init) gen_expr(v.init);
    else        emit(OP_PUSH_NULL);

    int slot = local_cnt_++;
    locals_[v.name] = slot;
    emit(OP_STORE_LOCAL, slot);
}

void IRCodegen::gen_if(const IfStmt &s) {
    gen_expr(s.cond);
    int jmp_else = emit_placeholder(OP_JMP_IFNOT);
    gen_stmt(s.then_block);
    if (s.else_block) {
        int jmp_end = emit_placeholder(OP_JMP);
        patch(jmp_else, current_pos());
        gen_stmt(*s.else_block);
        patch(jmp_end, current_pos());
    } else {
        patch(jmp_else, current_pos());
    }
}

void IRCodegen::gen_while(const WhileStmt &s) {
    int loop_start = current_pos();
    gen_expr(s.cond);
    int jmp_end = emit_placeholder(OP_JMP_IFNOT);
    gen_stmt(s.body);
    emit(OP_JMP, loop_start);
    patch(jmp_end, current_pos());
}

void IRCodegen::gen_return(const ReturnStmt &s) {
    if (s.value) {
        gen_expr(*s.value);
        emit(OP_RETURN);
    } else {
        emit(OP_RETURN_VOID);
    }
}

void IRCodegen::gen_expr(const ExprPtr &e) {
    std::visit([&](const auto &node) {
        using T = std::decay_t<decltype(node)>;

        if constexpr (std::is_same_v<T, IntLitExpr>) {
            emit(OP_PUSH_I64, node.value);
        }
        else if constexpr (std::is_same_v<T, FloatLitExpr>) {
            int64_t bits;
            memcpy(&bits, &node.value, 8);
            emit(OP_PUSH_F64, bits);
        }
        else if constexpr (std::is_same_v<T, BoolLitExpr>) {
            emit(OP_PUSH_BOOL, node.value ? 1 : 0);
        }
        else if constexpr (std::is_same_v<T, StrLitExpr>) {
            emit(OP_PUSH_STR, intern_string(node.value));
        }
        else if constexpr (std::is_same_v<T, NullExpr>) {
            emit(OP_PUSH_NULL);
        }
        else if constexpr (std::is_same_v<T, IdentExpr>) {
            auto it = locals_.find(node.name);
            if (it != locals_.end()) emit(OP_LOAD_LOCAL, it->second);
            else                     emit(OP_LOAD_GLOBAL, 0);
        }
        else if constexpr (std::is_same_v<T, BinopExpr>) {
            gen_expr(node.lhs);
            gen_expr(node.rhs);
            const std::string &op = node.op;
            if      (op == "+")  emit(OP_IADD);
            else if (op == "-")  emit(OP_ISUB);
            else if (op == "*")  emit(OP_IMUL);
            else if (op == "/")  emit(OP_IDIV);
            else if (op == "%")  emit(OP_IMOD);
            else if (op == "==") emit(OP_IEQ);
            else if (op == "!=") emit(OP_INEQ);
            else if (op == "<")  emit(OP_ILT);
            else if (op == ">")  emit(OP_IGT);
            else if (op == "<=") emit(OP_ILTE);
            else if (op == ">=") emit(OP_IGTE);
            else if (op == "&&") emit(OP_AND);
            else if (op == "||") emit(OP_OR);
            else if (op == "&")  emit(OP_BAND);
            else if (op == "|")  emit(OP_BOR);
            else if (op == "^")  emit(OP_BXOR);
            else if (op == "<<") emit(OP_LSHIFT);
            else if (op == ">>") emit(OP_RSHIFT);
        }
        else if constexpr (std::is_same_v<T, UnopExpr>) {
            gen_expr(node.operand);
            if      (node.op == "-") emit(OP_INEG);
            else if (node.op == "!") emit(OP_NOT);
            else if (node.op == "~") emit(OP_BNOT);
        }
        else if constexpr (std::is_same_v<T, CallExpr>) {
            gen_call(node);
        }
        else if constexpr (std::is_same_v<T, AssignExpr>) {
            // In expression context: evaluate rhs, dup for result, store
            gen_expr(node.rhs);
            emit(OP_DUP);
            if (auto *ident = std::get_if<IdentExpr>(&node.lhs->data)) {
                auto it = locals_.find(ident->name);
                if (it != locals_.end()) emit(OP_STORE_LOCAL, it->second);
            }
        }
        else if constexpr (std::is_same_v<T, CastExpr>) {
            gen_expr(node.operand);
        }
        else if constexpr (std::is_same_v<T, FieldExpr>) {
            gen_expr(node.base);
        }
        else if constexpr (std::is_same_v<T, IndexExpr>) {
            gen_expr(node.base);
            gen_expr(node.index);
            emit(OP_LOAD_PTR);
        }
        else if constexpr (std::is_same_v<T, AwaitExpr>) {
            gen_expr(node.inner);
        }
    }, e->data);
}

void IRCodegen::gen_call(const CallExpr &c) {
    // push args left-to-right
    for (auto &arg : c.args) gen_expr(arg);

    if (auto *ident = std::get_if<IdentExpr>(&c.callee->data)) {
        if (ident->name == "println") { emit(OP_PRINTLN); return; }
        if (ident->name == "print")   { emit(OP_PRINT);   return; }

        // exact match first
        auto it = fn_idx_.find(ident->name);
        if (it != fn_idx_.end()) {
            emit(OP_CALL, it->second, (int64_t)c.args.size());
            return;
        }
        // suffix match: bare name inside a mod (e.g. "factorial" -> "math::factorial")
        for (auto &[qname, idx] : fn_idx_) {
            auto pos = qname.rfind("::");
            std::string bare = (pos == std::string::npos) ? qname : qname.substr(pos + 2);
            if (bare == ident->name) {
                emit(OP_CALL, idx, (int64_t)c.args.size());
                return;
            }
        }
    }
    // dynamic call fallback
    gen_expr(c.callee);
    emit(OP_CALL, -1, (int64_t)c.args.size());
}

} // namespace astra
