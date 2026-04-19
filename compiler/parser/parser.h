#pragma once
#include "../../include/astra_token.h"
#include "../ast/ast.h"
#include <vector>
#include <string>
#include <memory>

namespace astra {

class Parser {
public:
    explicit Parser(std::vector<Token> tokens);
    AstraModule parse_module(const std::string &name);

private:
    std::vector<Token> toks_;
    size_t             pos_;

    Token  cur();
    Token  peek(int offset = 1);
    Token  consume();
    Token  expect(TokenType t);
    bool   check(TokenType t);
    bool   match(TokenType t);

    // Top-level
    StmtPtr parse_item();
    StmtPtr parse_fn_decl(bool is_pub, bool is_async, bool is_extern);
    StmtPtr parse_struct_decl(bool is_pub);
    StmtPtr parse_mod_decl();
    StmtPtr parse_use_stmt();

    // Statements
    StmtPtr parse_stmt();
    StmtPtr parse_block();
    StmtPtr parse_var_decl();
    StmtPtr parse_return_stmt();
    StmtPtr parse_if_stmt();
    StmtPtr parse_while_stmt();
    StmtPtr parse_for_stmt();

    // Expressions (Pratt parser)
    ExprPtr parse_expr(int min_prec = 0);
    ExprPtr parse_unary();
    ExprPtr parse_primary();
    ExprPtr parse_postfix(ExprPtr base);
    int     get_infix_prec(TokenType t);
    ExprPtr parse_infix(ExprPtr lhs, int min_prec);

    // Types
    TypePtr parse_type();

    void error(const std::string &msg);
};

} // namespace astra
