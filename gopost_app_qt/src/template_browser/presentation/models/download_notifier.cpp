#include "template_browser/presentation/models/download_notifier.h"

namespace gopost::template_browser {

DownloadNotifier::DownloadNotifier(
    RequestTemplateAccessUseCase* requestAccess,
    TemplateAccessRepository* accessRepo,
    QObject* parent)
    : QObject(parent)
    , m_requestAccess(requestAccess)
    , m_accessRepo(accessRepo)
{
}

void DownloadNotifier::download(const QString& templateId)
{
    if (!m_requestAccess || !m_accessRepo) return;

    setStatus(DownloadStatus::RequestingAccess);
    m_progress = 0.0;
    m_error.clear();
    emit progressChanged();
    emit errorChanged();

    (*m_requestAccess)(templateId, [this](auto result) {
        if (auto* accessVal = std::get_if<TemplateAccess>(&result)) {
            m_access = *accessVal;
            setStatus(DownloadStatus::Downloading);

            m_accessRepo->downloadTemplateToBytes(
                accessVal->signedUrl(),
                [this](int received, int total) {
                    if (total > 0) {
                        m_progress = static_cast<double>(received) / static_cast<double>(total);
                        emit progressChanged();
                    }
                },
                [this](auto downloadResult) {
                    if (auto* bytes = std::get_if<QByteArray>(&downloadResult)) {
                        Q_UNUSED(*bytes);
                        // In the full implementation, the bytes would be passed
                        // to the native engine bridge for decryption and loading.
                        setStatus(DownloadStatus::LoadingInEngine);
                        m_progress = 0.95;
                        emit progressChanged();

                        // Simulate engine loading completion.
                        // In production, TemplateBridge::loadEncryptedTemplate would be called here.
                        m_progress = 1.0;
                        emit progressChanged();
                        setStatus(DownloadStatus::Complete);
                        emit downloadComplete();
                    } else {
                        const auto& failure = std::get<core::Failure>(downloadResult);
                        m_error = failure.message();
                        setStatus(DownloadStatus::Error);
                        emit errorChanged();
                    }
                });
        } else {
            const auto& failure = std::get<core::Failure>(result);
            m_error = failure.message();
            setStatus(DownloadStatus::Error);
            emit errorChanged();
        }
    });
}

void DownloadNotifier::reset()
{
    m_status = DownloadStatus::Idle;
    m_progress = 0.0;
    m_error.clear();
    m_access = std::nullopt;
    m_localPath.clear();

    emit statusChanged();
    emit progressChanged();
    emit errorChanged();
}

void DownloadNotifier::setStatus(DownloadStatus status)
{
    if (m_status != status) {
        m_status = status;
        emit statusChanged();
    }
}

} // namespace gopost::template_browser
