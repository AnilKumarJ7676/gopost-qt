#pragma once

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>

namespace gopost::core {

/// Retries failed requests (configurable count, delay with exponential backoff).
class RetryInterceptor : public QObject {
    Q_OBJECT
public:
    explicit RetryInterceptor(QNetworkAccessManager* manager,
                              int maxRetries = 3,
                              QObject* parent = nullptr);
    ~RetryInterceptor() override = default;

    /// Check if a failed reply should be retried. If retryable, performs
    /// the retry with exponential backoff and returns the new reply.
    /// Returns nullptr if not retryable or max retries exhausted.
    QNetworkReply* onError(QNetworkReply* reply);

    int maxRetries() const { return m_maxRetries; }
    void setMaxRetries(int maxRetries) { m_maxRetries = maxRetries; }

private:
    bool isRetryable(QNetworkReply::NetworkError error) const;

    QNetworkAccessManager* m_manager;
    int m_maxRetries;
};

} // namespace gopost::core
