#pragma once

#include <QObject>
#include <QString>

namespace gopost::video_editor {

// ---------------------------------------------------------------------------
// ProxyProvider — wraps the ProxyGenerationService for QML access
//
// Converted 1:1 from proxy_provider.dart.
// ---------------------------------------------------------------------------
class ProxyProvider : public QObject {
    Q_OBJECT

    Q_PROPERTY(bool isGenerating READ isGenerating NOTIFY stateChanged)

public:
    explicit ProxyProvider(QObject* parent = nullptr);
    ~ProxyProvider() override;

    bool isGenerating() const { return isGenerating_; }

    /// Generate a proxy file for the given source path.
    Q_INVOKABLE void generate(const QString& sourcePath,
                               const QString& outputPath);

    /// Cancel any in-progress generation.
    Q_INVOKABLE void cancel();

signals:
    void stateChanged();
    void proxyReady(const QString& sourcePath, const QString& proxyPath);
    void proxyFailed(const QString& sourcePath, const QString& error);

private:
    bool isGenerating_ = false;
};

} // namespace gopost::video_editor
