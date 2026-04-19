#include "sema.h"
#include <sstream>

namespace astra {

// ---- Scope ----
void Scope::define(const Symbol &sym) { symbols_.push_back(sym); }
Symbol *Scope::lookup(const std::string &name) {
    for (auto &s : symbols_) if (s.name == name) return &s;
    return nullptr;
}

// ---- SemanticAnalyzer ----
void SemanticAnalyzer::push_scope() { scopes_.emplace_back(); }
void SemanticAnalyzer::pop_scope()  { scopes_.pop_back(); }

Symbol *SemanticAnalyzer::lookup(const std::string &name) {
    for (int i = (int)scopes_.size()-1; i >= 0; i--) {
        Symbol *s = scopes_[i].lookup(name);
        if (s) return s;
    }
    return nullptr;
}

void SemanticAnalyzer::define(const Symbol &sym) {
    if (!scopes_.empty()) scopes_.back().define(sym);
}

void SemanticAnalyzer::type_error(const std::string &msg, int line, int col) {
    std::ostringstream ss;
    ss << "[sema] " << msg << " at " << line << ":" << col;
    throw std::runtime_error(ss.str());
}

static TypePtr make_prim(PrimType p) {
    auto t = std::make_shared<TypeNode>();
    t->kind = TypeNode::Kind::Primitive;
    t->prim = p;
    return t;
}

bool SemanticAnalyzer::types_compatible(TypePtr a, TypePtr b) {
    if (!a || !b) return true; // unresolved = permissive
    if (a->kind != b->kind) return false;
    if (a->kind == TypeNode::Kind::Primitive) return a->prim == b->prim;
    if (a->kind == TypeNode::Kind::Named)     return a->name == b->name;
    return true;
}

void SemanticAnalyzer::analyze(AstraModule &mod) {
    // Register built-in functions
    static FnDeclStmt println_fn, print_fn;
    println_fn.name = "println"; println_fn.ret_type = make_prim(PrimType::Void);
    print_fn.name   = "print";   print_fn.ret_type   = make_prim(PrimType::Void);
    fn_table_["println"] = &println_fn;
    fn_table_["print"]   = &print_fn;

    // First pass: collect function signatures (including inside mods)
    collect_fns(mod.items, "");
    push_scope();
    for (auto &item : mod.items) analyze_stmt(item);
    pop_scope();
}

void SemanticAnalyzer::collect_fns(const std::vector<StmtPtr> &items, const std::string &prefix) {
    for (auto &item : items) {
        if (auto *fn = std::get_if<FnDeclStmt>(&item->data)) {
            std::string qname = prefix.empty() ? fn->name : prefix + "::" + fn->name;
            fn_table_[qname] = fn;
        } else if (auto *mod = std::get_if<ModDeclStmt>(&item->data)) {
            std::string new_prefix = prefix.empty() ? mod->name : prefix + "::" + mod->name;
            collect_fns(mod->items, new_prefix);
        }
    }
}

void SemanticAnalyzer::analyze_stmt(StmtPtr &s) {
    std::visit([&](auto &node) {
        using T = std::decay_t<decltype(node)>;
        if constexpr (std::is_same_v<T, FnDeclStmt>)     analyze_fn(node);
        else if constexpr (std::is_same_v<T, VarDeclStmt>) analyze_var_decl(node);
        else if constexpr (std::is_same_v<T, BlockStmt>)   analyze_block(node);
        else if constexpr (std::is_same_v<T, IfStmt>)      analyze_if(node);
        else if constexpr (std::is_same_v<T, WhileStmt>)   analyze_while(node);
        else if constexpr (std::is_same_v<T, ForStmt>)     analyze_for(node);
        else if constexpr (std::is_same_v<T, ReturnStmt>)  analyze_return(node);
        else if constexpr (std::is_same_v<T, ExprStmt>)    analyze_expr(node.expr);
        else if constexpr (std::is_same_v<T, StructDeclStmt>) { /* TODO */ }
        else if constexpr (std::is_same_v<T, ModDeclStmt>) {
            // register bare names within mod scope so recursive calls resolve
            for (auto &i : node.items) {
                if (auto *fn = std::get_if<FnDeclStmt>(&i->data)) {
                    fn_table_[fn->name] = fn; // bare name for internal calls
                }
            }
            push_scope();
            for (auto &i : node.items) analyze_stmt(i);
            pop_scope();
        }
        else if constexpr (std::is_same_v<T, UseStmt>) { /* TODO: resolve imports */ }
    }, s->data);
}

void SemanticAnalyzer::analyze_fn(FnDeclStmt &fn) {
    if (fn.is_extern) return;
    push_scope();
    ret_type_stack_.push_back(fn.ret_type);
    int slot = 0;
    for (auto &p : fn.params) {
        define({p.name, p.type, false, slot++});
    }
    if (fn.body) analyze_stmt(fn.body);
    ret_type_stack_.pop_back();
    pop_scope();
}

void SemanticAnalyzer::analyze_block(BlockStmt &blk) {
    push_scope();
    for (auto &s : blk.stmts) analyze_stmt(s);
    pop_scope();
}

void SemanticAnalyzer::analyze_var_decl(VarDeclStmt &v) {
    TypePtr inferred = nullptr;
    if (v.init) inferred = analyze_expr(v.init);
    TypePtr ty = v.type_ann ? v.type_ann : inferred;
    if (!ty) ty = make_prim(PrimType::Void);
    if (v.type_ann && inferred && !types_compatible(v.type_ann, inferred)) {
        // soft warning for now
    }
    int slot = scopes_.empty() ? 0 : scopes_.back().next_slot();
    define({v.name, ty, v.is_mut, slot});
}

void SemanticAnalyzer::analyze_if(IfStmt &s) {
    analyze_expr(s.cond);
    analyze_stmt(s.then_block);
    if (s.else_block) analyze_stmt(*s.else_block);
}

void SemanticAnalyzer::analyze_while(WhileStmt &s) {
    analyze_expr(s.cond);
    analyze_stmt(s.body);
}

void SemanticAnalyzer::analyze_for(ForStmt &s) {
    analyze_expr(s.iter);
    push_scope();
    define({s.var, make_prim(PrimType::I64), false, 0});
    analyze_stmt(s.body);
    pop_scope();
}

void SemanticAnalyzer::analyze_return(ReturnStmt &s) {
    TypePtr ret = ret_type_stack_.empty() ? nullptr : ret_type_stack_.back();
    if (s.value) {
        TypePtr vt = analyze_expr(*s.value);
        if (ret && !types_compatible(ret, vt)) {
            // type mismatch warning
        }
    }
}

TypePtr SemanticAnalyzer::analyze_expr(ExprPtr &e) {
    TypePtr result = std::visit([&](auto &node) -> TypePtr {
        using T = std::decay_t<decltype(node)>;
        if constexpr (std::is_same_v<T, IntLitExpr>)   return make_prim(PrimType::I64);
        if constexpr (std::is_same_v<T, FloatLitExpr>) return make_prim(PrimType::F64);
        if constexpr (std::is_same_v<T, BoolLitExpr>)  return make_prim(PrimType::Bool);
        if constexpr (std::is_same_v<T, StrLitExpr>)   return make_prim(PrimType::Str);
        if constexpr (std::is_same_v<T, NullExpr>)     return make_prim(PrimType::Void);
        if constexpr (std::is_same_v<T, IdentExpr>) {
            Symbol *sym = lookup(node.name);
            if (!sym) {
                // check fn table
                auto it = fn_table_.find(node.name);
                if (it != fn_table_.end()) return make_prim(PrimType::Void);
                type_error("undefined symbol '" + node.name + "'", e->line, e->col);
            }
            return sym->type;
        }
        if constexpr (std::is_same_v<T, BinopExpr>) {
            TypePtr l = analyze_expr(node.lhs);
            TypePtr r = analyze_expr(node.rhs);
            // comparison ops return bool
            if (node.op == "==" || node.op == "!=" || node.op == "<" ||
                node.op == ">"  || node.op == "<=" || node.op == ">=")
                return make_prim(PrimType::Bool);
            return l; // arithmetic: return lhs type
        }
        if constexpr (std::is_same_v<T, UnopExpr>) {
            return analyze_expr(node.operand);
        }
        if constexpr (std::is_same_v<T, CallExpr>) {
            return analyze_call(node, e);
        }
        if constexpr (std::is_same_v<T, AssignExpr>) {
            analyze_expr(node.lhs);
            return analyze_expr(node.rhs);
        }
        if constexpr (std::is_same_v<T, CastExpr>) {
            analyze_expr(node.operand);
            return node.target;
        }
        if constexpr (std::is_same_v<T, FieldExpr>) {
            analyze_expr(node.base);
            return make_prim(PrimType::Void); // TODO: struct field lookup
        }
        if constexpr (std::is_same_v<T, IndexExpr>) {
            analyze_expr(node.base);
            analyze_expr(node.index);
            return make_prim(PrimType::Void);
        }
        if constexpr (std::is_same_v<T, AwaitExpr>) {
            return analyze_expr(node.inner);
        }
        return make_prim(PrimType::Void);
    }, e->data);
    e->resolved_type = result;
    return result;
}

TypePtr SemanticAnalyzer::analyze_call(CallExpr &c, ExprPtr &e) {
    // resolve callee name
    if (auto *ident = std::get_if<IdentExpr>(&c.callee->data)) {
        auto it = fn_table_.find(ident->name);
        if (it != fn_table_.end()) {
            for (auto &arg : c.args) analyze_expr(arg);
            return it->second->ret_type;
        }
    }
    analyze_expr(c.callee);
    for (auto &arg : c.args) analyze_expr(arg);
    return make_prim(PrimType::Void);
}

} // namespace astra
