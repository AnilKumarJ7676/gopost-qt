#pragma once

#include <QObject>
#include <QString>

namespace gopost::auth {

class AuthTokens {
    Q_GADGET
    Q_PROPERTY(QString accessToken MEMBER m_accessToken)
    Q_PROPERTY(QString refreshToken MEMBER m_refreshToken)
    Q_PROPERTY(int expiresIn MEMBER m_expiresIn)

public:
    AuthTokens() = default;

    AuthTokens(const QString& accessToken,
               const QString& refreshToken,
               int expiresIn)
        : m_accessToken(accessToken)
        , m_refreshToken(refreshToken)
        , m_expiresIn(expiresIn) {}

    const QString& accessToken() const { return m_accessToken; }
    const QString& refreshToken() const { return m_refreshToken; }
    int expiresIn() const { return m_expiresIn; }

    bool isValid() const { return !m_accessToken.isEmpty(); }

    bool operator==(const AuthTokens& other) const {
        return m_accessToken == other.m_accessToken
            && m_refreshToken == other.m_refreshToken
            && m_expiresIn == other.m_expiresIn;
    }

    bool operator!=(const AuthTokens& other) const { return !(*this == other); }

private:
    QString m_accessToken;
    QString m_refreshToken;
    int m_expiresIn = 0;
};

} // namespace gopost::auth

Q_DECLARE_METATYPE(gopost::auth::AuthTokens)
