// Environment: a single call frame — a flat name -> value map (spec §12).
// Mirrors nemi/interpreter.py's Environment. Nemi has no nested lexical scopes,
// so one frame per call suffices.
#ifndef NEMI_ENVIRONMENT_HPP
#define NEMI_ENVIRONMENT_HPP

#include <string>
#include <unordered_map>
#include <utility>

#include "nemi/errors.hpp"
#include "nemi/value.hpp"

namespace nemi {

class Environment {
public:
    void define(const std::string& name, Value value) {
        vars_[name] = std::move(value);
    }

    const Value& get(const std::string& name) const {
        auto it = vars_.find(name);
        if (it == vars_.end()) {
            throw ExecutionError("undefined variable: " + name);
        }
        return it->second;
    }

    bool has(const std::string& name) const {
        return vars_.find(name) != vars_.end();
    }

private:
    std::unordered_map<std::string, Value> vars_;
};

}  // namespace nemi

#endif  // NEMI_ENVIRONMENT_HPP
