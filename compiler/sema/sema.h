#pragma once
#include "../ast/ast.h"
#include <unordered_map>
#include <string>
#include <vector>
#include <stdexcept>

namespace astra {

struct Symbol {
    std::string name;
    TypePtr     type;
    bool        is_mut;
    int         slot;   // local variable slot index
};

class Scope {
public:
    void define(const Symbol &sym);
    Symbol *lookup(const std::string &name);
    int next_slot() const { return (int)symbols_.size(); }
private:
    std::vector<Symbol> symbols_;
};

class SemanticAnalyzer {
public:
    void analyze(AstraModule &mod);

private:
    std::vector<Scope> scopes_;
    // function return type stack
    std::vector<TypePtr> ret_type_stack_;

    void push_scope();
    void pop_scope();
    Symbol *lookup(const std::string &name);
    void define(const Symbol &sym);

    void analyze_stmt(StmtPtr &s);
    void collect_fns(const std::vector<StmtPtr> &items, const std::string &prefix);
    void analyze_fn(FnDeclStmt &fn);
    void analyze_block(BlockStmt &blk);
    void analyze_var_decl(VarDeclStmt &v);
    void analyze_if(IfStmt &s);
    void analyze_while(WhileStmt &s);
    void analyze_for(ForStmt &s);
    void analyze_return(ReturnStmt &s);

    TypePtr analyze_expr(ExprPtr &e);
    TypePtr analyze_call(CallExpr &c, ExprPtr &e);

    bool types_compatible(TypePtr a, TypePtr b);
    void type_error(const std::string &msg, int line, int col);

    // function table: name -> FnDeclStmt*
    std::unordered_map<std::string, FnDeclStmt*> fn_table_;
};

} // namespace astra
