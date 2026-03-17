#pragma once

#include <QSet>
#include <QString>
#include <QSslConfiguration>
#include <QSslCertificate>
#include <QCryptographicHash>
#include <QNetworkAccessManager>

namespace gopost::core {

/// Configures a QNetworkAccessManager to enforce certificate pinning against
/// a set of SHA-256 fingerprints.
///
/// Fingerprints should be uppercase hex strings (with or without colons).
/// If \a fingerprints is empty the function is a no-op and standard system
/// certificate validation applies.
class SslPinningConfig {
public:
    SslPinningConfig() = default;

    /// Configure SSL pinning on the given network access manager.
    static void configureCertificatePinning(
        QNetworkAccessManager* manager,
        const QSet<QString>& fingerprints);

    /// Validate a certificate chain against pinned fingerprints.
    /// Returns true if at least one certificate matches a pinned fingerprint.
    static bool validateCertificate(
        const QList<QSslCertificate>& chain,
        const QSet<QString>& normalisedFingerprints);

    /// Normalise a fingerprint string: uppercase, colons removed.
    static QString normaliseFingerprint(const QString& fingerprint);
};

} // namespace gopost::core
