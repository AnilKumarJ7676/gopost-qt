#pragma once

#include <QString>
#include <chrono>

namespace gopost::core {

namespace ApiConstants {

inline const QString baseUrl = QStringLiteral("http://localhost:8080/api/v1");
inline const QString stagingUrl = QStringLiteral("https://api-staging.gopost.app/api/v1");
inline const QString productionUrl = QStringLiteral("https://api.gopost.app/api/v1");

inline constexpr std::chrono::seconds connectTimeout{30};
inline constexpr std::chrono::seconds receiveTimeout{30};

inline const QString refreshTokenKey = QStringLiteral("refresh_token");

} // namespace ApiConstants

} // namespace gopost::core
