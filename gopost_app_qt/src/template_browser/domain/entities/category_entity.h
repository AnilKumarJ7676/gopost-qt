#pragma once

#include <QObject>
#include <QString>

namespace gopost::template_browser {

class CategoryEntity {
    Q_GADGET
    Q_PROPERTY(QString id MEMBER m_id)
    Q_PROPERTY(QString name MEMBER m_name)
    Q_PROPERTY(QString slug MEMBER m_slug)
    Q_PROPERTY(QString description MEMBER m_description)
    Q_PROPERTY(QString iconUrl MEMBER m_iconUrl)
    Q_PROPERTY(int sortOrder MEMBER m_sortOrder)
    Q_PROPERTY(bool isActive MEMBER m_isActive)

public:
    CategoryEntity() = default;

    CategoryEntity(const QString& id,
                   const QString& name,
                   const QString& slug,
                   const QString& description = {},
                   const QString& iconUrl = {},
                   int sortOrder = 0,
                   bool isActive = true)
        : m_id(id)
        , m_name(name)
        , m_slug(slug)
        , m_description(description)
        , m_iconUrl(iconUrl)
        , m_sortOrder(sortOrder)
        , m_isActive(isActive) {}

    const QString& id() const { return m_id; }
    const QString& name() const { return m_name; }
    const QString& slug() const { return m_slug; }
    const QString& description() const { return m_description; }
    const QString& iconUrl() const { return m_iconUrl; }
    int sortOrder() const { return m_sortOrder; }
    bool isActive() const { return m_isActive; }

    bool isValid() const { return !m_id.isEmpty(); }

    bool operator==(const CategoryEntity& other) const {
        return m_id == other.m_id && m_slug == other.m_slug;
    }
    bool operator!=(const CategoryEntity& other) const { return !(*this == other); }

private:
    QString m_id;
    QString m_name;
    QString m_slug;
    QString m_description;
    QString m_iconUrl;
    int m_sortOrder = 0;
    bool m_isActive = true;
};

} // namespace gopost::template_browser

Q_DECLARE_METATYPE(gopost::template_browser::CategoryEntity)
