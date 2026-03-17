#include "template_browser/presentation/models/template_search_notifier.h"

#include <QSet>

namespace gopost::template_browser {

TemplateSearchNotifier::TemplateSearchNotifier(SearchTemplatesUseCase* searchTemplates, QObject* parent)
    : QObject(parent)
    , m_searchTemplates(searchTemplates)
{
    m_debounceTimer.setSingleShot(true);
    m_debounceTimer.setInterval(300);
    connect(&m_debounceTimer, &QTimer::timeout, this, [this]() {
        executeSearch(m_query);
    });
}

void TemplateSearchNotifier::search(const QString& query)
{
    m_query = query;
    emit queryChanged();

    m_debounceTimer.stop();

    if (query.isEmpty()) {
        clear();
        return;
    }

    m_debounceTimer.start();
}

void TemplateSearchNotifier::executeSearch(const QString& query)
{
    if (!m_searchTemplates) return;

    m_isSearching = true;
    m_error.clear();
    emit isSearchingChanged();
    emit errorChanged();

    (*m_searchTemplates)(query, m_filter, [this, query](auto result) {
        if (m_query != query) return; // stale result

        m_isSearching = false;
        if (auto* paginated = std::get_if<PaginatedResult<TemplateEntity>>(&result)) {
            m_results = paginated->items();
            m_hasMore = paginated->hasMore();
            m_nextCursor = paginated->nextCursor();
            emit resultsChanged();
            emit hasMoreChanged();
        } else {
            const auto& failure = std::get<core::Failure>(result);
            m_error = failure.message();
            emit errorChanged();
        }
        emit isSearchingChanged();
    });
}

void TemplateSearchNotifier::loadMore()
{
    if (m_isSearching || !m_hasMore || !m_nextCursor.has_value() || !m_searchTemplates) return;

    m_isSearching = true;
    emit isSearchingChanged();

    auto filter = m_filter.copyWith(
        std::nullopt, std::nullopt, std::nullopt, std::nullopt,
        std::nullopt, std::nullopt, m_nextCursor);

    (*m_searchTemplates)(m_query, filter, [this](auto result) {
        m_isSearching = false;
        if (auto* paginated = std::get_if<PaginatedResult<TemplateEntity>>(&result)) {
            QSet<QString> existingIds;
            for (const auto& t : m_results) existingIds.insert(t.id());

            for (const auto& item : paginated->items()) {
                if (!existingIds.contains(item.id())) {
                    m_results.append(item);
                }
            }
            m_hasMore = paginated->hasMore();
            m_nextCursor = paginated->nextCursor();
            emit resultsChanged();
            emit hasMoreChanged();
        } else {
            const auto& failure = std::get<core::Failure>(result);
            m_error = failure.message();
            emit errorChanged();
        }
        emit isSearchingChanged();
    });
}

void TemplateSearchNotifier::updateFilter(const TemplateFilter& filter)
{
    m_filter = filter;
    if (!m_query.isEmpty()) {
        executeSearch(m_query);
    }
}

void TemplateSearchNotifier::clear()
{
    m_debounceTimer.stop();
    m_results.clear();
    m_isSearching = false;
    m_error.clear();
    m_query.clear();
    m_hasMore = false;
    m_nextCursor = std::nullopt;
    m_filter = TemplateFilter();

    emit resultsChanged();
    emit isSearchingChanged();
    emit errorChanged();
    emit queryChanged();
    emit hasMoreChanged();
}

} // namespace gopost::template_browser
