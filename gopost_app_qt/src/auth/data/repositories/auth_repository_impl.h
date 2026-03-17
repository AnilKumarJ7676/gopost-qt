#pragma once

#include "auth/domain/repositories/auth_repository.h"
#include "auth/data/datasources/auth_remote_datasource.h"

namespace gopost::auth {

class AuthRepositoryImpl : public AuthRepository {
public:
    explicit AuthRepositoryImpl(AuthRemoteDataSource* remoteDataSource);
    ~AuthRepositoryImpl() override = default;

    void register_(
        const QString& name,
        const QString& email,
        const QString& password,
        std::function<void(AuthResult<AuthPair>)> callback) override;

    void login(
        const QString& email,
        const QString& password,
        std::function<void(AuthResult<AuthPair>)> callback) override;

    void refreshToken(
        const QString& refreshToken,
        std::function<void(AuthResult<AuthTokens>)> callback) override;

    void logout(
        const QString& refreshToken,
        std::function<void(std::optional<core::Failure>)> callback) override;

    void oauthLogin(
        const QString& provider,
        const QString& idToken,
        std::function<void(AuthResult<AuthPair>)> callback) override;

private:
    AuthRemoteDataSource* m_remoteDataSource;
};

} // namespace gopost::auth
