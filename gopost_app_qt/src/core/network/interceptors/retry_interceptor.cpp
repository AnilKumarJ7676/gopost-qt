#include "core/network/interceptors/retry_interceptor.h"

#include <QEventLoop>
#include <QTimer>

namespace gopost::core {

RetryInterceptor::RetryInterceptor(QNetworkAccessManager* manager,
                                   int maxRetries,
                                   QObject* parent)
    : QObject(parent)
    , m_manager(manager)
    , m_maxRetries(maxRetries)
{
}

QNetworkReply* RetryInterceptor::onError(QNetworkReply* reply)
{
    if (!isRetryable(reply->error())) {
        return nullptr;
    }

    const auto originalRequest = reply->request();
    const auto operation = reply->operation();

    int retries = 0;
    while (retries < m_maxRetries) {
        retries++;

        // Exponential backoff: 500ms * 2^retries
        const int delayMs = 500 * (1 << retries);
        QEventLoop waitLoop;
        QTimer::singleShot(delayMs, &waitLoop, &QEventLoop::quit);
        waitLoop.exec();

        QNetworkReply* retryReply = nullptr;
        switch (operation) {
        case QNetworkAccessManager::GetOperation:
            retryReply = m_manager->get(originalRequest);
            break;
        case QNetworkAccessManager::PostOperation:
            retryReply = m_manager->post(originalRequest, reply->readAll());
            break;
        case QNetworkAccessManager::PutOperation:
            retryReply = m_manager->put(originalRequest, reply->readAll());
            break;
        case QNetworkAccessManager::DeleteOperation:
            retryReply = m_manager->deleteResource(originalRequest);
            break;
        default:
            return nullptr;
        }

        // Wait for retry reply
        QEventLoop replyLoop;
        QObject::connect(retryReply, &QNetworkReply::finished,
                         &replyLoop, &QEventLoop::quit);
        replyLoop.exec();

        if (retryReply->error() == QNetworkReply::NoError) {
            return retryReply;
        }

        if (retries >= m_maxRetries) {
            retryReply->deleteLater();
            return nullptr;
        }

        retryReply->deleteLater();
    }

    return nullptr;
}

bool RetryInterceptor::isRetryable(QNetworkReply::NetworkError error) const
{
    switch (error) {
    case QNetworkReply::TimeoutError:
    case QNetworkReply::ConnectionRefusedError:
    case QNetworkReply::RemoteHostClosedError:
    case QNetworkReply::HostNotFoundError:
    case QNetworkReply::TemporaryNetworkFailureError:
    case QNetworkReply::NetworkSessionFailedError:
    case QNetworkReply::UnknownNetworkError:
        return true;
    default:
        return false;
    }
}

} // namespace gopost::core
