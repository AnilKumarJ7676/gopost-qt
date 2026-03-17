#include "template_browser/data/models/template_model.h"

#include <QDateTime>

namespace gopost::template_browser {

// --- EditableFieldModel ---

EditableFieldModel EditableFieldModel::fromJson(const QJsonObject& json)
{
    EditableFieldModel model;
    model.m_key = json[QStringLiteral("key")].toString();
    model.m_label = json[QStringLiteral("label")].toString();
    model.m_type = json[QStringLiteral("type")].toString();
    model.m_defaultValue = json[QStringLiteral("default_value")].toString();
    return model;
}

EditableField EditableFieldModel::toEntity() const
{
    EditableFieldType ft = EditableFieldType::Text;
    if (m_type == QStringLiteral("image"))
        ft = EditableFieldType::Image;
    else if (m_type == QStringLiteral("color"))
        ft = EditableFieldType::Color;
    else if (m_type == QStringLiteral("number"))
        ft = EditableFieldType::Number;

    return EditableField(m_key, m_label, ft, m_defaultValue);
}

// --- TemplateModel ---

TemplateModel TemplateModel::fromJson(const QJsonObject& json)
{
    TemplateModel model;
    model.m_id = json[QStringLiteral("id")].toString();
    model.m_name = json[QStringLiteral("name")].toString();
    model.m_description = json[QStringLiteral("description")].toString();
    model.m_type = json[QStringLiteral("type")].toString();
    model.m_categoryId = json[QStringLiteral("category_id")].toString();
    model.m_creatorId = json[QStringLiteral("creator_id")].toString();
    model.m_status = json[QStringLiteral("status")].toString(QStringLiteral("published"));
    model.m_thumbnailUrl = json[QStringLiteral("thumbnail_url")].toString();
    model.m_previewUrl = json[QStringLiteral("preview_url")].toString();
    model.m_width = json[QStringLiteral("width")].toInt(1080);
    model.m_height = json[QStringLiteral("height")].toInt(1920);

    if (json.contains(QStringLiteral("duration_ms")) && !json[QStringLiteral("duration_ms")].isNull())
        model.m_durationMs = json[QStringLiteral("duration_ms")].toInt();

    model.m_layerCount = json[QStringLiteral("layer_count")].toInt(0);

    const auto fieldsArray = json[QStringLiteral("editable_fields")].toArray();
    for (const auto& fieldVal : fieldsArray) {
        model.m_editableFields.append(EditableFieldModel::fromJson(fieldVal.toObject()));
    }

    model.m_usageCount = json[QStringLiteral("usage_count")].toInt(0);
    model.m_isPremium = json[QStringLiteral("is_premium")].toBool(false);
    model.m_version = json[QStringLiteral("version")].toInt(1);
    model.m_createdAt = json[QStringLiteral("created_at")].toString();
    model.m_updatedAt = json[QStringLiteral("updated_at")].toString();
    model.m_publishedAt = json[QStringLiteral("published_at")].toString();

    const auto tagsArray = json[QStringLiteral("tags")].toArray();
    for (const auto& tagVal : tagsArray) {
        model.m_tags.append(tagVal.toString());
    }

    model.m_categoryName = json[QStringLiteral("category_name")].toString();
    model.m_creatorName = json[QStringLiteral("creator_name")].toString();
    model.m_creatorAvatarUrl = json[QStringLiteral("creator_avatar_url")].toString();

    return model;
}

TemplateEntity TemplateModel::toEntity() const
{
    TemplateType type = (m_type == QStringLiteral("video"))
        ? TemplateType::Video : TemplateType::Image;

    QList<EditableField> fields;
    fields.reserve(m_editableFields.size());
    for (const auto& f : m_editableFields) {
        fields.append(f.toEntity());
    }

    std::optional<QDateTime> publishedAt;
    if (!m_publishedAt.isEmpty()) {
        publishedAt = QDateTime::fromString(m_publishedAt, Qt::ISODate);
    }

    return TemplateEntity(
        m_id,
        m_name,
        m_description,
        type,
        m_categoryId,
        m_creatorId,
        parseStatus(m_status),
        m_thumbnailUrl,
        m_previewUrl,
        m_width,
        m_height,
        m_durationMs,
        m_layerCount,
        fields,
        m_usageCount,
        m_isPremium,
        m_version,
        QDateTime::fromString(m_createdAt, Qt::ISODate),
        QDateTime::fromString(m_updatedAt, Qt::ISODate),
        publishedAt,
        m_tags,
        m_categoryName,
        m_creatorName,
        m_creatorAvatarUrl
    );
}

TemplateStatus TemplateModel::parseStatus(const QString& status)
{
    if (status == QStringLiteral("draft")) return TemplateStatus::Draft;
    if (status == QStringLiteral("review")) return TemplateStatus::Review;
    if (status == QStringLiteral("archived")) return TemplateStatus::Archived;
    return TemplateStatus::Published;
}

} // namespace gopost::template_browser
