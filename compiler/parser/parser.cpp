#include "parser.h"
#include <stdexcept>
#include <sstream>

namespace astra {

Parser::Parser(std::vector<Token> tokens) : toks_(std::move(tokens)), pos_(0) {}

Token Parser::cur()  { return pos_ < toks_.size() ? toks_[pos_] : toks_.back(); }
Token Parser::peek(int o) { size_t i = pos_+o; return i < toks_.size() ? toks_[i] : toks_.back(); }
Token Parser::consume() { Token t = cur(); if (pos_ < toks_.size()) pos_++; return t; }

bool Parser::check(TokenType t) { return cur().type == t; }
bool Parser::match(TokenType t) { if (check(t)) { consume(); return true; } return false; }

Token Parser::expect(TokenType t) {
    if (!check(t)) {
        std::ostringstream ss;
        ss << "expected " << token_type_name(t) << " but got '"
           << std::string(cur().start, cur().length)
           << "' at " << cur().line << ":" << cur().col;
        error(ss.str());
    }
    return consume();
}

void Parser::error(const std::string &msg) {
    throw std::runtime_error("[parser] " + msg);
}

// ---- Module ----
AstraModule Parser::parse_module(const std::string &name) {
    AstraModule mod;
    mod.name = name;
    while (!check(TOK_EOF)) {
        mod.items.push_back(parse_item());
    }
    return mod;
}

// ---- Top-level items ----
StmtPtr Parser::parse_item() {
    bool is_pub    = match(TOK_PUB);
    bool is_async  = match(TOK_ASYNC);
    bool is_extern = match(TOK_EXTERN);

    if (check(TOK_FN))     return parse_fn_decl(is_pub, is_async, is_extern);
    if (check(TOK_STRUCT))  return parse_struct_decl(is_pub);
    if (check(TOK_MOD))     return parse_mod_decl();
    if (check(TOK_USE))     return parse_use_stmt();

    // fall through to statement
    return parse_stmt();
}

StmtPtr Parser::parse_fn_decl(bool is_pub, bool is_async, bool is_extern) {
    expect(TOK_FN);
    Token name_tok = expect(TOK_IDENT);
    std::string name(name_tok.start, name_tok.length);

    expect(TOK_LPAREN);
    std::vector<FnParam> params;
    while (!check(TOK_RPAREN) && !check(TOK_EOF)) {
        Token pname = expect(TOK_IDENT);
        expect(TOK_COLON);
        TypePtr ptype = parse_type();
        params.push_back({std::string(pname.start, pname.length), ptype});
        if (!match(TOK_COMMA)) break;
    }
    expect(TOK_RPAREN);

    TypePtr ret = std::make_shared<TypeNode>();
    ret->kind = TypeNode::Kind::Primitive;
    ret->prim = PrimType::Void;
    if (match(TOK_ARROW)) ret = parse_type();

    StmtPtr body = nullptr;
    if (is_extern) {
        expect(TOK_SEMICOLON);
    } else {
        body = parse_block();
    }

    auto s = std::make_shared<Stmt>();
    s->line = name_tok.line; s->col = name_tok.col;
    FnDeclStmt fn;
    fn.name = name; fn.params = params; fn.ret_type = ret;
    fn.body = body; fn.is_pub = is_pub; fn.is_async = is_async; fn.is_extern = is_extern;
    s->data = fn;
    return s;
}

StmtPtr Parser::parse_struct_decl(bool /*is_pub*/) {
    expect(TOK_STRUCT);
    Token name_tok = expect(TOK_IDENT);
    std::string name(name_tok.start, name_tok.length);
    expect(TOK_LBRACE);
    std::vector<StructField> fields;
    while (!check(TOK_RBRACE) && !check(TOK_EOF)) {
        Token fn = expect(TOK_IDENT);
        expect(TOK_COLON);
        TypePtr ft = parse_type();
        fields.push_back({std::string(fn.start, fn.length), ft});
        match(TOK_COMMA);
    }
    expect(TOK_RBRACE);
    auto s = std::make_shared<Stmt>();
    s->line = name_tok.line; s->col = name_tok.col;
    s->data = StructDeclStmt{name, fields};
    return s;
}

StmtPtr Parser::parse_mod_decl() {
    expect(TOK_MOD);
    Token name_tok = expect(TOK_IDENT);
    std::string name(name_tok.start, name_tok.length);
    expect(TOK_LBRACE);
    std::vector<StmtPtr> items;
    while (!check(TOK_RBRACE) && !check(TOK_EOF)) items.push_back(parse_item());
    expect(TOK_RBRACE);
    auto s = std::make_shared<Stmt>();
    s->data = ModDeclStmt{name, items};
    return s;
}

StmtPtr Parser::parse_use_stmt() {
    expect(TOK_USE);
    std::vector<std::string> path;
    Token t = expect(TOK_IDENT);
    path.push_back(std::string(t.start, t.length));
    while (match(TOK_COLONCOLON)) {
        Token seg = expect(TOK_IDENT);
        path.push_back(std::string(seg.start, seg.length));
    }
    expect(TOK_SEMICOLON);
    auto s = std::make_shared<Stmt>();
    s->data = UseStmt{path};
    return s;
}

// ---- Statements ----
StmtPtr Parser::parse_stmt() {
    if (check(TOK_LET))    return parse_var_decl();
    if (check(TOK_RETURN)) return parse_return_stmt();
    if (check(TOK_IF))     return parse_if_stmt();
    if (check(TOK_WHILE))  return parse_while_stmt();
    if (check(TOK_FOR))    return parse_for_stmt();
    if (check(TOK_LBRACE)) return parse_block();

    // expression statement
    auto e = parse_expr();
    expect(TOK_SEMICOLON);
    auto s = std::make_shared<Stmt>();
    s->data = ExprStmt{e};
    return s;
}

StmtPtr Parser::parse_block() {
    expect(TOK_LBRACE);
    std::vector<StmtPtr> stmts;
    while (!check(TOK_RBRACE) && !check(TOK_EOF)) stmts.push_back(parse_stmt());
    expect(TOK_RBRACE);
    auto s = std::make_shared<Stmt>();
    s->data = BlockStmt{stmts};
    return s;
}

StmtPtr Parser::parse_var_decl() {
    expect(TOK_LET);
    bool is_mut = match(TOK_MUT);
    Token name_tok = expect(TOK_IDENT);
    std::string name(name_tok.start, name_tok.length);
    TypePtr ann = nullptr;
    if (match(TOK_COLON)) ann = parse_type();
    ExprPtr init = nullptr;
    if (match(TOK_EQ)) init = parse_expr();
    expect(TOK_SEMICOLON);
    auto s = std::make_shared<Stmt>();
    s->line = name_tok.line; s->col = name_tok.col;
    s->data = VarDeclStmt{name, is_mut, ann, init};
    return s;
}

StmtPtr Parser::parse_return_stmt() {
    Token t = consume(); // consume 'return'
    std::optional<ExprPtr> val;
    if (!check(TOK_SEMICOLON)) val = parse_expr();
    expect(TOK_SEMICOLON);
    auto s = std::make_shared<Stmt>();
    s->line = t.line; s->col = t.col;
    s->data = ReturnStmt{val};
    return s;
}

StmtPtr Parser::parse_if_stmt() {
    Token t = consume(); // 'if'
    ExprPtr cond = parse_expr();
    StmtPtr then_b = parse_block();
    std::optional<StmtPtr> else_b;
    if (match(TOK_ELSE)) {
        if (check(TOK_IF)) else_b = parse_if_stmt();
        else               else_b = parse_block();
    }
    auto s = std::make_shared<Stmt>();
    s->line = t.line; s->col = t.col;
    s->data = IfStmt{cond, then_b, else_b};
    return s;
}

StmtPtr Parser::parse_while_stmt() {
    Token t = consume(); // 'while'
    ExprPtr cond = parse_expr();
    StmtPtr body = parse_block();
    auto s = std::make_shared<Stmt>();
    s->line = t.line; s->col = t.col;
    s->data = WhileStmt{cond, body};
    return s;
}

StmtPtr Parser::parse_for_stmt() {
    Token t = consume(); // 'for'
    Token var_tok = expect(TOK_IDENT);
    std::string var(var_tok.start, var_tok.length);
    expect(TOK_IN);
    ExprPtr iter = parse_expr();
    StmtPtr body = parse_block();
    auto s = std::make_shared<Stmt>();
    s->line = t.line; s->col = t.col;
    s->data = ForStmt{var, iter, body};
    return s;
}

// ---- Types ----
TypePtr Parser::parse_type() {
    auto t = std::make_shared<TypeNode>();
    if (match(TOK_STAR)) {
        t->kind  = TypeNode::Kind::Pointer;
        t->inner = parse_type();
        return t;
    }
    if (match(TOK_LBRACKET)) {
        t->kind  = TypeNode::Kind::Array;
        t->inner = parse_type();
        t->array_size = -1;
        if (match(TOK_SEMICOLON)) {
            Token sz = expect(TOK_INT_LIT);
            t->array_size = std::stoi(std::string(sz.start, sz.length));
        }
        expect(TOK_RBRACKET);
        return t;
    }
    t->kind = TypeNode::Kind::Primitive;
    Token tok = consume();
    switch (tok.type) {
        case TOK_TYPE_I8:   t->prim = PrimType::I8;   break;
        case TOK_TYPE_I16:  t->prim = PrimType::I16;  break;
        case TOK_TYPE_I32:  t->prim = PrimType::I32;  break;
        case TOK_TYPE_I64:  t->prim = PrimType::I64;  break;
        case TOK_TYPE_U8:   t->prim = PrimType::U8;   break;
        case TOK_TYPE_U16:  t->prim = PrimType::U16;  break;
        case TOK_TYPE_U32:  t->prim = PrimType::U32;  break;
        case TOK_TYPE_U64:  t->prim = PrimType::U64;  break;
        case TOK_TYPE_F32:  t->prim = PrimType::F32;  break;
        case TOK_TYPE_F64:  t->prim = PrimType::F64;  break;
        case TOK_TYPE_BOOL: t->prim = PrimType::Bool; break;
        case TOK_TYPE_STR:  t->prim = PrimType::Str;  break;
        case TOK_TYPE_VOID: t->prim = PrimType::Void; break;
        case TOK_IDENT:
            t->kind = TypeNode::Kind::Named;
            t->name = std::string(tok.start, tok.length);
            break;
        default:
            error("expected type");
    }
    return t;
}

// ---- Pratt expression parser ----
static std::string tok_str(Token t) { return std::string(t.start, t.length); }

int Parser::get_infix_prec(TokenType t) {
    switch (t) {
        case TOK_EQ: case TOK_PLUS_EQ: case TOK_MINUS_EQ:
        case TOK_STAR_EQ: case TOK_SLASH_EQ: return 1;
        case TOK_OR:    return 2;
        case TOK_AND:   return 3;
        case TOK_PIPE:  return 4;
        case TOK_CARET: return 5;
        case TOK_AMP:   return 6;
        case TOK_EQEQ: case TOK_NEQ: return 7;
        case TOK_LT: case TOK_GT: case TOK_LTE: case TOK_GTE: return 8;
        case TOK_LSHIFT: case TOK_RSHIFT: return 9;
        case TOK_PLUS: case TOK_MINUS: return 10;
        case TOK_STAR: case TOK_SLASH: case TOK_PERCENT: return 11;
        case TOK_AS:    return 12;
        case TOK_DOT: case TOK_LBRACKET: case TOK_LPAREN: return 14;
        default: return -1;
    }
}

ExprPtr Parser::parse_expr(int min_prec) {
    ExprPtr lhs = parse_unary();
    return parse_infix(lhs, min_prec);
}

ExprPtr Parser::parse_infix(ExprPtr lhs, int min_prec) {
    while (true) {
        int prec = get_infix_prec(cur().type);
        if (prec < min_prec) break;

        Token op_tok = consume();

        if (op_tok.type == TOK_AS) {
            TypePtr target = parse_type();
            auto e = std::make_shared<Expr>();
            e->data = CastExpr{lhs, target};
            lhs = e;
            continue;
        }
        if (op_tok.type == TOK_DOT) {
            Token field_tok = expect(TOK_IDENT);
            auto e = std::make_shared<Expr>();
            e->data = FieldExpr{lhs, std::string(field_tok.start, field_tok.length)};
            lhs = e;
            continue;
        }
        if (op_tok.type == TOK_LBRACKET) {
            ExprPtr idx = parse_expr();
            expect(TOK_RBRACKET);
            auto e = std::make_shared<Expr>();
            e->data = IndexExpr{lhs, idx};
            lhs = e;
            continue;
        }
        if (op_tok.type == TOK_LPAREN) {
            std::vector<ExprPtr> args;
            while (!check(TOK_RPAREN) && !check(TOK_EOF)) {
                args.push_back(parse_expr());
                if (!match(TOK_COMMA)) break;
            }
            expect(TOK_RPAREN);
            auto e = std::make_shared<Expr>();
            e->data = CallExpr{lhs, args};
            lhs = e;
            continue;
        }
        // assignment operators
        if (op_tok.type == TOK_EQ || op_tok.type == TOK_PLUS_EQ ||
            op_tok.type == TOK_MINUS_EQ || op_tok.type == TOK_STAR_EQ ||
            op_tok.type == TOK_SLASH_EQ) {
            ExprPtr rhs = parse_expr(prec); // right-assoc
            auto e = std::make_shared<Expr>();
            e->data = AssignExpr{lhs, rhs};
            lhs = e;
            continue;
        }
        // binary op
        ExprPtr rhs = parse_expr(prec + 1);
        auto e = std::make_shared<Expr>();
        e->data = BinopExpr{tok_str(op_tok), lhs, rhs};
        lhs = e;
    }
    return lhs;
}

ExprPtr Parser::parse_unary() {
    if (check(TOK_BANG) || check(TOK_MINUS) || check(TOK_TILDE) || check(TOK_AMP) || check(TOK_STAR)) {
        Token op = consume();
        ExprPtr operand = parse_unary();
        auto e = std::make_shared<Expr>();
        e->data = UnopExpr{tok_str(op), operand};
        return e;
    }
    if (match(TOK_AWAIT)) {
        ExprPtr inner = parse_unary();
        auto e = std::make_shared<Expr>();
        e->data = AwaitExpr{inner};
        return e;
    }
    return parse_postfix(parse_primary());
}

ExprPtr Parser::parse_postfix(ExprPtr base) {
    // postfix handled in parse_infix via prec 14
    return base;
}

ExprPtr Parser::parse_primary() {
    Token t = cur();
    auto e = std::make_shared<Expr>();
    e->line = t.line; e->col = t.col;

    if (t.type == TOK_INT_LIT) {
        consume();
        e->data = IntLitExpr{std::stoll(std::string(t.start, t.length))};
        return e;
    }
    if (t.type == TOK_FLOAT_LIT) {
        consume();
        e->data = FloatLitExpr{std::stod(std::string(t.start, t.length))};
        return e;
    }
    if (t.type == TOK_STRING_LIT) {
        consume();
        // strip quotes
        std::string raw(t.start + 1, t.length - 2);
        e->data = StrLitExpr{raw};
        return e;
    }
    if (t.type == TOK_TRUE)  { consume(); e->data = BoolLitExpr{true};  return e; }
    if (t.type == TOK_FALSE) { consume(); e->data = BoolLitExpr{false}; return e; }
    if (t.type == TOK_NULL)  { consume(); e->data = NullExpr{};         return e; }

    if (t.type == TOK_IDENT) {
        consume();
        std::string name(t.start, t.length);
        // handle path expressions: mod::fn
        while (check(TOK_COLONCOLON)) {
            consume();
            Token seg = expect(TOK_IDENT);
            name += "::" + std::string(seg.start, seg.length);
        }
        e->data = IdentExpr{name};
        return e;
    }
    if (t.type == TOK_LPAREN) {
        consume();
        ExprPtr inner = parse_expr();
        expect(TOK_RPAREN);
        return inner;
    }
    std::ostringstream ss;
    ss << "unexpected token '" << std::string(t.start, t.length)
       << "' at " << t.line << ":" << t.col;
    error(ss.str());
    return nullptr;
}

} // namespace astra
