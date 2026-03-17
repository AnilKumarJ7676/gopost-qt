#include "template_browser/data/models/template_access_model.h"

#include <QDateTime>

namespace gopost::template_browser {

TemplateAccessModel TemplateAccessModel::fromJson(const QJsonObject& json)
{
    TemplateAccessModel model;
    model.m_signedUrl = json[QStringLiteral("signed_url")].toString();
    model.m_sessionKey = json[QStringLiteral("session_key")].toString();
    model.m_renderToken = json[QStringLiteral("render_token")].toString();
    model.m_expiresAt = json[QStringLiteral("expires_at")].toString();
    return model;
}

TemplateAccess TemplateAccessModel::toEntity() const
{
    return TemplateAccess(
        m_signedUrl,
        m_sessionKey,
        m_renderToken,
        QDateTime::fromString(m_expiresAt, Qt::ISODate)
    );
}

} // namespace gopost::template_browser
