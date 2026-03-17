#pragma once

#include <QObject>
#include <QString>

#include "auth/domain/entities/user_entity.h"

namespace gopost::auth {

/// Authentication status enum, registered with Qt meta system.
class AuthStatusClass {
    Q_GADGET
public:
    enum Value {
        Initial,
        Loading,
        Authenticated,
        Unauthenticated,
        Guest,
        Error
    };
    Q_ENUM(Value)
};

using AuthStatus = AuthStatusClass::Value;

/// Value type representing the full authentication state.
class AuthState {
    Q_GADGET
    Q_PROPERTY(int status MEMBER m_statusInt)
    Q_PROPERTY(QString accessToken MEMBER m_accessToken)
    Q_PROPERTY(QString refreshToken MEMBER m_refreshToken)
    Q_PROPERTY(QString errorMessage MEMBER m_errorMessage)

public:
    AuthState() = default;

    AuthState(AuthStatus status,
              const UserEntity& user = {},
              const QString& accessToken = {},
              const QString& refreshToken = {},
              const QString& errorMessage = {})
        : m_status(status)
        , m_statusInt(static_cast<int>(status))
        , m_user(user)
        , m_accessToken(accessToken)
        , m_refreshToken(refreshToken)
        , m_errorMessage(errorMessage) {}

    AuthStatus status() const { return m_status; }
    const UserEntity& user() const { return m_user; }
    const QString& accessToken() const { return m_accessToken; }
    const QString& refreshToken() const { return m_refreshToken; }
    const QString& errorMessage() const { return m_errorMessage; }

    bool isAuthenticated() const { return m_status == AuthStatus::Authenticated; }
    bool isGuest() const { return m_status == AuthStatus::Guest; }
    bool canAccessApp() const { return isAuthenticated() || isGuest(); }

    AuthState copyWith(
        std::optional<AuthStatus> status = std::nullopt,
        std::optional<UserEntity> user = std::nullopt,
        std::optional<QString> accessToken = std::nullopt,
        std::optional<QString> refreshToken = std::nullopt,
        std::optional<QString> errorMessage = std::nullopt) const
    {
        return AuthState(
            status.value_or(m_status),
            user.value_or(m_user),
            accessToken.value_or(m_accessToken),
            refreshToken.value_or(m_refreshToken),
            errorMessage.value_or(m_errorMessage)
        );
    }

    bool operator==(const AuthState& other) const {
        return m_status == other.m_status
            && m_user == other.m_user
            && m_accessToken == other.m_accessToken
            && m_refreshToken == other.m_refreshToken
            && m_errorMessage == other.m_errorMessage;
    }

    bool operator!=(const AuthState& other) const { return !(*this == other); }

private:
    AuthStatus m_status = AuthStatus::Initial;
    int m_statusInt = static_cast<int>(AuthStatus::Initial);
    UserEntity m_user;
    QString m_accessToken;
    QString m_refreshToken;
    QString m_errorMessage;
};

} // namespace gopost::auth

Q_DECLARE_METATYPE(gopost::auth::AuthState)
