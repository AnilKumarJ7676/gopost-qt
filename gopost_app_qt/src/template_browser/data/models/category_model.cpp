#include "template_browser/data/models/category_model.h"

namespace gopost::template_browser {

CategoryModel CategoryModel::fromJson(const QJsonObject& json)
{
    CategoryModel model;
    model.m_id = json[QStringLiteral("id")].toString();
    model.m_name = json[QStringLiteral("name")].toString();
    model.m_slug = json[QStringLiteral("slug")].toString();
    model.m_description = json[QStringLiteral("description")].toString();
    model.m_iconUrl = json[QStringLiteral("icon_url")].toString();
    model.m_sortOrder = json[QStringLiteral("sort_order")].toInt(0);
    model.m_isActive = json[QStringLiteral("is_active")].toBool(true);
    return model;
}

CategoryEntity CategoryModel::toEntity() const
{
    return CategoryEntity(
        m_id, m_name, m_slug, m_description, m_iconUrl, m_sortOrder, m_isActive
    );
}

} // namespace gopost::template_browser
