#pragma once

#include "core/interfaces/IResult.h"
#include <cstdint>
#include <string>
#include <vector>

namespace gopost::io {

class HashCalculator {
public:
    [[nodiscard]] static gopost::Result<std::string> hashFile(const std::string& path);
    [[nodiscard]] static std::string hashBytes(const std::vector<uint8_t>& data);
    [[nodiscard]] static std::string hashBytes(const uint8_t* data, size_t length);

private:
    // Bundled SHA-256 — no OpenSSL dependency
    struct SHA256State {
        uint32_t state[8];
        uint64_t bitcount;
        uint8_t buffer[64];
    };

    static void sha256Init(SHA256State& ctx);
    static void sha256Update(SHA256State& ctx, const uint8_t* data, size_t len);
    static void sha256Final(SHA256State& ctx, uint8_t hash[32]);
    static void sha256Transform(SHA256State& ctx, const uint8_t data[64]);
    static std::string hexEncode(const uint8_t* data, size_t len);
};

} // namespace gopost::io
