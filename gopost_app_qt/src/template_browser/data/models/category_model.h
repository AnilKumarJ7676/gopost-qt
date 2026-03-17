#pragma once

#include <QJsonObject>
#include <QString>

#include "template_browser/domain/entities/category_entity.h"

namespace gopost::template_browser {

class CategoryModel {
public:
    CategoryModel() = default;

    CategoryModel(const QString& id,
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

    static CategoryModel fromJson(const QJsonObject& json);
    CategoryEntity toEntity() const;

    const QString& id() const { return m_id; }
    const QString& name() const { return m_name; }

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
