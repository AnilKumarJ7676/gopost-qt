#pragma once

#include <QString>
#include <functional>

#include "auth/domain/repositories/auth_repository.h"

namespace gopost::auth {

class LoginUseCase {
public:
    explicit LoginUseCase(AuthRepository* repository)
        : m_repository(repository) {}

    void operator()(const QString& email,
                    const QString& password,
                    std::function<void(AuthResult<AuthPair>)> callback) const {
        m_repository->login(email, password, std::move(callback));
    }

private:
    AuthRepository* m_repository;
};

} // namespace gopost::auth
