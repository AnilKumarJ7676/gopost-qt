#pragma once

#include <QObject>
#include <QString>
#include <optional>

#include "template_browser/domain/entities/template_access.h"
#include "template_browser/domain/repositories/template_repository.h"
#include "template_browser/domain/usecases/request_template_access_usecase.h"

namespace gopost::template_browser {

enum class DownloadStatus {
    Idle,
    RequestingAccess,
    Downloading,
    LoadingInEngine,
    Complete,
    Error
};

/// SRP: Manages the encrypted template download lifecycle:
/// request access -> download from CDN -> load in native engine (decrypt + parse).
class DownloadNotifier : public QObject {
    Q_OBJECT
    Q_PROPERTY(int status READ statusInt NOTIFY statusChanged)
    Q_PROPERTY(double progress READ progress NOTIFY progressChanged)
    Q_PROPERTY(QString error READ error NOTIFY errorChanged)

public:
    DownloadNotifier(RequestTemplateAccessUseCase* requestAccess,
                     TemplateAccessRepository* accessRepo,
                     QObject* parent = nullptr);
    ~DownloadNotifier() override = default;

    DownloadStatus status() const { return m_status; }
    int statusInt() const { return static_cast<int>(m_status); }
    double progress() const { return m_progress; }
    QString error() const { return m_error; }
    std::optional<TemplateAccess> access() const { return m_access; }

    Q_INVOKABLE void download(const QString& templateId);
    Q_INVOKABLE void reset();

signals:
    void statusChanged();
    void progressChanged();
    void errorChanged();
    void downloadComplete();

private:
    void setStatus(DownloadStatus status);

    RequestTemplateAccessUseCase* m_requestAccess;
    TemplateAccessRepository* m_accessRepo;

    DownloadStatus m_status = DownloadStatus::Idle;
    double m_progress = 0.0;
    QString m_error;
    std::optional<TemplateAccess> m_access;
    QString m_localPath;
};

} // namespace gopost::template_browser
