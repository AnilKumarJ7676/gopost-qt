#pragma once

#include <QString>
#include <QSet>
#include "core/constants/api_constants.h"

namespace gopost::core {

enum class AppEnvironment {
    Development,
    Staging,
    Production
};

class EnvironmentConfig {
public:
    EnvironmentConfig() = delete;

    static AppEnvironment current();
    static QString baseUrl();

    static bool isProduction() { return current() == AppEnvironment::Production; }
    static bool isDevelopment() { return current() == AppEnvironment::Development; }

    static QSet<QString> sslPinnedFingerprints();
};

} // namespace gopost::core
