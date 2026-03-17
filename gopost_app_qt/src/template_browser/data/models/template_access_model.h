#pragma once

#include <QJsonObject>
#include <QString>

#include "template_browser/domain/entities/template_access.h"

namespace gopost::template_browser {

class TemplateAccessModel {
public:
    TemplateAccessModel() = default;

    TemplateAccessModel(const QString& signedUrl,
                        const QString& sessionKey,
                        const QString& renderToken,
                        const QString& expiresAt)
        : m_signedUrl(signedUrl)
        , m_sessionKey(sessionKey)
        , m_renderToken(renderToken)
        , m_expiresAt(expiresAt) {}

    static TemplateAccessModel fromJson(const QJsonObject& json);
    TemplateAccess toEntity() const;

    const QString& signedUrl() const { return m_signedUrl; }
    const QString& sessionKey() const { return m_sessionKey; }

private:
    QString m_signedUrl;
    QString m_sessionKey;
    QString m_renderToken;
    QString m_expiresAt;
};

} // namespace gopost::template_browser
