#pragma once
#include "../ast/ast.h"
#include "../../include/astra_ir.h"
#include <vector>
#include <string>
#include <unordered_map>

namespace astra {

// Internal representation before flattening to C IRModule
struct IRFunctionBuf {
    std::string              name;
    std::vector<IRInstr>     instrs;
    int                      local_count  = 0;
    int                      param_count  = 0;
};

class IRCodegen {
public:
    IRModule generate(const AstraModule &mod);

private:
    std::vector<IRFunctionBuf>              funcs_;

    // per-function state
    std::vector<IRInstr>                    instrs_;
    std::unordered_map<std::string, int>    locals_;
    int                                     local_cnt_ = 0;

    // string pool
    std::unordered_map<std::string, int>    str_idx_;

    // function index table
    std::unordered_map<std::string, int>    fn_idx_;

    void gen_items(const std::vector<StmtPtr> &items, const std::string &prefix);
    void gen_fn(const FnDeclStmt &fn, const std::string &qname = "");
    void gen_stmt(const StmtPtr &s);
    void gen_block(const BlockStmt &b);
    void gen_var_decl(const VarDeclStmt &v);
    void gen_if(const IfStmt &s);
    void gen_while(const WhileStmt &s);
    void gen_return(const ReturnStmt &s);
    void gen_expr(const ExprPtr &e);
    void gen_call(const CallExpr &c);

    void emit(OpCode op, int64_t a = 0, int64_t b = 0);
    int  emit_placeholder(OpCode op);
    void patch(int idx, int64_t a);

    int  intern_string(const std::string &s);
    int  current_pos() const { return (int)instrs_.size(); }
};

} // namespace astra
