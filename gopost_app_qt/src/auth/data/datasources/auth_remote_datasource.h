#pragma once

#include <QObject>
#include <QString>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <functional>

#include "auth/data/models/auth_response_model.h"

namespace gopost::auth {

/// Abstract interface for auth remote data source.
class AuthRemoteDataSource : public QObject {
    Q_OBJECT
public:
    explicit AuthRemoteDataSource(QObject* parent = nullptr)
        : QObject(parent) {}
    ~AuthRemoteDataSource() override = default;

    virtual void register_(
        const QString& name,
        const QString& email,
        const QString& password,
        std::function<void(AuthResponseModel)> onSuccess,
        std::function<void(const QString& message, std::optional<int> statusCode, const QString& code)> onError) = 0;

    virtual void login(
        const QString& email,
        const QString& password,
        std::function<void(AuthResponseModel)> onSuccess,
        std::function<void(const QString& message, std::optional<int> statusCode, const QString& code)> onError) = 0;

    virtual void refreshToken(
        const QString& refreshToken,
        std::function<void(AuthResponseModel)> onSuccess,
        std::function<void(const QString& message, std::optional<int> statusCode, const QString& code)> onError) = 0;

    virtual void logout(
        const QString& refreshToken,
        std::function<void()> onSuccess,
        std::function<void(const QString& message, std::optional<int> statusCode, const QString& code)> onError) = 0;

    virtual void oauthLogin(
        const QString& provider,
        const QString& idToken,
        std::function<void(AuthResponseModel)> onSuccess,
        std::function<void(const QString& message, std::optional<int> statusCode, const QString& code)> onError) = 0;
};

/// Implementation of AuthRemoteDataSource using QNetworkAccessManager.
class AuthRemoteDataSourceImpl : public AuthRemoteDataSource {
    Q_OBJECT
public:
    explicit AuthRemoteDataSourceImpl(QNetworkAccessManager* networkManager,
                                      const QString& baseUrl,
                                      QObject* parent = nullptr);
    ~AuthRemoteDataSourceImpl() override = default;

    void register_(
        const QString& name,
        const QString& email,
        const QString& password,
        std::function<void(AuthResponseModel)> onSuccess,
        std::function<void(const QString& message, std::optional<int> statusCode, const QString& code)> onError) override;

    void login(
        const QString& email,
        const QString& password,
        std::function<void(AuthResponseModel)> onSuccess,
        std::function<void(const QString& message, std::optional<int> statusCode, const QString& code)> onError) override;

    void refreshToken(
        const QString& refreshToken,
        std::function<void(AuthResponseModel)> onSuccess,
        std::function<void(const QString& message, std::optional<int> statusCode, const QString& code)> onError) override;

    void logout(
        const QString& refreshToken,
        std::function<void()> onSuccess,
        std::function<void(const QString& message, std::optional<int> statusCode, const QString& code)> onError) override;

    void oauthLogin(
        const QString& provider,
        const QString& idToken,
        std::function<void(AuthResponseModel)> onSuccess,
        std::function<void(const QString& message, std::optional<int> statusCode, const QString& code)> onError) override;

private:
    void postJson(const QString& endpoint,
                  const QJsonObject& body,
                  std::function<void(QJsonObject)> onSuccess,
                  std::function<void(const QString& message, std::optional<int> statusCode, const QString& code)> onError);

    static QString extractMessage(const QJsonObject& errorBody);
    static QString extractCode(const QJsonObject& errorBody);

    QNetworkAccessManager* m_networkManager;
    QString m_baseUrl;
};

} // namespace gopost::auth
