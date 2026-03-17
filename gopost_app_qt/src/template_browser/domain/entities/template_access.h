#pragma once

#include <QObject>
#include <QString>
#include <QDateTime>

namespace gopost::template_browser {

class TemplateAccess {
    Q_GADGET
    Q_PROPERTY(QString signedUrl MEMBER m_signedUrl)
    Q_PROPERTY(QString sessionKey MEMBER m_sessionKey)
    Q_PROPERTY(QString renderToken MEMBER m_renderToken)
    Q_PROPERTY(bool isExpired READ isExpired)

public:
    TemplateAccess() = default;

    TemplateAccess(const QString& signedUrl,
                   const QString& sessionKey,
                   const QString& renderToken,
                   const QDateTime& expiresAt)
        : m_signedUrl(signedUrl)
        , m_sessionKey(sessionKey)
        , m_renderToken(renderToken)
        , m_expiresAt(expiresAt) {}

    const QString& signedUrl() const { return m_signedUrl; }
    const QString& sessionKey() const { return m_sessionKey; }
    const QString& renderToken() const { return m_renderToken; }
    const QDateTime& expiresAt() const { return m_expiresAt; }

    bool isExpired() const { return QDateTime::currentDateTimeUtc() > m_expiresAt; }

    bool operator==(const TemplateAccess& other) const {
        return m_signedUrl == other.m_signedUrl
            && m_sessionKey == other.m_sessionKey
            && m_renderToken == other.m_renderToken
            && m_expiresAt == other.m_expiresAt;
    }
    bool operator!=(const TemplateAccess& other) const { return !(*this == other); }

private:
    QString m_signedUrl;
    QString m_sessionKey;
    QString m_renderToken;
    QDateTime m_expiresAt;
};

} // namespace gopost::template_browser

Q_DECLARE_METATYPE(gopost::template_browser::TemplateAccess)
