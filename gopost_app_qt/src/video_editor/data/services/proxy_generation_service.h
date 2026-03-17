#pragma once

#include <QString>
#include <QHash>
#include <QDir>
#include <functional>
#include <optional>
#include <memory>
#include <cstdint>

#include "video_editor/domain/services/proxy_service.h"
#include "video_editor/data/services/ffmpeg_runner.h"

namespace gopost::video_editor {

class ProxyGenerationService : public ProxyService {
public:
    ProxyGenerationService();

    static ProxyGenerationService& instance();

    std::optional<QString> existingProxyPath(const QString& sourcePath) override;

    std::optional<QString> generateProxy(
        const QString& sourcePath,
        std::function<void(double progress)> onProgress = nullptr
    ) override;

    void cancelAll() override;
    void clearProxyForSource(const QString& sourcePath) override;
    int64_t clearAllProxies() override;
    int64_t getCacheSize() override;
    bool verifyProxy(const QString& proxyPath) override;

private:
    static constexpr int kProxyHeight = 720;
    static constexpr const char* kProxyCodec = "libx264";
    static constexpr const char* kProxyPreset = "fast";
    static constexpr int kProxyCrf = 23;
    static constexpr int kProxyAudioBitrate = 128;

    std::shared_ptr<FfmpegRunner> ffmpeg_;
    std::optional<QDir> proxyDir_;

    [[nodiscard]] QDir ensureProxyDir();
    [[nodiscard]] static QString proxyFileName(const QString& sourcePath);
    [[nodiscard]] static bool hasMoovAtom(const QString& filePath);
};

} // namespace gopost::video_editor
