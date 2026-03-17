#include "core/network/http_client.h"
#include "core/network/interceptors/auth_interceptor.h"
#include "core/network/interceptors/error_interceptor.h"
#include "core/network/interceptors/logging_interceptor.h"
#include "core/network/interceptors/retry_interceptor.h"
#include "core/network/ssl_pinning.h"
#include "core/config/app_environment.h"

#include <QUrlQuery>

namespace gopost::core {

HttpClient::HttpClient(
    AuthInterceptor* authInterceptor,
    const QString& baseUrl,
    const QSet<QString>& pinnedCertFingerprints,
    QObject* parent)
    : QObject(parent)
    , m_manager(new QNetworkAccessManager(this))
    , m_authInterceptor(authInterceptor)
    , m_loggingInterceptor(new LoggingInterceptor(this))
    , m_errorInterceptor(new ErrorInterceptor(this))
    , m_retryInterceptor(new RetryInterceptor(m_manager, 3, this))
    , m_baseUrl(baseUrl.isEmpty() ? EnvironmentConfig::baseUrl() : baseUrl)
{
    // Configure timeouts (30 seconds)
    m_manager->setTransferTimeout(30000);

    // Configure SSL pinning
    SslPinningConfig::configureCertificatePinning(m_manager, pinnedCertFingerprints);
}

QNetworkRequest HttpClient::buildRequest(const QString& path) const
{
    QNetworkRequest request(QUrl(m_baseUrl + path));
    request.setHeader(QNetworkRequest::ContentTypeHeader,
                      QStringLiteral("application/json"));
    request.setRawHeader("Accept", "application/json");
    return request;
}

void HttpClient::setupInterceptors(QNetworkReply* reply)
{
    QObject::connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        // Logging interceptor
        m_loggingInterceptor->onResponse(reply);

        // Auth interceptor (handles 401 + token refresh)
        auto* retryReply = m_authInterceptor->onResponse(reply, m_manager);
        if (retryReply) {
            setupInterceptors(retryReply);
            reply->deleteLater();
            return;
        }

        // Retry interceptor for transient errors
        if (reply->error() != QNetworkReply::NoError) {
            auto* retriedReply = m_retryInterceptor->onError(reply);
            if (retriedReply) {
                setupInterceptors(retriedReply);
                reply->deleteLater();
                return;
            }
        }

        // Error interceptor (throws application exceptions)
        m_errorInterceptor->onResponse(reply);
    });
}

QNetworkReply* HttpClient::get(const QString& path,
                                const QMap<QString, QString>& queryParams)
{
    auto request = buildRequest(path);

    if (!queryParams.isEmpty()) {
        QUrl url = request.url();
        QUrlQuery query;
        for (auto it = queryParams.constBegin(); it != queryParams.constEnd(); ++it) {
            query.addQueryItem(it.key(), it.value());
        }
        url.setQuery(query);
        request.setUrl(url);
    }

    // Log outgoing request
    m_loggingInterceptor->onRequest(request, "GET");

    // Inject auth header
    m_authInterceptor->onRequest(request);

    auto* reply = m_manager->get(request);
    setupInterceptors(reply);
    return reply;
}

QNetworkReply* HttpClient::post(const QString& path, const QJsonObject& body)
{
    auto request = buildRequest(path);

    // Log outgoing request
    m_loggingInterceptor->onRequest(request, "POST");

    // Inject auth header
    m_authInterceptor->onRequest(request);

    const auto bodyData = QJsonDocument(body).toJson(QJsonDocument::Compact);
    auto* reply = m_manager->post(request, bodyData);
    setupInterceptors(reply);
    return reply;
}

QNetworkReply* HttpClient::put(const QString& path, const QJsonObject& body)
{
    auto request = buildRequest(path);

    // Log outgoing request
    m_loggingInterceptor->onRequest(request, "PUT");

    // Inject auth header
    m_authInterceptor->onRequest(request);

    const auto bodyData = QJsonDocument(body).toJson(QJsonDocument::Compact);
    auto* reply = m_manager->put(request, bodyData);
    setupInterceptors(reply);
    return reply;
}

QNetworkReply* HttpClient::delete_(const QString& path)
{
    auto request = buildRequest(path);

    // Log outgoing request
    m_loggingInterceptor->onRequest(request, "DELETE");

    // Inject auth header
    m_authInterceptor->onRequest(request);

    auto* reply = m_manager->deleteResource(request);
    setupInterceptors(reply);
    return reply;
}

} // namespace gopost::core
