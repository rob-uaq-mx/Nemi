// Exception hierarchy. Mirrors nemi/errors.py:
//   NemiError (base) -> LexicalError / ParseError / ExecutionError.
// Each carries an optional SourceLocation.
#ifndef NEMI_ERRORS_HPP
#define NEMI_ERRORS_HPP

#include <stdexcept>
#include <string>

#include "nemi/source_location.hpp"

namespace nemi {

class NemiError : public std::runtime_error {
public:
    explicit NemiError(const std::string& message,
                       SourceLocation location = {})
        : std::runtime_error(format(message, location)),
          message_(message),
          location_(location) {}

    const std::string& message() const { return message_; }
    const SourceLocation& location() const { return location_; }

private:
    static std::string format(const std::string& message,
                              const SourceLocation& loc) {
        if (loc.line != 0) return message + " (" + loc.to_string() + ")";
        return message;
    }

    std::string message_;
    SourceLocation location_;
};

// Malformed tokens.
class LexicalError : public NemiError {
    using NemiError::NemiError;
};

// Grammar violations.
class ParseError : public NemiError {
    using NemiError::NemiError;
};

// Runtime faults: type errors, division by zero, out-of-range indices, ...
class ExecutionError : public NemiError {
    using NemiError::NemiError;
};

}  // namespace nemi

#endif  // NEMI_ERRORS_HPP
