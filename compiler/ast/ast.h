#pragma once
#include <string>
#include <vector>
#include <memory>
#include <variant>
#include <optional>

namespace astra {

// ---- Type representation ----
enum class PrimType { I8, I16, I32, I64, U8, U16, U32, U64, F32, F64, Bool, Str, Void };

struct TypeNode {
    enum class Kind { Primitive, Pointer, Array, Named, Fn } kind;
    PrimType                    prim;       // if Primitive
    std::shared_ptr<TypeNode>   inner;      // if Pointer/Array
    std::string                 name;       // if Named
    std::vector<std::shared_ptr<TypeNode>> params; // if Fn
    std::shared_ptr<TypeNode>   ret;        // if Fn
    int                         array_size; // if Array, -1 = dynamic
};

using TypePtr = std::shared_ptr<TypeNode>;

// ---- Forward declarations ----
struct Expr;
struct Stmt;
using ExprPtr = std::shared_ptr<Expr>;
using StmtPtr = std::shared_ptr<Stmt>;

// ---- Expressions ----
struct IntLitExpr    { int64_t value; };
struct FloatLitExpr  { double  value; };
struct BoolLitExpr   { bool    value; };
struct StrLitExpr    { std::string value; };
struct NullExpr      {};
struct IdentExpr     { std::string name; };

struct BinopExpr {
    std::string op;
    ExprPtr     lhs, rhs;
};

struct UnopExpr {
    std::string op;
    ExprPtr     operand;
};

struct CallExpr {
    ExprPtr                  callee;
    std::vector<ExprPtr>     args;
};

struct IndexExpr {
    ExprPtr base, index;
};

struct FieldExpr {
    ExprPtr     base;
    std::string field;
};

struct CastExpr {
    ExprPtr operand;
    TypePtr target;
};

struct AssignExpr {
    ExprPtr lhs, rhs;
};

struct AwaitExpr {
    ExprPtr inner;
};

struct Expr {
    int line = 0, col = 0;
    TypePtr resolved_type; // filled by sema
    std::variant<
        IntLitExpr, FloatLitExpr, BoolLitExpr, StrLitExpr, NullExpr,
        IdentExpr, BinopExpr, UnopExpr, CallExpr, IndexExpr,
        FieldExpr, CastExpr, AssignExpr, AwaitExpr
    > data;
};

// ---- Statements ----
struct VarDeclStmt {
    std::string name;
    bool        is_mut;
    TypePtr     type_ann; // optional annotation
    ExprPtr     init;
};

struct ReturnStmt  { std::optional<ExprPtr> value; };
struct ExprStmt    { ExprPtr expr; };

struct BlockStmt   { std::vector<StmtPtr> stmts; };

struct IfStmt {
    ExprPtr  cond;
    StmtPtr  then_block;
    std::optional<StmtPtr> else_block;
};

struct WhileStmt {
    ExprPtr cond;
    StmtPtr body;
};

struct ForStmt {
    std::string var;
    ExprPtr     iter;
    StmtPtr     body;
};

struct StructField {
    std::string name;
    TypePtr     type;
};

struct StructDeclStmt {
    std::string              name;
    std::vector<StructField> fields;
};

struct FnParam {
    std::string name;
    TypePtr     type;
};

struct FnDeclStmt {
    std::string           name;
    std::vector<FnParam>  params;
    TypePtr               ret_type;
    StmtPtr               body;
    bool                  is_pub;
    bool                  is_async;
    bool                  is_extern;
};

struct ModDeclStmt {
    std::string          name;
    std::vector<StmtPtr> items;
};

struct UseStmt {
    std::vector<std::string> path; // e.g. ["std", "io"]
};

struct Stmt {
    int line = 0, col = 0;
    std::variant<
        VarDeclStmt, ReturnStmt, ExprStmt, BlockStmt,
        IfStmt, WhileStmt, ForStmt,
        StructDeclStmt, FnDeclStmt, ModDeclStmt, UseStmt
    > data;
};

// ---- Top-level module ----
struct AstraModule {
    std::string          name;
    std::vector<StmtPtr> items;
};

} // namespace astra
