#pragma once

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QSet>
#include <QString>
#include <QJsonDocument>
#include <QJsonObject>
#include <QByteArray>

namespace gopost::core {

class AuthInterceptor;
class LoggingInterceptor;
class ErrorInterceptor;
class RetryInterceptor;

/// HTTP client wrapping QNetworkAccessManager.
///
/// Configures timeouts (30s), default headers (Content-Type, Accept: application/json),
/// and integrates the interceptor chain (auth, logging, error, retry).
class HttpClient : public QObject {
    Q_OBJECT
public:
    explicit HttpClient(
        AuthInterceptor* authInterceptor,
        const QString& baseUrl = {},
        const QSet<QString>& pinnedCertFingerprints = {},
        QObject* parent = nullptr);
    ~HttpClient() override = default;

    /// GET request. Returns QNetworkReply* (caller must handle finished signal).
    QNetworkReply* get(const QString& path,
                       const QMap<QString, QString>& queryParams = {});

    /// POST request with JSON body.
    QNetworkReply* post(const QString& path,
                        const QJsonObject& body = {});

    /// PUT request with JSON body.
    QNetworkReply* put(const QString& path,
                       const QJsonObject& body = {});

    /// DELETE request.
    QNetworkReply* delete_(const QString& path);

    /// Access the underlying QNetworkAccessManager.
    QNetworkAccessManager* networkManager() const { return m_manager; }

    /// Access the auth interceptor.
    AuthInterceptor* authInterceptor() const { return m_authInterceptor; }

private:
    QNetworkRequest buildRequest(const QString& path) const;
    void setupInterceptors(QNetworkReply* reply);

    QNetworkAccessManager* m_manager;
    AuthInterceptor* m_authInterceptor;
    LoggingInterceptor* m_loggingInterceptor;
    ErrorInterceptor* m_errorInterceptor;
    RetryInterceptor* m_retryInterceptor;
    QString m_baseUrl;
};

} // namespace gopost::core
