#include "template_browser/presentation/models/template_list_notifier.h"

#include <QSet>

namespace gopost::template_browser {

TemplateListNotifier::TemplateListNotifier(GetTemplatesUseCase* getTemplates, QObject* parent)
    : QObject(parent)
    , m_getTemplates(getTemplates)
{
}

void TemplateListNotifier::loadTemplates()
{
    doLoad(m_filter.copyWith(
        std::nullopt, std::nullopt, std::nullopt, std::nullopt,
        std::nullopt, std::nullopt, std::nullopt,
        false, false, false, false, true /*clearCursor*/));
}

void TemplateListNotifier::loadMore()
{
    if (m_isLoadingMore || !m_hasMore || !m_nextCursor.has_value() || !m_getTemplates) return;

    m_isLoadingMore = true;
    emit isLoadingMoreChanged();

    auto filter = m_filter.copyWith(
        std::nullopt, std::nullopt, std::nullopt, std::nullopt,
        std::nullopt, std::nullopt, m_nextCursor);

    (*m_getTemplates)(filter, [this](auto result) {
        if (auto* paginated = std::get_if<PaginatedResult<TemplateEntity>>(&result)) {
            QSet<QString> existingIds;
            for (const auto& t : m_templates) existingIds.insert(t.id());

            for (const auto& item : paginated->items()) {
                if (!existingIds.contains(item.id())) {
                    m_templates.append(item);
                }
            }
            m_nextCursor = paginated->nextCursor();
            m_hasMore = paginated->hasMore();

            m_isLoadingMore = false;
            emit templatesChanged();
            emit isLoadingMoreChanged();
            emit hasMoreChanged();
        } else {
            const auto& failure = std::get<core::Failure>(result);
            m_error = failure.message();
            m_isLoadingMore = false;
            emit isLoadingMoreChanged();
            emit errorChanged();
        }
    });
}

void TemplateListNotifier::updateFilter(const TemplateFilter& filter)
{
    m_filter = filter;
    doLoad(filter.copyWith(
        std::nullopt, std::nullopt, std::nullopt, std::nullopt,
        std::nullopt, std::nullopt, std::nullopt,
        false, false, false, false, true /*clearCursor*/));
}

void TemplateListNotifier::refresh()
{
    doLoad(m_filter.copyWith(
        std::nullopt, std::nullopt, std::nullopt, std::nullopt,
        std::nullopt, std::nullopt, std::nullopt,
        false, false, false, false, true /*clearCursor*/));
}

void TemplateListNotifier::doLoad(const TemplateFilter& filter)
{
    if (!m_getTemplates) return;

    m_filter = filter;
    m_isLoading = true;
    m_error.clear();
    emit isLoadingChanged();
    emit errorChanged();

    (*m_getTemplates)(filter, [this](auto result) {
        m_isLoading = false;
        if (auto* paginated = std::get_if<PaginatedResult<TemplateEntity>>(&result)) {
            m_templates = paginated->items();
            m_nextCursor = paginated->nextCursor();
            m_hasMore = paginated->hasMore();
            emit templatesChanged();
            emit hasMoreChanged();
        } else {
            const auto& failure = std::get<core::Failure>(result);
            m_error = failure.message();
            emit errorChanged();
        }
        emit isLoadingChanged();
    });
}

} // namespace gopost::template_browser
