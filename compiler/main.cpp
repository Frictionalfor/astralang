/**
 * astrac — AstraLang unified compiler CLI
 *
 * Commands:
 *   astrac build  <src.as> [-o out.asc] [--debug] [--no-opt]
 *   astrac run    <src.as> [--trace] [--stack-limit N]
 *   astrac check  <src.as>
 *   astrac disasm <file.asc>
 *   astrac version
 */
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <cstring>
#include <cstdio>
#include <cstdlib>

extern "C" {
#include "../include/astra_token.h"
#include "../include/astra_ir.h"
#include "../include/astra_error.h"
}

#include "parser/parser.h"
#include "sema/sema.h"
#include "opt/optimizer.h"
#include "ir/codegen.h"

#define ASTRAC_VERSION "1.2.0"

// ---- file I/O ----
static std::string read_file(const char *path) {
    std::ifstream f(path);
    if (!f) {
        AstraDiagnostic d{};
        d.code     = ASTRA_ERR_FILE_NOT_FOUND;
        d.severity = ASTRA_SEVERITY_ERROR;
        d.file     = path;
        d.line = 0; d.col = 0;
        d.message  = "cannot open source file";
        astra_print_diagnostic(&d);
        exit(1);
    }
    std::ostringstream ss; ss << f.rdbuf();
    return ss.str();
}

static std::vector<Token> lex_all(const std::string &src, const char *path) {
    Lexer *l = lexer_create(src.c_str(), (int)src.size());
    std::vector<Token> toks;
    bool had_error = false;
    while (true) {
        Token t = lexer_next(l);
        if (t.type == TOK_ERROR) {
            AstraDiagnostic d{};
            d.code     = ASTRA_ERR_UNEXPECTED_CHAR;
            d.severity = ASTRA_SEVERITY_ERROR;
            d.file     = path;
            d.line     = t.line; d.col = t.col;
            d.message  = "unexpected character";
            astra_print_diagnostic(&d);
            had_error = true;
        }
        toks.push_back(t);
        if (t.type == TOK_EOF) break;
    }
    lexer_destroy(l);
    if (had_error) exit(1);
    return toks;
}

// ---- write .asc module ----
static void write_module(const IRModule &mod, const char *out_path, bool debug_flag, bool opt_flag) {
    FILE *f = fopen(out_path, "wb");
    if (!f) {
        fprintf(stderr, "cannot write: %s\n", out_path);
        exit(1);
    }

    uint32_t flags = ASTRA_FLAG_NONE;
    if (debug_flag) flags |= ASTRA_FLAG_DEBUG;
    if (opt_flag)   flags |= ASTRA_FLAG_OPTIMIZED;

    // header
    fwrite("ASTR", 1, 4, f);
    uint32_t ver = ASTRA_BYTECODE_VERSION;
    fwrite(&ver,   4, 1, f);
    fwrite(&flags, 4, 1, f);

    // string pool
    fwrite(&mod.string_count, 4, 1, f);
    for (int i = 0; i < mod.string_count; i++) {
        int32_t len = (int32_t)strlen(mod.string_pool[i]);
        fwrite(&len, 4, 1, f);
        fwrite(mod.string_pool[i], 1, len, f);
    }

    // functions
    fwrite(&mod.func_count, 4, 1, f);
    for (int i = 0; i < mod.func_count; i++) {
        const IRFunction &fn = mod.functions[i];
        fwrite(fn.name,         1, 128, f);
        fwrite(&fn.param_count, 4, 1,   f);
        fwrite(&fn.local_count, 4, 1,   f);
        fwrite(&fn.instr_count, 4, 1,   f);
        fwrite(fn.instrs, sizeof(IRInstr), fn.instr_count, f);
    }

    fwrite(&mod.entry_func_index, 4, 1, f);
    fclose(f);
}

// ---- compile pipeline ----
static IRModule compile(const char *src_path, bool enable_opt, bool debug_mode) {
    std::string src = read_file(src_path);
    std::vector<Token> tokens = lex_all(src, src_path);

    astra::Parser parser(tokens);
    astra::AstraModule mod;
    try {
        mod = parser.parse_module("main");
    } catch (const std::exception &e) {
        AstraDiagnostic d{};
        d.code = ASTRA_ERR_UNEXPECTED_TOKEN;
        d.severity = ASTRA_SEVERITY_ERROR;
        d.file = src_path; d.line = 0; d.col = 0;
        d.message = e.what();
        astra_print_diagnostic(&d);
        exit(1);
    }

    astra::SemanticAnalyzer sema;
    try {
        sema.analyze(mod);
    } catch (const std::exception &e) {
        AstraDiagnostic d{};
        d.code = ASTRA_ERR_UNDEFINED_SYMBOL;
        d.severity = ASTRA_SEVERITY_ERROR;
        d.file = src_path; d.line = 0; d.col = 0;
        d.message = e.what();
        astra_print_diagnostic(&d);
        exit(1);
    }

    if (enable_opt) {
        astra::Optimizer opt;
        opt.run(mod);
    }

    astra::IRCodegen cg;
    return cg.generate(mod);
}

// ---- commands ----
static void cmd_version() {
    printf("astrac %s  (bytecode v%d.%d)\n",
           ASTRAC_VERSION,
           ASTRA_BYTECODE_MAJOR,
           ASTRA_BYTECODE_MINOR);
}

static void cmd_build(int argc, char **argv) {
    const char *src_path = nullptr;
    const char *out_path = nullptr;
    bool debug_mode = false;
    bool no_opt     = false;

    for (int i = 0; i < argc; i++) {
        if (strcmp(argv[i], "--debug") == 0)    debug_mode = true;
        else if (strcmp(argv[i], "--no-opt") == 0) no_opt = true;
        else if (strcmp(argv[i], "-o") == 0 && i+1 < argc) out_path = argv[++i];
        else if (argv[i][0] != '-') src_path = argv[i];
    }
    if (!src_path) { fprintf(stderr, "usage: astrac build <src.as> [-o out.asc] [--debug] [--no-opt]\n"); exit(1); }

    // default output name: replace .as with .asc
    std::string auto_out;
    if (!out_path) {
        auto_out = src_path;
        auto pos = auto_out.rfind(".as");
        if (pos != std::string::npos) auto_out.replace(pos, 3, ".asc");
        else auto_out += ".asc";
        out_path = auto_out.c_str();
    }

    IRModule ir = compile(src_path, !no_opt, debug_mode);
    write_module(ir, out_path, debug_mode, !no_opt);
    printf("[astrac] built %s → %s\n", src_path, out_path);
}

static void cmd_check(int argc, char **argv) {
    if (argc < 1) { fprintf(stderr, "usage: astrac check <src.as>\n"); exit(1); }
    const char *src_path = argv[0];
    compile(src_path, false, false); // compile but discard output
    printf("[astrac] %s — OK\n", src_path);
}

static void cmd_run(int argc, char **argv) {
    // build to a temp file then exec runtime
    const char *src_path = nullptr;
    bool trace = false;
    int  stack_limit = 8192;

    for (int i = 0; i < argc; i++) {
        if (strcmp(argv[i], "--trace") == 0) trace = true;
        else if (strcmp(argv[i], "--stack-limit") == 0 && i+1 < argc)
            stack_limit = atoi(argv[++i]);
        else if (argv[i][0] != '-') src_path = argv[i];
    }
    if (!src_path) { fprintf(stderr, "usage: astrac run <src.as> [--trace]\n"); exit(1); }

    IRModule ir = compile(src_path, true, false);
    write_module(ir, "/tmp/astra_run.asc", false, true);

    // find runtime binary relative to this executable
    std::string rt = std::string(getenv("HOME")) + "/.local/bin/astra";
    // fallback: look next to astrac
    std::string cmd = rt + " /tmp/astra_run.asc";
    if (trace) cmd += " --trace";
    cmd += " --stack-limit " + std::to_string(stack_limit);

    // try local build first
    FILE *test = fopen("runtime/target/release/astra", "rb");
    if (test) { fclose(test); cmd = "runtime/target/release/astra /tmp/astra_run.asc"; }
    if (trace) cmd += " --trace";
    cmd += " --stack-limit " + std::to_string(stack_limit);

    system(cmd.c_str());
}

static void cmd_disasm(int argc, char **argv) {
    if (argc < 1) { fprintf(stderr, "usage: astrac disasm <file.asc>\n"); exit(1); }
    // delegate to astradis if available, else inline
    std::string cmd = std::string("build/astradis ") + argv[0];
    if (system(cmd.c_str()) != 0) {
        fprintf(stderr, "astradis not found — run: make disasm\n");
    }
}

static void usage() {
    printf("astrac %s — AstraLang compiler\n\n", ASTRAC_VERSION);
    printf("Commands:\n");
    printf("  build  <src.as> [-o out.asc] [--debug] [--no-opt]\n");
    printf("  run    <src.as> [--trace] [--stack-limit N]\n");
    printf("  check  <src.as>\n");
    printf("  disasm <file.asc>\n");
    printf("  version\n");
}

int main(int argc, char **argv) {
    if (argc < 2) { usage(); return 1; }

    const char *cmd = argv[1];
    int sub_argc = argc - 2;
    char **sub_argv = argv + 2;

    if (strcmp(cmd, "build")   == 0) { cmd_build(sub_argc, sub_argv);   return 0; }
    if (strcmp(cmd, "run")     == 0) { cmd_run(sub_argc, sub_argv);      return 0; }
    if (strcmp(cmd, "check")   == 0) { cmd_check(sub_argc, sub_argv);    return 0; }
    if (strcmp(cmd, "disasm")  == 0) { cmd_disasm(sub_argc, sub_argv);   return 0; }
    if (strcmp(cmd, "version") == 0) { cmd_version();                    return 0; }

    // legacy: if first arg is a .as file, treat as build
    if (strstr(argv[1], ".as") && argc >= 2) {
        // backward compat: astrac <src.as> [-o out.asc]
        cmd_build(argc - 1, argv + 1);
        return 0;
    }

    fprintf(stderr, "unknown command: %s\n", cmd);
    usage();
    return 1;
}
