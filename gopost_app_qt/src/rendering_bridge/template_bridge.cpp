#include "rendering_bridge/template_bridge.h"

namespace gopost::rendering {

TemplateBridge::TemplateBridge(GopostEngine& engine) : engine_(engine) {}

TemplateMetadata TemplateBridge::loadEncryptedTemplate(
    const QByteArray& encryptedBlob, const QByteArray& sessionKey) {
    if (!engine_.isInitialized()) {
        engine_.initialize(EngineConfig{});
    }
    return engine_.loadTemplate(encryptedBlob, sessionKey);
}

void TemplateBridge::unloadTemplate(const QString& templateId) {
    if (!engine_.isInitialized()) return;
    engine_.unloadTemplate(templateId);
}

bool TemplateBridge::isReady() const { return engine_.isInitialized(); }

} // namespace gopost::rendering
