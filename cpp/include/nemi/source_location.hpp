// SourceLocation: a 1-based (line, column) position in a source file.
// Mirrors nemi/errors.py's SourceLocation. Value type, no behaviour.
#ifndef NEMI_SOURCE_LOCATION_HPP
#define NEMI_SOURCE_LOCATION_HPP

#include <string>

namespace nemi {

struct SourceLocation {
    int line = 0;    // 0 == unknown
    int column = 0;
    std::string file;  // empty == unknown/in-memory source (e.g. --llama text)

    std::string to_string() const {
        if (line == 0) return "<unknown>";
        std::string prefix = file.empty() ? "" : file + ", ";
        if (column == 0) return prefix + std::to_string(line);
        return prefix + std::to_string(line) + ":" + std::to_string(column);
    }
};

}  // namespace nemi

#endif  // NEMI_SOURCE_LOCATION_HPP
