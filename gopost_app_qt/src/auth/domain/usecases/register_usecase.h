#pragma once

#include <QString>
#include <functional>

#include "auth/domain/repositories/auth_repository.h"

namespace gopost::auth {

class RegisterUseCase {
public:
    explicit RegisterUseCase(AuthRepository* repository)
        : m_repository(repository) {}

    void operator()(const QString& name,
                    const QString& email,
                    const QString& password,
                    std::function<void(AuthResult<AuthPair>)> callback) const {
        m_repository->register_(name, email, password, std::move(callback));
    }

private:
    AuthRepository* m_repository;
};

} // namespace gopost::auth
