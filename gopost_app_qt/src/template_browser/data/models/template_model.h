#pragma once

#include <QJsonObject>
#include <QJsonArray>
#include <QString>
#include <QList>
#include <QStringList>
#include <optional>

#include "template_browser/domain/entities/template_entity.h"

namespace gopost::template_browser {

class EditableFieldModel {
public:
    EditableFieldModel() = default;

    EditableFieldModel(const QString& key,
                       const QString& label,
                       const QString& type,
                       const QString& defaultValue = {})
        : m_key(key), m_label(label), m_type(type), m_defaultValue(defaultValue) {}

    static EditableFieldModel fromJson(const QJsonObject& json);
    EditableField toEntity() const;

    const QString& key() const { return m_key; }
    const QString& label() const { return m_label; }
    const QString& type() const { return m_type; }
    const QString& defaultValue() const { return m_defaultValue; }

private:
    QString m_key;
    QString m_label;
    QString m_type;
    QString m_defaultValue;
};

class TemplateModel {
public:
    TemplateModel() = default;

    static TemplateModel fromJson(const QJsonObject& json);
    TemplateEntity toEntity() const;

    const QString& id() const { return m_id; }
    const QString& name() const { return m_name; }

private:
    static TemplateStatus parseStatus(const QString& status);

    QString m_id;
    QString m_name;
    QString m_description;
    QString m_type;
    QString m_categoryId;
    QString m_creatorId;
    QString m_status;
    QString m_thumbnailUrl;
    QString m_previewUrl;
    int m_width = 1080;
    int m_height = 1920;
    std::optional<int> m_durationMs;
    int m_layerCount = 0;
    QList<EditableFieldModel> m_editableFields;
    int m_usageCount = 0;
    bool m_isPremium = false;
    int m_version = 1;
    QString m_createdAt;
    QString m_updatedAt;
    QString m_publishedAt;
    QStringList m_tags;
    QString m_categoryName;
    QString m_creatorName;
    QString m_creatorAvatarUrl;
};

} // namespace gopost::template_browser
