#pragma once

#include <QList>
#include <QString>
#include <optional>

namespace gopost::template_browser {

template <typename T>
class PaginatedResult {
public:
    PaginatedResult() = default;

    PaginatedResult(const QList<T>& items,
                    const std::optional<QString>& nextCursor = std::nullopt,
                    bool hasMore = false,
                    int totalCount = 0)
        : m_items(items)
        , m_nextCursor(nextCursor)
        , m_hasMore(hasMore)
        , m_totalCount(totalCount) {}

    const QList<T>& items() const { return m_items; }
    std::optional<QString> nextCursor() const { return m_nextCursor; }
    bool hasMore() const { return m_hasMore; }
    int totalCount() const { return m_totalCount; }

    bool operator==(const PaginatedResult& other) const {
        return m_items == other.m_items
            && m_nextCursor == other.m_nextCursor
            && m_hasMore == other.m_hasMore
            && m_totalCount == other.m_totalCount;
    }
    bool operator!=(const PaginatedResult& other) const { return !(*this == other); }

private:
    QList<T> m_items;
    std::optional<QString> m_nextCursor;
    bool m_hasMore = false;
    int m_totalCount = 0;
};

} // namespace gopost::template_browser
