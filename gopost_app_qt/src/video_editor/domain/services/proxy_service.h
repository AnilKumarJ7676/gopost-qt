#pragma once

#include <QString>
#include <functional>
#include <optional>
#include <cstdint>

namespace gopost::video_editor {

/// DIP: Abstraction for proxy generation so callers don't depend on the
/// concrete FFmpeg-backed implementation.
class ProxyService {
public:
    virtual ~ProxyService() = default;

    virtual std::optional<QString> existingProxyPath(const QString& sourcePath) = 0;

    virtual std::optional<QString> generateProxy(
        const QString& sourcePath,
        std::function<void(double progress)> onProgress = nullptr
    ) = 0;

    virtual void cancelAll() = 0;

    virtual void clearProxyForSource(const QString& sourcePath) = 0;

    virtual int64_t clearAllProxies() = 0;

    virtual int64_t getCacheSize() = 0;

    virtual bool verifyProxy(const QString& proxyPath) = 0;
};

} // namespace gopost::video_editor
