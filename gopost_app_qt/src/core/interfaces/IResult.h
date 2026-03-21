#pragma once

#include <optional>
#include <string>

namespace gopost {

template<typename T>
struct Result {
    std::optional<T> value;
    std::string error;
    int errorCode = 0;

    [[nodiscard]] bool ok() const { return value.has_value(); }
    [[nodiscard]] const T& get() const { return value.value(); }
    [[nodiscard]] T& get() { return value.value(); }

    static Result success(T val) { return {std::move(val), "", 0}; }
    static Result failure(std::string err, int code = -1) { return {std::nullopt, std::move(err), code}; }
};

// Specialization for void operations
template<>
struct Result<void> {
    bool succeeded = false;
    std::string error;
    int errorCode = 0;

    [[nodiscard]] bool ok() const { return succeeded; }

    static Result success() { return {true, "", 0}; }
    static Result failure(std::string err, int code = -1) { return {false, std::move(err), code}; }
};

} // namespace gopost
