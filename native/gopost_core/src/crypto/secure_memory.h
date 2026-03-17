#ifndef GOPOST_CRYPTO_SECURE_MEMORY_H
#define GOPOST_CRYPTO_SECURE_MEMORY_H

#include <cstddef>

namespace gopost {
namespace crypto {

void secure_zero(void* ptr, size_t len);
bool lock_memory(void* ptr, size_t len);
bool unlock_memory(void* ptr, size_t len);

}  // namespace crypto
}  // namespace gopost

#endif
