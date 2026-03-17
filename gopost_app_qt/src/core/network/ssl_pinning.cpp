#include "core/network/ssl_pinning.h"
#include "core/logging/app_logger.h"

#include <QNetworkReply>
#include <QSslError>

namespace gopost::core {

void SslPinningConfig::configureCertificatePinning(
    QNetworkAccessManager* manager,
    const QSet<QString>& fingerprints)
{
    if (fingerprints.isEmpty()) {
        return;
    }

    QSet<QString> normalised;
    for (const auto& f : fingerprints) {
        normalised.insert(normaliseFingerprint(f));
    }

    QObject::connect(manager, &QNetworkAccessManager::sslErrors,
        [normalised](QNetworkReply* reply, const QList<QSslError>& errors) {
            const auto chain = reply->sslConfiguration().peerCertificateChain();
            if (validateCertificate(chain, normalised)) {
                reply->ignoreSslErrors();
            } else {
                const auto host = reply->url().host();
                const auto port = reply->url().port();
                AppLogger::warning(
                    QStringLiteral("SSL Pinning: rejected certificate for %1:%2")
                        .arg(host)
                        .arg(port));
            }
        });
}

bool SslPinningConfig::validateCertificate(
    const QList<QSslCertificate>& chain,
    const QSet<QString>& normalisedFingerprints)
{
    for (const auto& cert : chain) {
        const auto digest = QCryptographicHash::hash(
            cert.toDer(), QCryptographicHash::Sha256);
        const auto hexFingerprint = QString::fromLatin1(digest.toHex()).toUpper();
        if (normalisedFingerprints.contains(hexFingerprint)) {
            return true;
        }
    }
    return false;
}

QString SslPinningConfig::normaliseFingerprint(const QString& fingerprint)
{
    return fingerprint.toUpper().remove(QLatin1Char(':'));
}

} // namespace gopost::core
