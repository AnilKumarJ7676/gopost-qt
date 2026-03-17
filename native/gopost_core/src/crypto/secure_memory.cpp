#include "crypto/secure_memory.h"
#include <cstddef>
#include <cstdint>
#include <cstring>

#ifdef _WIN32
#include <windows.h>
#elif defined(__APPLE__) || defined(__linux__) || defined(__ANDROID__)
#include <sys/mman.h>
#endif

namespace gopost {
namespace crypto {

void secure_zero(void* ptr, size_t len) {
    if (!ptr || len == 0) return;

#if defined(_WIN32)
    SecureZeroMemory(ptr, len);
#elif defined(__linux__) || defined(__ANDROID__)
    explicit_bzero(ptr, len);
#else
    volatile uint8_t* p = static_cast<volatile uint8_t*>(ptr);
    for (size_t i = 0; i < len; ++i) p[i] = 0;
#endif
}

bool lock_memory(void* ptr, size_t len) {
#if defined(__APPLE__) || defined(__linux__) || defined(__ANDROID__)
    return mlock(ptr, len) == 0;
#elif defined(_WIN32)
    return VirtualLock(ptr, len) != 0;
#else
    (void)ptr;
    (void)len;
    return true;
#endif
}

bool unlock_memory(void* ptr, size_t len) {
#if defined(__APPLE__) || defined(__linux__) || defined(__ANDROID__)
    return munlock(ptr, len) == 0;
#elif defined(_WIN32)
    return VirtualUnlock(ptr, len) != 0;
#else
    (void)ptr;
    (void)len;
    return true;
#endif
}

}  // namespace crypto
}  // namespace gopost
