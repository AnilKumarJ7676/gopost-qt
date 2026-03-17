#include "core/network/interceptors/auth_interceptor.h"
#include "core/security/secure_storage_service.h"
#include "core/constants/api_constants.h"
#include "core/network/ssl_pinning.h"
#include "core/logging/app_logger.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QEventLoop>
#include <QUrl>

namespace gopost::core {

AuthInterceptor::AuthInterceptor(
    SecureStorageService* secureStorage,
    const QString& baseUrl,
    const QSet<QString>& pinnedCertFingerprints,
    QObject* parent)
    : QObject(parent)
    , m_secureStorage(secureStorage)
    , m_refreshManager(buildRefreshManager(baseUrl, pinnedCertFingerprints, this))
    , m_baseUrl(baseUrl)
{
}

QNetworkAccessManager* AuthInterceptor::buildRefreshManager(
    const QString& baseUrl,
    const QSet<QString>& pinnedCertFingerprints,
    QObject* parent)
{
    auto* manager = new QNetworkAccessManager(parent);
    manager->setTransferTimeout(
        static_cast<int>(std::chrono::milliseconds(ApiConstants::connectTimeout).count()));
    SslPinningConfig::configureCertificatePinning(manager, pinnedCertFingerprints);
    return manager;
}

void AuthInterceptor::setToken(const QString& token)
{
    m_accessToken = token;
}

void AuthInterceptor::clearToken()
{
    m_accessToken.clear();
}

QString AuthInterceptor::accessToken() const
{
    return m_accessToken;
}

void AuthInterceptor::onRequest(QNetworkRequest& request)
{
    if (!m_accessToken.isEmpty()) {
        request.setRawHeader("Authorization",
                             QStringLiteral("Bearer %1").arg(m_accessToken).toUtf8());
    }
}

QNetworkReply* AuthInterceptor::onResponse(
    QNetworkReply* reply,
    QNetworkAccessManager* manager)
{
    if (reply->error() == QNetworkReply::NoError) {
        return nullptr;
    }

    const int statusCode = reply->attribute(
        QNetworkRequest::HttpStatusCodeAttribute).toInt();

    if (statusCode != 401 || isRefreshRequest(reply->url())) {
        return nullptr;
    }

    return attemptTokenRefresh(reply, manager);
}

QNetworkReply* AuthInterceptor::attemptTokenRefresh(
    QNetworkReply* originalReply,
    QNetworkAccessManager* manager)
{
    const auto refreshToken = m_secureStorage->read(ApiConstants::refreshTokenKey);
    if (!refreshToken.has_value()) {
        forceLogout();
        return nullptr;
    }

    // Build refresh request
    QNetworkRequest refreshRequest(QUrl(m_baseUrl + QStringLiteral("/auth/refresh")));
    refreshRequest.setHeader(QNetworkRequest::ContentTypeHeader,
                             QStringLiteral("application/json"));
    refreshRequest.setRawHeader("Accept", "application/json");

    QJsonObject body;
    body[QStringLiteral("refresh_token")] = refreshToken.value();
    const auto bodyData = QJsonDocument(body).toJson(QJsonDocument::Compact);

    // Synchronous refresh via event loop
    QEventLoop loop;
    auto* refreshReply = m_refreshManager->post(refreshRequest, bodyData);
    QObject::connect(refreshReply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();

    if (refreshReply->error() != QNetworkReply::NoError) {
        AppLogger::warning(QStringLiteral("AuthInterceptor: refresh failed, forcing logout"));
        refreshReply->deleteLater();
        forceLogout();
        return nullptr;
    }

    const auto responseData = QJsonDocument::fromJson(refreshReply->readAll()).object();
    refreshReply->deleteLater();

    const auto dataObj = responseData[QStringLiteral("data")].toObject();
    const auto newAccessToken = dataObj[QStringLiteral("access_token")].toString();
    const auto newRefreshToken = dataObj[QStringLiteral("refresh_token")].toString();

    m_accessToken = newAccessToken;
    m_secureStorage->write(ApiConstants::refreshTokenKey, newRefreshToken);

    AppLogger::debug(QStringLiteral("AuthInterceptor: token refreshed successfully"));
    emit tokenRefreshed(newAccessToken);

    // Retry original request with new token
    QNetworkRequest retryRequest(originalReply->url());
    // Copy headers from original
    for (const auto& header : originalReply->request().rawHeaderList()) {
        retryRequest.setRawHeader(header, originalReply->request().rawHeader(header));
    }
    retryRequest.setRawHeader("Authorization",
                              QStringLiteral("Bearer %1").arg(newAccessToken).toUtf8());

    // Determine original HTTP method and retry
    const auto verb = originalReply->request().attribute(
        QNetworkRequest::CustomVerbAttribute).toByteArray();
    const auto operation = originalReply->operation();

    switch (operation) {
    case QNetworkAccessManager::GetOperation:
        return manager->get(retryRequest);
    case QNetworkAccessManager::PostOperation:
        return manager->post(retryRequest, originalReply->readAll());
    case QNetworkAccessManager::PutOperation:
        return manager->put(retryRequest, originalReply->readAll());
    case QNetworkAccessManager::DeleteOperation:
        return manager->deleteResource(retryRequest);
    default:
        return manager->sendCustomRequest(retryRequest, verb);
    }
}

bool AuthInterceptor::isRefreshRequest(const QUrl& url) const
{
    return url.path().contains(QStringLiteral("/auth/refresh"));
}

void AuthInterceptor::forceLogout()
{
    m_accessToken.clear();
    m_secureStorage->delete_(ApiConstants::refreshTokenKey);
    if (onSessionExpired) {
        onSessionExpired();
    }
    emit sessionExpired();
}

} // namespace gopost::core
