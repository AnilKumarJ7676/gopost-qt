#pragma once

#include <QObject>
#include <QString>
#include <QDateTime>
#include <QList>
#include <QStringList>
#include <optional>

namespace gopost::template_browser {

enum class TemplateType { Video, Image };

enum class TemplateStatus { Draft, Review, Published, Archived };

enum class EditableFieldType { Text, Image, Color, Number };

class EditableField {
    Q_GADGET
    Q_PROPERTY(QString key MEMBER m_key)
    Q_PROPERTY(QString label MEMBER m_label)
    Q_PROPERTY(int fieldType READ fieldTypeInt)
    Q_PROPERTY(QString defaultValue MEMBER m_defaultValue)

public:
    EditableField() = default;

    EditableField(const QString& key,
                  const QString& label,
                  EditableFieldType fieldType,
                  const QString& defaultValue = {})
        : m_key(key)
        , m_label(label)
        , m_fieldType(fieldType)
        , m_defaultValue(defaultValue) {}

    const QString& key() const { return m_key; }
    const QString& label() const { return m_label; }
    EditableFieldType fieldType() const { return m_fieldType; }
    const QString& defaultValue() const { return m_defaultValue; }

    int fieldTypeInt() const { return static_cast<int>(m_fieldType); }

    bool operator==(const EditableField& other) const {
        return m_key == other.m_key
            && m_label == other.m_label
            && m_fieldType == other.m_fieldType
            && m_defaultValue == other.m_defaultValue;
    }
    bool operator!=(const EditableField& other) const { return !(*this == other); }

private:
    QString m_key;
    QString m_label;
    EditableFieldType m_fieldType = EditableFieldType::Text;
    QString m_defaultValue;
};

class TemplateEntity {
    Q_GADGET
    Q_PROPERTY(QString id MEMBER m_id)
    Q_PROPERTY(QString name MEMBER m_name)
    Q_PROPERTY(QString description MEMBER m_description)
    Q_PROPERTY(int type READ typeInt)
    Q_PROPERTY(QString categoryId MEMBER m_categoryId)
    Q_PROPERTY(QString creatorId MEMBER m_creatorId)
    Q_PROPERTY(int status READ statusInt)
    Q_PROPERTY(QString thumbnailUrl MEMBER m_thumbnailUrl)
    Q_PROPERTY(QString previewUrl MEMBER m_previewUrl)
    Q_PROPERTY(int width MEMBER m_width)
    Q_PROPERTY(int height MEMBER m_height)
    Q_PROPERTY(int layerCount MEMBER m_layerCount)
    Q_PROPERTY(int usageCount MEMBER m_usageCount)
    Q_PROPERTY(bool isPremium MEMBER m_isPremium)
    Q_PROPERTY(int version MEMBER m_version)
    Q_PROPERTY(QStringList tags MEMBER m_tags)
    Q_PROPERTY(QString categoryName MEMBER m_categoryName)
    Q_PROPERTY(QString creatorName MEMBER m_creatorName)
    Q_PROPERTY(QString creatorAvatarUrl MEMBER m_creatorAvatarUrl)
    Q_PROPERTY(QString dimensions READ dimensions)
    Q_PROPERTY(bool isVideo READ isVideo)
    Q_PROPERTY(bool isImage READ isImage)

public:
    TemplateEntity() = default;

    TemplateEntity(const QString& id,
                   const QString& name,
                   const QString& description,
                   TemplateType type,
                   const QString& categoryId,
                   const QString& creatorId,
                   TemplateStatus status,
                   const QString& thumbnailUrl,
                   const QString& previewUrl,
                   int width,
                   int height,
                   std::optional<int> durationMs,
                   int layerCount,
                   const QList<EditableField>& editableFields,
                   int usageCount,
                   bool isPremium,
                   int version,
                   const QDateTime& createdAt,
                   const QDateTime& updatedAt,
                   const std::optional<QDateTime>& publishedAt,
                   const QStringList& tags,
                   const QString& categoryName = {},
                   const QString& creatorName = {},
                   const QString& creatorAvatarUrl = {})
        : m_id(id)
        , m_name(name)
        , m_description(description)
        , m_type(type)
        , m_categoryId(categoryId)
        , m_creatorId(creatorId)
        , m_status(status)
        , m_thumbnailUrl(thumbnailUrl)
        , m_previewUrl(previewUrl)
        , m_width(width)
        , m_height(height)
        , m_durationMs(durationMs)
        , m_layerCount(layerCount)
        , m_editableFields(editableFields)
        , m_usageCount(usageCount)
        , m_isPremium(isPremium)
        , m_version(version)
        , m_createdAt(createdAt)
        , m_updatedAt(updatedAt)
        , m_publishedAt(publishedAt)
        , m_tags(tags)
        , m_categoryName(categoryName)
        , m_creatorName(creatorName)
        , m_creatorAvatarUrl(creatorAvatarUrl) {}

    // Accessors
    const QString& id() const { return m_id; }
    const QString& name() const { return m_name; }
    const QString& description() const { return m_description; }
    TemplateType type() const { return m_type; }
    const QString& categoryId() const { return m_categoryId; }
    const QString& creatorId() const { return m_creatorId; }
    TemplateStatus status() const { return m_status; }
    const QString& thumbnailUrl() const { return m_thumbnailUrl; }
    const QString& previewUrl() const { return m_previewUrl; }
    int width() const { return m_width; }
    int height() const { return m_height; }
    std::optional<int> durationMs() const { return m_durationMs; }
    int layerCount() const { return m_layerCount; }
    const QList<EditableField>& editableFields() const { return m_editableFields; }
    int usageCount() const { return m_usageCount; }
    bool isPremium() const { return m_isPremium; }
    int version() const { return m_version; }
    const QDateTime& createdAt() const { return m_createdAt; }
    const QDateTime& updatedAt() const { return m_updatedAt; }
    std::optional<QDateTime> publishedAt() const { return m_publishedAt; }
    const QStringList& tags() const { return m_tags; }
    const QString& categoryName() const { return m_categoryName; }
    const QString& creatorName() const { return m_creatorName; }
    const QString& creatorAvatarUrl() const { return m_creatorAvatarUrl; }

    // Derived
    bool isVideo() const { return m_type == TemplateType::Video; }
    bool isImage() const { return m_type == TemplateType::Image; }
    QString dimensions() const { return QStringLiteral("%1x%2").arg(m_width).arg(m_height); }
    std::optional<qint64> durationSeconds() const {
        return m_durationMs.has_value() ? std::optional<qint64>(*m_durationMs / 1000) : std::nullopt;
    }

    int typeInt() const { return static_cast<int>(m_type); }
    int statusInt() const { return static_cast<int>(m_status); }

    bool isValid() const { return !m_id.isEmpty(); }

    bool operator==(const TemplateEntity& other) const { return m_id == other.m_id; }
    bool operator!=(const TemplateEntity& other) const { return !(*this == other); }

private:
    QString m_id;
    QString m_name;
    QString m_description;
    TemplateType m_type = TemplateType::Image;
    QString m_categoryId;
    QString m_creatorId;
    TemplateStatus m_status = TemplateStatus::Published;
    QString m_thumbnailUrl;
    QString m_previewUrl;
    int m_width = 1080;
    int m_height = 1920;
    std::optional<int> m_durationMs;
    int m_layerCount = 0;
    QList<EditableField> m_editableFields;
    int m_usageCount = 0;
    bool m_isPremium = false;
    int m_version = 1;
    QDateTime m_createdAt;
    QDateTime m_updatedAt;
    std::optional<QDateTime> m_publishedAt;
    QStringList m_tags;
    QString m_categoryName;
    QString m_creatorName;
    QString m_creatorAvatarUrl;
};

} // namespace gopost::template_browser

Q_DECLARE_METATYPE(gopost::template_browser::EditableField)
Q_DECLARE_METATYPE(gopost::template_browser::TemplateEntity)
