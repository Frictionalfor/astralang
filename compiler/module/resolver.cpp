#include "resolver.h"
#include <fstream>
#include <sstream>
#include <algorithm>

namespace astra {

ModuleResolver::ModuleResolver(const std::string &stdlib_dir,
                               const std::string &source_dir)
    : stdlib_dir_(stdlib_dir), source_dir_(source_dir) {}

std::string ModuleResolver::name_to_path(const std::string &name,
                                          const std::string &from_dir) const {
    if (name.rfind("std::", 0) == 0) {
        std::string mod = name.substr(5);
        return stdlib_dir_ + "/std_" + mod + ".as";
    }
    // replace :: with /
    std::string rel;
    for (size_t i = 0; i < name.size(); i++) {
        if (i + 1 < name.size() && name[i] == ':' && name[i+1] == ':') {
            rel += '/'; i++; // skip second colon
        } else {
            rel += name[i];
        }
    }
    return from_dir + "/" + rel + ".as";
}

std::string ModuleResolver::resolve_path(const std::string &module_name,
                                          const std::string &from_file) const {
    std::string from_dir = source_dir_;
    auto slash = from_file.rfind('/');
    if (slash != std::string::npos) from_dir = from_file.substr(0, slash);

    std::string path = name_to_path(module_name, from_dir);
    std::ifstream f(path);
    if (!f) {
        throw std::runtime_error("module not found: '" + module_name +
                                 "' (looked at: " + path + ")");
    }
    return path;
}

// Scan a source file for `use X;` statements.
// Skips line comments (//), block comments (/* */), and string literals.
static std::vector<std::string> scan_imports(const std::string &path) {
    std::vector<std::string> imports;
    std::ifstream f(path);
    if (!f) return imports;

    std::string src((std::istreambuf_iterator<char>(f)),
                     std::istreambuf_iterator<char>());

    size_t i = 0;
    size_t n = src.size();

    while (i < n) {
        // skip line comment
        if (i + 1 < n && src[i] == '/' && src[i+1] == '/') {
            while (i < n && src[i] != '\n') i++;
            continue;
        }
        // skip block comment
        if (i + 1 < n && src[i] == '/' && src[i+1] == '*') {
            i += 2;
            while (i + 1 < n && !(src[i] == '*' && src[i+1] == '/')) i++;
            i += 2;
            continue;
        }
        // skip string literal
        if (src[i] == '"') {
            i++;
            while (i < n && src[i] != '"') {
                if (src[i] == '\\') i++; // skip escape
                i++;
            }
            if (i < n) i++; // closing quote
            continue;
        }
        // match `use ` keyword (must be at word boundary)
        if (i + 4 <= n && src.substr(i, 4) == "use " &&
            (i == 0 || src[i-1] == '\n' || src[i-1] == ' ' || src[i-1] == '\t' || src[i-1] == ';' || src[i-1] == '}')) {
            i += 4;
            // skip whitespace
            while (i < n && (src[i] == ' ' || src[i] == '\t')) i++;
            // read until semicolon or newline
            size_t start = i;
            while (i < n && src[i] != ';' && src[i] != '\n') i++;
            std::string name = src.substr(start, i - start);
            // trim trailing whitespace
            while (!name.empty() && (name.back() == ' ' || name.back() == '\t' || name.back() == '\r'))
                name.pop_back();
            if (!name.empty()) imports.push_back(name);
            continue;
        }
        i++;
    }
    return imports;
}

// Non-recursive DFS node resolution — uses an explicit work stack
void ModuleResolver::resolve_node(const std::string &mod_name,
                                   const std::string &from_file) {
    // Use an explicit stack to avoid unbounded recursion
    struct Work { std::string name; std::string from; };
    std::vector<Work> stack;
    stack.push_back({mod_name, from_file});

    while (!stack.empty()) {
        Work w = stack.back();
        stack.pop_back();

        if (graph_.count(w.name)) continue; // already resolved

        std::string path;
        try {
            path = resolve_path(w.name, w.from);
        } catch (...) {
            continue; // missing module — soft error, skip
        }

        ModuleNode node;
        node.name      = w.name;
        node.file_path = path;
        node.imports   = scan_imports(path);
        node.is_stdlib = (w.name.rfind("std::", 0) == 0);
        graph_[w.name] = node;

        // push dependencies onto work stack
        for (auto &imp : node.imports) {
            if (!graph_.count(imp)) {
                stack.push_back({imp, path});
            }
        }
    }
}

std::vector<ModuleNode> ModuleResolver::build_load_order(const std::string &entry_file) {
    ModuleNode entry;
    entry.name      = "__main__";
    entry.file_path = entry_file;
    entry.imports   = scan_imports(entry_file);
    entry.is_stdlib = false;
    graph_["__main__"] = entry;

    for (auto &imp : entry.imports) {
        resolve_node(imp, entry_file);
    }

    dfs("__main__");
    return order_;
}

void ModuleResolver::dfs(const std::string &name) {
    if (visited_.count(name)) return;
    if (on_stack_.count(name)) {
        throw std::runtime_error("circular dependency detected: " + name);
    }
    on_stack_.insert(name);
    auto it = graph_.find(name);
    if (it != graph_.end()) {
        for (auto &dep : it->second.imports) {
            if (graph_.count(dep)) dfs(dep);
        }
        visited_.insert(name);
        on_stack_.erase(name);
        order_.push_back(it->second);
    }
}

} // namespace astra
