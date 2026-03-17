#pragma once

#include <QString>
#include <QJsonObject>

#include "auth/domain/entities/auth_tokens.h"
#include "auth/domain/entities/user_entity.h"

namespace gopost::auth {

class UserModel {
public:
    UserModel() = default;

    UserModel(const QString& id,
              const QString& email,
              const QString& name,
              const QString& avatarUrl = {})
        : m_id(id), m_email(email), m_name(name), m_avatarUrl(avatarUrl) {}

    static UserModel fromJson(const QJsonObject& json);

    UserEntity toEntity() const;

    const QString& id() const { return m_id; }
    const QString& email() const { return m_email; }
    const QString& name() const { return m_name; }
    const QString& avatarUrl() const { return m_avatarUrl; }

private:
    QString m_id;
    QString m_email;
    QString m_name;
    QString m_avatarUrl;
};

class AuthResponseModel {
public:
    AuthResponseModel() = default;

    AuthResponseModel(const QString& accessToken,
                      const QString& refreshToken,
                      int expiresIn,
                      const UserModel& user)
        : m_accessToken(accessToken)
        , m_refreshToken(refreshToken)
        , m_expiresIn(expiresIn)
        , m_user(user) {}

    static AuthResponseModel fromJson(const QJsonObject& json);

    AuthTokens toTokens() const;
    UserEntity toUserEntity() const;

    const QString& accessToken() const { return m_accessToken; }
    const QString& refreshToken() const { return m_refreshToken; }
    int expiresIn() const { return m_expiresIn; }
    const UserModel& user() const { return m_user; }

private:
    QString m_accessToken;
    QString m_refreshToken;
    int m_expiresIn = 0;
    UserModel m_user;
};

} // namespace gopost::auth
