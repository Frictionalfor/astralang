#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <stdexcept>

namespace astra {

/**
 * Module resolver — handles import/export, file mapping, and
 * circular dependency detection.
 *
 * Resolution rules:
 *   use std::math;          -> stdlib/std_math.as
 *   use std::io;            -> stdlib/std_io.as
 *   use mymod;              -> ./mymod.as  (relative to source file)
 *   use pkg::submod;        -> ./pkg/submod.as
 *
 * Circular dependency prevention:
 *   A depth-first traversal builds the dependency graph.
 *   If a module is encountered while it is already on the current
 *   resolution stack, a circular dependency error is raised.
 *
 * Load order:
 *   Modules are returned in topological order (dependencies first).
 */

struct ModuleNode {
    std::string              name;       // qualified name e.g. "std::math"
    std::string              file_path;  // resolved file path
    std::vector<std::string> imports;    // names of modules this one imports
    bool                     is_stdlib;
};

class ModuleResolver {
public:
    explicit ModuleResolver(const std::string &stdlib_dir,
                            const std::string &source_dir);

    /**
     * Resolve a module name to a file path.
     * Throws std::runtime_error if not found.
     */
    std::string resolve_path(const std::string &module_name,
                             const std::string &from_file) const;

    /**
     * Build the full dependency graph starting from entry_file.
     * Returns modules in topological load order.
     * Throws std::runtime_error on circular dependency.
     */
    std::vector<ModuleNode> build_load_order(const std::string &entry_file);

private:
    std::string stdlib_dir_;
    std::string source_dir_;

    std::unordered_map<std::string, ModuleNode> graph_;
    std::unordered_set<std::string>             visited_;
    std::unordered_set<std::string>             on_stack_;
    std::vector<ModuleNode>                     order_;

    void resolve_node(const std::string &name, const std::string &from_file);
    void dfs(const std::string &name);
    std::string name_to_path(const std::string &name,
                             const std::string &from_dir) const;
};

} // namespace astra
