#include "auth/data/datasources/auth_remote_datasource.h"

#include <QNetworkRequest>
#include <QJsonDocument>
#include <QJsonObject>
#include <QUrl>

namespace gopost::auth {

AuthRemoteDataSourceImpl::AuthRemoteDataSourceImpl(
    QNetworkAccessManager* networkManager,
    const QString& baseUrl,
    QObject* parent)
    : AuthRemoteDataSource(parent)
    , m_networkManager(networkManager)
    , m_baseUrl(baseUrl)
{
}

void AuthRemoteDataSourceImpl::register_(
    const QString& name,
    const QString& email,
    const QString& password,
    std::function<void(AuthResponseModel)> onSuccess,
    std::function<void(const QString&, std::optional<int>, const QString&)> onError)
{
    QJsonObject body;
    body[QStringLiteral("name")] = name;
    body[QStringLiteral("email")] = email;
    body[QStringLiteral("password")] = password;

    postJson(QStringLiteral("/auth/register"), body,
        [onSuccess = std::move(onSuccess)](const QJsonObject& data) {
            onSuccess(AuthResponseModel::fromJson(data));
        },
        std::move(onError));
}

void AuthRemoteDataSourceImpl::login(
    const QString& email,
    const QString& password,
    std::function<void(AuthResponseModel)> onSuccess,
    std::function<void(const QString&, std::optional<int>, const QString&)> onError)
{
    QJsonObject body;
    body[QStringLiteral("email")] = email;
    body[QStringLiteral("password")] = password;

    postJson(QStringLiteral("/auth/login"), body,
        [onSuccess = std::move(onSuccess)](const QJsonObject& data) {
            onSuccess(AuthResponseModel::fromJson(data));
        },
        std::move(onError));
}

void AuthRemoteDataSourceImpl::refreshToken(
    const QString& refreshToken,
    std::function<void(AuthResponseModel)> onSuccess,
    std::function<void(const QString&, std::optional<int>, const QString&)> onError)
{
    QJsonObject body;
    body[QStringLiteral("refresh_token")] = refreshToken;

    postJson(QStringLiteral("/auth/refresh"), body,
        [onSuccess = std::move(onSuccess)](const QJsonObject& data) {
            onSuccess(AuthResponseModel::fromJson(data));
        },
        std::move(onError));
}

void AuthRemoteDataSourceImpl::logout(
    const QString& refreshToken,
    std::function<void()> onSuccess,
    std::function<void(const QString&, std::optional<int>, const QString&)> onError)
{
    QJsonObject body;
    body[QStringLiteral("refresh_token")] = refreshToken;

    postJson(QStringLiteral("/auth/logout"), body,
        [onSuccess = std::move(onSuccess)](const QJsonObject& /*data*/) {
            onSuccess();
        },
        std::move(onError));
}

void AuthRemoteDataSourceImpl::oauthLogin(
    const QString& provider,
    const QString& idToken,
    std::function<void(AuthResponseModel)> onSuccess,
    std::function<void(const QString&, std::optional<int>, const QString&)> onError)
{
    QJsonObject body;
    body[QStringLiteral("id_token")] = idToken;

    postJson(QStringLiteral("/auth/oauth/") + provider, body,
        [onSuccess = std::move(onSuccess)](const QJsonObject& data) {
            onSuccess(AuthResponseModel::fromJson(data));
        },
        std::move(onError));
}

void AuthRemoteDataSourceImpl::postJson(
    const QString& endpoint,
    const QJsonObject& body,
    std::function<void(QJsonObject)> onSuccess,
    std::function<void(const QString&, std::optional<int>, const QString&)> onError)
{
    QUrl url(m_baseUrl + endpoint);
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/json"));

    QByteArray payload = QJsonDocument(body).toJson(QJsonDocument::Compact);
    QNetworkReply* reply = m_networkManager->post(request, payload);

    connect(reply, &QNetworkReply::finished, this, [reply, onSuccess = std::move(onSuccess), onError = std::move(onError)]() {
        reply->deleteLater();

        const int statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        const QByteArray responseData = reply->readAll();
        const QJsonDocument doc = QJsonDocument::fromJson(responseData);
        const QJsonObject responseObj = doc.object();

        if (reply->error() == QNetworkReply::NoError && statusCode >= 200 && statusCode < 300) {
            const QJsonObject data = responseObj[QStringLiteral("data")].toObject();
            onSuccess(data);
        } else if (reply->error() == QNetworkReply::ConnectionRefusedError
                   || reply->error() == QNetworkReply::HostNotFoundError
                   || reply->error() == QNetworkReply::TimeoutError) {
            onError(QStringLiteral("Network error"), std::nullopt, QStringLiteral("NETWORK_ERROR"));
        } else {
            const QString message = extractMessage(responseObj);
            const QString code = extractCode(responseObj);
            onError(message, statusCode, code);
        }
    });
}

QString AuthRemoteDataSourceImpl::extractMessage(const QJsonObject& responseObj)
{
    const QJsonObject error = responseObj[QStringLiteral("error")].toObject();
    if (!error.isEmpty()) {
        const QString msg = error[QStringLiteral("message")].toString();
        if (!msg.isEmpty()) return msg;
    }
    return QStringLiteral("Server error");
}

QString AuthRemoteDataSourceImpl::extractCode(const QJsonObject& responseObj)
{
    const QJsonObject error = responseObj[QStringLiteral("error")].toObject();
    if (!error.isEmpty()) {
        return error[QStringLiteral("code")].toString();
    }
    return {};
}

} // namespace gopost::auth
