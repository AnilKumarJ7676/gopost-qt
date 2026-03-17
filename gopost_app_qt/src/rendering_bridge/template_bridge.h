#pragma once

#include "rendering_bridge/engine_api.h"

namespace gopost::rendering {

/// SRP: Orchestrates the template download -> decrypt -> load lifecycle.
/// DIP: Depends on abstract GopostEngine, not FFI implementation.
class TemplateBridge {
public:
    explicit TemplateBridge(GopostEngine& engine);
    ~TemplateBridge() = default;

    TemplateMetadata loadEncryptedTemplate(const QByteArray& encryptedBlob,
                                           const QByteArray& sessionKey);

    void unloadTemplate(const QString& templateId);

    [[nodiscard]] bool isReady() const;

private:
    GopostEngine& engine_;
};

} // namespace gopost::rendering
