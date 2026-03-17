#include "core/config/app_environment.h"

#include <QProcessEnvironment>

namespace gopost::core {

AppEnvironment EnvironmentConfig::current() {
    static const QString envKey =
        QProcessEnvironment::systemEnvironment().value(
            QStringLiteral("APP_ENV"), QStringLiteral("development"));

    if (envKey == QStringLiteral("production"))
        return AppEnvironment::Production;
    if (envKey == QStringLiteral("staging"))
        return AppEnvironment::Staging;
    return AppEnvironment::Development;
}

QString EnvironmentConfig::baseUrl() {
    switch (current()) {
    case AppEnvironment::Production:
        return ApiConstants::productionUrl;
    case AppEnvironment::Staging:
        return ApiConstants::stagingUrl;
    case AppEnvironment::Development:
    default:
        return ApiConstants::baseUrl;
    }
}

QSet<QString> EnvironmentConfig::sslPinnedFingerprints() {
    switch (current()) {
    case AppEnvironment::Production:
        return {
            QStringLiteral("PLACEHOLDER_LEAF_CERT_SHA256"),
            QStringLiteral("PLACEHOLDER_INTERMEDIATE_CERT_SHA256"),
        };
    case AppEnvironment::Staging:
    case AppEnvironment::Development:
    default:
        return {};
    }
}

} // namespace gopost::core
