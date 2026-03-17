#include "video_editor/presentation/notifiers/proxy_provider.h"
#include "video_editor/data/services/proxy_generation_service.h"

#include <QDebug>
#include <QtConcurrent>

namespace gopost::video_editor {

ProxyProvider::ProxyProvider(QObject* parent) : QObject(parent) {}
ProxyProvider::~ProxyProvider() = default;

void ProxyProvider::generate(const QString& sourcePath, const QString& outputPath) {
    if (isGenerating_) return;
    isGenerating_ = true;
    emit stateChanged();

    Q_UNUSED(outputPath); // ProxyGenerationService manages output paths internally

    qDebug() << "[ProxyProvider] Generating proxy for:" << sourcePath;

    // Run proxy generation on background thread
    QtConcurrent::run([this, sourcePath]() {
        auto& service = ProxyGenerationService::instance();

        auto result = service.generateProxy(sourcePath,
            [](double progress) {
                Q_UNUSED(progress);
            });

        QMetaObject::invokeMethod(this, [this, sourcePath, result]() {
            isGenerating_ = false;
            emit stateChanged();

            if (result.has_value()) {
                qDebug() << "[ProxyProvider] Proxy ready:" << *result;
                emit proxyReady(sourcePath, *result);
            } else {
                qWarning() << "[ProxyProvider] Proxy generation failed for:" << sourcePath;
                emit proxyFailed(sourcePath, QStringLiteral("Proxy generation failed"));
            }
        }, Qt::QueuedConnection);
    });
}

void ProxyProvider::cancel() {
    if (!isGenerating_) return;
    ProxyGenerationService::instance().cancelAll();
    isGenerating_ = false;
    emit stateChanged();
}

} // namespace gopost::video_editor
