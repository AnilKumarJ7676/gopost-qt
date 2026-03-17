#pragma once

#include <QString>
#include <functional>
#include <optional>

#include "auth/domain/repositories/auth_repository.h"

namespace gopost::auth {

class LogoutUseCase {
public:
    explicit LogoutUseCase(AuthRepository* repository)
        : m_repository(repository) {}

    void operator()(const QString& refreshToken,
                    std::function<void(std::optional<core::Failure>)> callback) const {
        m_repository->logout(refreshToken, std::move(callback));
    }

private:
    AuthRepository* m_repository;
};

} // namespace gopost::auth
