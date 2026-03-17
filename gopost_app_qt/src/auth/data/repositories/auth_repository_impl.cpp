#include "auth/data/repositories/auth_repository_impl.h"

#include "core/error/failures.h"

namespace gopost::auth {

AuthRepositoryImpl::AuthRepositoryImpl(AuthRemoteDataSource* remoteDataSource)
    : m_remoteDataSource(remoteDataSource)
{
}

void AuthRepositoryImpl::register_(
    const QString& name,
    const QString& email,
    const QString& password,
    std::function<void(AuthResult<AuthPair>)> callback)
{
    m_remoteDataSource->register_(name, email, password,
        [callback](const AuthResponseModel& response) {
            callback(AuthPair{response.toTokens(), response.toUserEntity()});
        },
        [callback](const QString& message, std::optional<int> statusCode, const QString& code) {
            if (code == QStringLiteral("NETWORK_ERROR")) {
                callback(core::NetworkFailure());
            } else {
                callback(core::ServerFailure(message, code, statusCode));
            }
        });
}

void AuthRepositoryImpl::login(
    const QString& email,
    const QString& password,
    std::function<void(AuthResult<AuthPair>)> callback)
{
    m_remoteDataSource->login(email, password,
        [callback](const AuthResponseModel& response) {
            callback(AuthPair{response.toTokens(), response.toUserEntity()});
        },
        [callback](const QString& message, std::optional<int> statusCode, const QString& code) {
            if (code == QStringLiteral("NETWORK_ERROR")) {
                callback(core::NetworkFailure());
            } else {
                callback(core::ServerFailure(message, code, statusCode));
            }
        });
}

void AuthRepositoryImpl::refreshToken(
    const QString& refreshToken,
    std::function<void(AuthResult<AuthTokens>)> callback)
{
    m_remoteDataSource->refreshToken(refreshToken,
        [callback](const AuthResponseModel& response) {
            callback(response.toTokens());
        },
        [callback](const QString& message, std::optional<int> statusCode, const QString& code) {
            if (code == QStringLiteral("NETWORK_ERROR")) {
                callback(core::NetworkFailure());
            } else {
                callback(core::ServerFailure(message, code, statusCode));
            }
        });
}

void AuthRepositoryImpl::logout(
    const QString& refreshToken,
    std::function<void(std::optional<core::Failure>)> callback)
{
    m_remoteDataSource->logout(refreshToken,
        [callback]() {
            callback(std::nullopt);
        },
        [callback](const QString& message, std::optional<int> statusCode, const QString& code) {
            if (code == QStringLiteral("NETWORK_ERROR")) {
                callback(core::NetworkFailure());
            } else {
                callback(core::ServerFailure(message, code, statusCode));
            }
        });
}

void AuthRepositoryImpl::oauthLogin(
    const QString& provider,
    const QString& idToken,
    std::function<void(AuthResult<AuthPair>)> callback)
{
    m_remoteDataSource->oauthLogin(provider, idToken,
        [callback](const AuthResponseModel& response) {
            callback(AuthPair{response.toTokens(), response.toUserEntity()});
        },
        [callback](const QString& message, std::optional<int> statusCode, const QString& code) {
            if (code == QStringLiteral("NETWORK_ERROR")) {
                callback(core::NetworkFailure());
            } else {
                callback(core::ServerFailure(message, code, statusCode));
            }
        });
}

} // namespace gopost::auth
