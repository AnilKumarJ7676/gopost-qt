#pragma once

#include <QObject>
#include <QSet>
#include <QString>
#include <QSslCertificate>

namespace gopost::core {

/// @deprecated Use SslPinningConfig::configureCertificatePinning from
/// ssl_pinning.h instead.
///
/// This class is kept for backward compatibility. Real SSL certificate
/// pinning is now handled at the QNetworkAccessManager level via
/// SslPinningConfig.
class [[deprecated("Use SslPinningConfig::configureCertificatePinning instead")]]
SslPinningInterceptor : public QObject {
    Q_OBJECT
public:
    explicit SslPinningInterceptor(QObject* parent = nullptr)
        : QObject(parent) {}
    ~SslPinningInterceptor() override = default;
};

} // namespace gopost::core
