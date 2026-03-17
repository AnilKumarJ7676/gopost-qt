#include "auth/data/models/auth_response_model.h"

namespace gopost::auth {

// ---------- UserModel ----------

UserModel UserModel::fromJson(const QJsonObject& json)
{
    return UserModel(
        json[QStringLiteral("id")].toString(),
        json[QStringLiteral("email")].toString(),
        json[QStringLiteral("name")].toString(),
        json[QStringLiteral("avatar_url")].toString()
    );
}

UserEntity UserModel::toEntity() const
{
    return UserEntity(m_id, m_email, m_name, m_avatarUrl);
}

// ---------- AuthResponseModel ----------

AuthResponseModel AuthResponseModel::fromJson(const QJsonObject& json)
{
    return AuthResponseModel(
        json[QStringLiteral("access_token")].toString(),
        json[QStringLiteral("refresh_token")].toString(),
        json[QStringLiteral("expires_in")].toInt(),
        UserModel::fromJson(json[QStringLiteral("user")].toObject())
    );
}

AuthTokens AuthResponseModel::toTokens() const
{
    return AuthTokens(m_accessToken, m_refreshToken, m_expiresIn);
}

UserEntity AuthResponseModel::toUserEntity() const
{
    return m_user.toEntity();
}

} // namespace gopost::auth
