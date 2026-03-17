#pragma once

#include <QString>
#include <optional>
#include <variant>
#include <utility>

#include "auth/domain/entities/auth_tokens.h"
#include "auth/domain/entities/user_entity.h"
#include "core/error/failures.h"

namespace gopost::auth {

/// Result type: either a value T or a Failure.
template <typename T>
using AuthResult = std::variant<T, core::Failure>;

/// Pair returned by login/register: tokens + user.
using AuthPair = std::pair<AuthTokens, UserEntity>;

/// Abstract repository interface for authentication operations.
class AuthRepository {
public:
    virtual ~AuthRepository() = default;

    virtual void register_(
        const QString& name,
        const QString& email,
        const QString& password,
        std::function<void(AuthResult<AuthPair>)> callback) = 0;

    virtual void login(
        const QString& email,
        const QString& password,
        std::function<void(AuthResult<AuthPair>)> callback) = 0;

    virtual void refreshToken(
        const QString& refreshToken,
        std::function<void(AuthResult<AuthTokens>)> callback) = 0;

    virtual void logout(
        const QString& refreshToken,
        std::function<void(std::optional<core::Failure>)> callback) = 0;

    virtual void oauthLogin(
        const QString& provider,
        const QString& idToken,
        std::function<void(AuthResult<AuthPair>)> callback) = 0;
};

} // namespace gopost::auth
