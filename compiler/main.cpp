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
#include <sys/wait.h>
#include <unistd.h>

extern "C" {
#include "../include/astra_token.h"
#include "../include/astra_ir.h"
#include "../include/astra_error.h"
#include "../include/astra_version.h"
#include "../include/astra_opcode_table.h"
}

#include "parser/parser.h"
#include "sema/sema.h"
#include "opt/optimizer.h"
#include "opt/opt_level.h"
#include "ir/codegen.h"
#include "module/resolver.h"

#define ASTRAC_VERSION ASTRA_COMPILER_VERSION

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

static int run_program(const std::string &program, const std::vector<std::string> &args) {
    std::vector<char *> argv;
    argv.reserve(args.size() + 2);
    argv.push_back(const_cast<char *>(program.c_str()));
    for (const auto &arg : args) argv.push_back(const_cast<char *>(arg.c_str()));
    argv.push_back(nullptr);

    pid_t pid = fork();
    if (pid < 0) {
        perror("fork");
        return 127;
    }
    if (pid == 0) {
        execvp(program.c_str(), argv.data());
        perror(program.c_str());
        _exit(127);
    }

    int status = 0;
    if (waitpid(pid, &status, 0) < 0) {
        perror("waitpid");
        return 127;
    }
    if (WIFEXITED(status)) return WEXITSTATUS(status);
    if (WIFSIGNALED(status)) return 128 + WTERMSIG(status);
    return 127;
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
static IRModule compile(const char *src_path, astra::OptLevel opt_level, bool debug_mode) {
    (void)debug_mode;
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

    try {
        astra::Optimizer opt(opt_level);
        opt.run(mod);

        astra::IRCodegen cg;
        return cg.generate(mod);
    } catch (const std::exception &e) {
        AstraDiagnostic d{};
        d.code = ASTRA_ERR_CODEGEN;
        d.severity = ASTRA_SEVERITY_ERROR;
        d.file = src_path; d.line = 0; d.col = 0;
        d.message = e.what();
        astra_print_diagnostic(&d);
        exit(1);
    }
}

// ---- commands ----
static void cmd_version() {
    printf("astrac %s  (bytecode v%d.%d, stdlib v%s)\n",
           ASTRAC_VERSION,
           ASTRA_BYTECODE_MAJOR,
           ASTRA_BYTECODE_MINOR,
           ASTRA_STDLIB_VERSION);
}

static void cmd_build(int argc, char **argv) {
    const char *src_path = nullptr;
    const char *out_path = nullptr;
    bool debug_mode = false;
    bool no_opt     = false; (void)no_opt;
    astra::OptLevel opt_level = astra::OptLevel::O2;

    for (int i = 0; i < argc; i++) {
        if (strcmp(argv[i], "--debug") == 0)       debug_mode = true;
        else if (strcmp(argv[i], "--no-opt") == 0) { no_opt = true; opt_level = astra::OptLevel::O0; }
        else if (strcmp(argv[i], "-O0") == 0)      opt_level = astra::OptLevel::O0;
        else if (strcmp(argv[i], "-O1") == 0)      opt_level = astra::OptLevel::O1;
        else if (strcmp(argv[i], "-O2") == 0)      opt_level = astra::OptLevel::O2;
        else if (strcmp(argv[i], "-o") == 0 && i+1 < argc) out_path = argv[++i];
        else if (argv[i][0] != '-') src_path = argv[i];
    }
    if (!src_path) { fprintf(stderr, "usage: astrac build <src.as> [-o out.asc] [-O0|-O1|-O2] [--debug]\n"); exit(1); }

    std::string auto_out;
    if (!out_path) {
        auto_out = src_path;
        auto pos = auto_out.rfind(".as");
        if (pos != std::string::npos) auto_out.replace(pos, 3, ".asc");
        else auto_out += ".asc";
        out_path = auto_out.c_str();
    }

    IRModule ir = compile(src_path, opt_level, debug_mode);
    write_module(ir, out_path, debug_mode, opt_level >= astra::OptLevel::O1);
    printf("[astrac] built %s -> %s  (%s)\n", src_path, out_path,
           astra::opt_level_name(opt_level));
}

static void cmd_check(int argc, char **argv) {
    if (argc < 1) { fprintf(stderr, "usage: astrac check <src.as>\n"); exit(1); }
    const char *src_path = argv[0];
    compile(src_path, astra::OptLevel::O0, false);
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

    IRModule ir = compile(src_path, astra::OptLevel::O2, false);
    write_module(ir, "/tmp/astra_run.asc", false, true);

    std::string rt;
    FILE *test = fopen("runtime/target/release/astra", "rb");
    if (test) {
        fclose(test);
        rt = "runtime/target/release/astra";
    } else {
        const char *home = getenv("HOME");
        rt = home ? std::string(home) + "/.local/bin/astra" : "astra";
    }

    std::vector<std::string> rt_args = {"/tmp/astra_run.asc"};
    if (trace) rt_args.push_back("--trace");
    rt_args.push_back("--stack-limit");
    rt_args.push_back(std::to_string(stack_limit));

    int status = run_program(rt, rt_args);
    if (status != 0) exit(status);
}

static void cmd_disasm(int argc, char **argv) {
    if (argc < 1) { fprintf(stderr, "usage: astrac disasm <file.asc>\n"); exit(1); }
    int status = run_program("build/astradis", {argv[0]});
    if (status != 0) {
        fprintf(stderr, "astradis not found — run: make disasm\n");
        exit(status);
    }
}

static void usage() {
    printf("astrac %s — AstraLang compiler\n\n", ASTRAC_VERSION);
    printf("Commands:\n");
    printf("  build  <src.as> [-o out.asc] [-O0|-O1|-O2] [--debug]\n");
    printf("  run    <src.as> [--trace] [--stack-limit N]\n");
    printf("  check  <src.as>\n");
    printf("  disasm <file.asc>\n");
    printf("  opcodes\n");
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
    if (strcmp(cmd, "opcodes") == 0) { astra_opcode_table_print();       return 0; }

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
