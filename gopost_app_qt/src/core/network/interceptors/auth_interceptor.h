#pragma once

#include <QObject>
#include <QString>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QSet>
#include <functional>

namespace gopost::core {

class SecureStorageService;

/// Handles Bearer token injection, automatic 401 token refresh with retry,
/// and session-expiry notification.
///
/// Concurrent requests hitting 401 are serialised -- only one refresh call
/// is made and the rest wait via a queued mechanism.
class AuthInterceptor : public QObject {
    Q_OBJECT
public:
    explicit AuthInterceptor(
        SecureStorageService* secureStorage,
        const QString& baseUrl,
        const QSet<QString>& pinnedCertFingerprints = {},
        QObject* parent = nullptr);
    ~AuthInterceptor() override = default;

    void setToken(const QString& token);
    void clearToken();
    QString accessToken() const;

    /// Inject Authorization header into the request before sending.
    void onRequest(QNetworkRequest& request);

    /// Handle a reply; if 401, attempt token refresh and retry.
    /// Returns a new QNetworkReply* if the request was retried, or nullptr
    /// if no retry was needed.
    QNetworkReply* onResponse(QNetworkReply* reply,
                              QNetworkAccessManager* manager);

    /// Called when refresh fails or no refresh token exists.
    /// The provider layer hooks this to force-logout the user.
    std::function<void()> onSessionExpired;

signals:
    void sessionExpired();
    void tokenRefreshed(const QString& newToken);

private:
    static QNetworkAccessManager* buildRefreshManager(
        const QString& baseUrl,
        const QSet<QString>& pinnedCertFingerprints,
        QObject* parent);

    bool isRefreshRequest(const QUrl& url) const;
    void forceLogout();

    /// Attempt to refresh the token and retry the original request.
    /// Returns the retry reply, or nullptr on failure.
    QNetworkReply* attemptTokenRefresh(
        QNetworkReply* originalReply,
        QNetworkAccessManager* manager);

    QString m_accessToken;
    SecureStorageService* m_secureStorage;
    QNetworkAccessManager* m_refreshManager;
    QString m_baseUrl;
    bool m_isRefreshing = false;
};

} // namespace gopost::core
