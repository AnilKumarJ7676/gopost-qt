#pragma once

#include <QObject>
#include <QString>
#include <QMap>
#include <optional>

#include "template_browser/domain/entities/template_entity.h"

namespace gopost::template_browser {

enum class TemplateSortBy { Popular, Newest, Trending };

class TemplateFilter {
    Q_GADGET
    Q_PROPERTY(QString query MEMBER m_query)
    Q_PROPERTY(QString categoryId MEMBER m_categoryId)
    Q_PROPERTY(int limit MEMBER m_limit)

public:
    TemplateFilter() = default;

    TemplateFilter(const std::optional<QString>& query,
                   const std::optional<TemplateType>& type,
                   const std::optional<QString>& categoryId,
                   const std::optional<bool>& isPremium,
                   TemplateSortBy sortBy = TemplateSortBy::Popular,
                   int limit = 20,
                   const std::optional<QString>& cursor = std::nullopt)
        : m_query(query.value_or(QString{}))
        , m_type(type)
        , m_categoryId(categoryId.value_or(QString{}))
        , m_isPremium(isPremium)
        , m_sortBy(sortBy)
        , m_limit(limit)
        , m_cursor(cursor) {}

    // Accessors
    QString query() const { return m_query; }
    std::optional<TemplateType> type() const { return m_type; }
    QString categoryId() const { return m_categoryId; }
    std::optional<bool> isPremium() const { return m_isPremium; }
    TemplateSortBy sortBy() const { return m_sortBy; }
    int limit() const { return m_limit; }
    std::optional<QString> cursor() const { return m_cursor; }

    // copyWith equivalent
    TemplateFilter copyWith(
        const std::optional<QString>& query = std::nullopt,
        const std::optional<TemplateType>& type = std::nullopt,
        const std::optional<QString>& categoryId = std::nullopt,
        const std::optional<bool>& isPremium = std::nullopt,
        const std::optional<TemplateSortBy>& sortBy = std::nullopt,
        const std::optional<int>& limit = std::nullopt,
        const std::optional<QString>& cursor = std::nullopt,
        bool clearQuery = false,
        bool clearType = false,
        bool clearCategory = false,
        bool clearPremium = false,
        bool clearCursor = false) const
    {
        TemplateFilter f;
        f.m_query = clearQuery ? QString{} : (query.has_value() ? *query : m_query);
        f.m_type = clearType ? std::nullopt : (type.has_value() ? type : m_type);
        f.m_categoryId = clearCategory ? QString{} : (categoryId.has_value() ? *categoryId : m_categoryId);
        f.m_isPremium = clearPremium ? std::nullopt : (isPremium.has_value() ? isPremium : m_isPremium);
        f.m_sortBy = sortBy.value_or(m_sortBy);
        f.m_limit = limit.value_or(m_limit);
        f.m_cursor = clearCursor ? std::nullopt : (cursor.has_value() ? cursor : m_cursor);
        return f;
    }

    QMap<QString, QString> toQueryParameters() const {
        QMap<QString, QString> params;
        params[QStringLiteral("limit")] = QString::number(m_limit);
        params[QStringLiteral("sort")] = sortByName();

        if (!m_query.isEmpty())
            params[QStringLiteral("q")] = m_query;
        if (m_type.has_value())
            params[QStringLiteral("type")] = (*m_type == TemplateType::Video)
                ? QStringLiteral("video") : QStringLiteral("image");
        if (!m_categoryId.isEmpty())
            params[QStringLiteral("category_id")] = m_categoryId;
        if (m_isPremium.has_value())
            params[QStringLiteral("is_premium")] = *m_isPremium
                ? QStringLiteral("true") : QStringLiteral("false");
        if (m_cursor.has_value())
            params[QStringLiteral("cursor")] = *m_cursor;

        return params;
    }

    bool operator==(const TemplateFilter& other) const {
        return m_query == other.m_query
            && m_type == other.m_type
            && m_categoryId == other.m_categoryId
            && m_isPremium == other.m_isPremium
            && m_sortBy == other.m_sortBy
            && m_limit == other.m_limit
            && m_cursor == other.m_cursor;
    }
    bool operator!=(const TemplateFilter& other) const { return !(*this == other); }

private:
    QString sortByName() const {
        switch (m_sortBy) {
        case TemplateSortBy::Popular: return QStringLiteral("popular");
        case TemplateSortBy::Newest: return QStringLiteral("newest");
        case TemplateSortBy::Trending: return QStringLiteral("trending");
        }
        return QStringLiteral("popular");
    }

    QString m_query;
    std::optional<TemplateType> m_type;
    QString m_categoryId;
    std::optional<bool> m_isPremium;
    TemplateSortBy m_sortBy = TemplateSortBy::Popular;
    int m_limit = 20;
    std::optional<QString> m_cursor;
};

} // namespace gopost::template_browser

Q_DECLARE_METATYPE(gopost::template_browser::TemplateFilter)
