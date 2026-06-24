#include "sema.h"
#include <sstream>
#include <variant>

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

std::string SemanticAnalyzer::type_name(TypePtr t) {
    if (!t) return "<unresolved>";
    if (t->kind == TypeNode::Kind::Named) return t->name;
    if (t->kind == TypeNode::Kind::Pointer) return "*" + type_name(t->inner);
    if (t->kind == TypeNode::Kind::Array) return "[" + type_name(t->inner) + "]";
    if (t->kind == TypeNode::Kind::Fn) return "fn";
    switch (t->prim) {
        case PrimType::I8: return "i8";
        case PrimType::I16: return "i16";
        case PrimType::I32: return "i32";
        case PrimType::I64: return "i64";
        case PrimType::U8: return "u8";
        case PrimType::U16: return "u16";
        case PrimType::U32: return "u32";
        case PrimType::U64: return "u64";
        case PrimType::F32: return "f32";
        case PrimType::F64: return "f64";
        case PrimType::Bool: return "bool";
        case PrimType::Str: return "str";
        case PrimType::Void: return "void";
    }
    return "<unknown>";
}

bool SemanticAnalyzer::is_integer(TypePtr t) {
    if (!t || t->kind != TypeNode::Kind::Primitive) return false;
    return t->prim == PrimType::I8 || t->prim == PrimType::I16 ||
           t->prim == PrimType::I32 || t->prim == PrimType::I64 ||
           t->prim == PrimType::U8 || t->prim == PrimType::U16 ||
           t->prim == PrimType::U32 || t->prim == PrimType::U64;
}

bool SemanticAnalyzer::is_numeric(TypePtr t) {
    if (!t || t->kind != TypeNode::Kind::Primitive) return false;
    return is_integer(t) || t->prim == PrimType::F32 || t->prim == PrimType::F64;
}

bool SemanticAnalyzer::is_bool(TypePtr t) {
    return t && t->kind == TypeNode::Kind::Primitive && t->prim == PrimType::Bool;
}

bool SemanticAnalyzer::is_void(TypePtr t) {
    return t && t->kind == TypeNode::Kind::Primitive && t->prim == PrimType::Void;
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
    collect_structs(mod.items);
    auto main_it = fn_table_.find("main");
    if (main_it == fn_table_.end() || main_it->second->is_extern) {
        type_error("missing required entrypoint 'fn main() -> void'", 0, 0);
    }
    if (!main_it->second->params.empty()) {
        type_error("entrypoint 'main' must not take parameters", 0, 0);
    }
    if (!is_void(main_it->second->ret_type)) {
        type_error("entrypoint 'main' must return void", 0, 0);
    }
    push_scope();
    for (auto &item : mod.items) analyze_stmt(item);
    pop_scope();
}

void SemanticAnalyzer::collect_structs(const std::vector<StmtPtr> &items) {
    for (auto &item : items) {
        if (auto *st = std::get_if<StructDeclStmt>(&item->data)) {
            struct_table_[st->name] = st;
        } else if (auto *mod = std::get_if<ModDeclStmt>(&item->data)) {
            collect_structs(mod->items);
        }
    }
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
    if (!ty || is_void(ty)) {
        type_error("variable '" + v.name + "' requires a non-void initializer or explicit type", 0, 0);
    }
    if (v.type_ann && inferred && !types_compatible(v.type_ann, inferred)) {
        type_error("type mismatch in declaration of '" + v.name + "': expected " +
                   type_name(v.type_ann) + ", found " + type_name(inferred), 0, 0);
    }
    int slot = scopes_.empty() ? 0 : scopes_.back().next_slot();
    define({v.name, ty, v.is_mut, slot});
}

void SemanticAnalyzer::analyze_if(IfStmt &s) {
    TypePtr cond = analyze_expr(s.cond);
    if (!is_bool(cond)) type_error("if condition must be bool, found " + type_name(cond), s.cond->line, s.cond->col);
    analyze_stmt(s.then_block);
    if (s.else_block) analyze_stmt(*s.else_block);
}

void SemanticAnalyzer::analyze_while(WhileStmt &s) {
    TypePtr cond = analyze_expr(s.cond);
    if (!is_bool(cond)) type_error("while condition must be bool, found " + type_name(cond), s.cond->line, s.cond->col);
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
            type_error("return type mismatch: expected " + type_name(ret) +
                       ", found " + type_name(vt), (*s.value)->line, (*s.value)->col);
        }
    } else if (ret && !is_void(ret)) {
        type_error("return type mismatch: expected " + type_name(ret) + ", found void", 0, 0);
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
            if (node.op == "&&" || node.op == "||") {
                if (!is_bool(l) || !is_bool(r)) {
                    type_error("logical operator '" + node.op + "' requires bool operands", e->line, e->col);
                }
                return make_prim(PrimType::Bool);
            }
            // comparison ops return bool
            if (node.op == "==" || node.op == "!=" || node.op == "<" ||
                node.op == ">"  || node.op == "<=" || node.op == ">=") {
                if (!types_compatible(l, r)) {
                    type_error("comparison type mismatch: " + type_name(l) +
                               " and " + type_name(r), e->line, e->col);
                }
                return make_prim(PrimType::Bool);
            }
            if (node.op == "&" || node.op == "|" || node.op == "^" ||
                node.op == "<<" || node.op == ">>" || node.op == "%") {
                if (!is_integer(l) || !is_integer(r) || !types_compatible(l, r)) {
                    type_error("operator '" + node.op + "' requires matching integer operands", e->line, e->col);
                }
                return l;
            }
            if (node.op == "+" || node.op == "-" || node.op == "*" || node.op == "/") {
                if (!is_numeric(l) || !is_numeric(r) || !types_compatible(l, r)) {
                    type_error("operator '" + node.op + "' requires matching numeric operands", e->line, e->col);
                }
                return l;
            }
            return l; // arithmetic: return lhs type
        }
        if constexpr (std::is_same_v<T, UnopExpr>) {
            TypePtr inner = analyze_expr(node.operand);
            if (node.op == "!" && !is_bool(inner)) {
                type_error("operator '!' requires bool operand", e->line, e->col);
            }
            if ((node.op == "-" || node.op == "~") && !is_numeric(inner)) {
                type_error("operator '" + node.op + "' requires numeric operand", e->line, e->col);
            }
            return inner;
        }
        if constexpr (std::is_same_v<T, CallExpr>) {
            return analyze_call(node, e);
        }
        if constexpr (std::is_same_v<T, AssignExpr>) {
            Symbol *target = nullptr;
            if (auto *ident = std::get_if<IdentExpr>(&node.lhs->data)) {
                target = lookup(ident->name);
                if (!target) type_error("undefined symbol '" + ident->name + "'", node.lhs->line, node.lhs->col);
            } else {
                type_error("invalid assignment target", node.lhs->line, node.lhs->col);
            }
            if (!target->is_mut) {
                type_error("cannot assign to immutable binding '" + target->name + "'", node.lhs->line, node.lhs->col);
            }
            TypePtr rhs = analyze_expr(node.rhs);
            if (!types_compatible(target->type, rhs)) {
                type_error("assignment type mismatch for '" + target->name + "': expected " +
                           type_name(target->type) + ", found " + type_name(rhs),
                           node.rhs->line, node.rhs->col);
            }
            return rhs;
        }
        if constexpr (std::is_same_v<T, CastExpr>) {
            analyze_expr(node.operand);
            return node.target;
        }
        if constexpr (std::is_same_v<T, FieldExpr>) {
            TypePtr base_ty = analyze_expr(node.base);
            if (!base_ty || base_ty->kind != TypeNode::Kind::Named) {
                type_error("field access requires a struct value", e->line, e->col);
            }
            auto st = struct_table_.find(base_ty->name);
            if (st == struct_table_.end()) {
                type_error("unknown struct type '" + base_ty->name + "'", e->line, e->col);
            }
            for (const auto &field : st->second->fields) {
                if (field.name == node.field) return field.type;
            }
            type_error("struct '" + base_ty->name + "' has no field '" + node.field + "'", e->line, e->col);
            return make_prim(PrimType::Void);
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
        if (ident->name == "println" || ident->name == "print") {
            if (c.args.size() != 1) {
                type_error("function '" + ident->name + "' expects 1 argument, got " +
                           std::to_string(c.args.size()), e->line, e->col);
            }
            for (auto &arg : c.args) analyze_expr(arg);
            return make_prim(PrimType::Void);
        }
        auto it = fn_table_.find(ident->name);
        if (it != fn_table_.end()) {
            if (it->second->is_extern) {
                type_error("extern function calls are not supported by this runtime yet", e->line, e->col);
            }
            if (c.args.size() != it->second->params.size()) {
                type_error("function '" + ident->name + "' expects " +
                           std::to_string(it->second->params.size()) + " arguments, got " +
                           std::to_string(c.args.size()), e->line, e->col);
            }
            for (size_t i = 0; i < c.args.size(); i++) {
                TypePtr arg_ty = analyze_expr(c.args[i]);
                TypePtr param_ty = it->second->params[i].type;
                if (!types_compatible(param_ty, arg_ty)) {
                    type_error("argument " + std::to_string(i + 1) + " for '" + ident->name +
                               "' expected " + type_name(param_ty) + ", found " +
                               type_name(arg_ty), c.args[i]->line, c.args[i]->col);
                }
            }
            return it->second->ret_type;
        }
        Symbol *sym = lookup(ident->name);
        if (!sym) {
            type_error("undefined function '" + ident->name + "'", e->line, e->col);
        }
        type_error("symbol '" + ident->name + "' is not callable", e->line, e->col);
    }
    analyze_expr(c.callee);
    for (auto &arg : c.args) analyze_expr(arg);
    type_error("unsupported dynamic function call", e->line, e->col);
    return make_prim(PrimType::Void);
}

} // namespace astra
