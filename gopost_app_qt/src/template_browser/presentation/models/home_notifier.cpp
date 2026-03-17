#include "template_browser/presentation/models/home_notifier.h"

namespace gopost::template_browser {

HomeNotifier::HomeNotifier(
    GetFeaturedTemplatesUseCase* getFeatured,
    GetTrendingTemplatesUseCase* getTrending,
    GetRecentTemplatesUseCase* getRecent,
    GetCategoriesUseCase* getCategories,
    QObject* parent)
    : QObject(parent)
    , m_getFeatured(getFeatured)
    , m_getTrending(getTrending)
    , m_getRecent(getRecent)
    , m_getCategories(getCategories)
{
}

void HomeNotifier::load()
{
    if (!m_getFeatured || !m_getTrending || !m_getRecent || !m_getCategories) return;

    m_isLoading = true;
    m_error.clear();
    m_pendingRequests = 4;
    emit isLoadingChanged();
    emit errorChanged();

    // Featured
    (*m_getFeatured)([this](auto result) {
        if (auto* list = std::get_if<QList<TemplateEntity>>(&result)) {
            m_featured = *list;
        } else {
            const auto& failure = std::get<core::Failure>(result);
            if (m_error.isEmpty()) m_error = failure.message();
        }
        checkAllLoaded();
    });

    // Trending
    (*m_getTrending)(10, [this](auto result) {
        if (auto* list = std::get_if<QList<TemplateEntity>>(&result)) {
            m_trending = *list;
        } else {
            const auto& failure = std::get<core::Failure>(result);
            if (m_error.isEmpty()) m_error = failure.message();
        }
        checkAllLoaded();
    });

    // Recent
    (*m_getRecent)(10, [this](auto result) {
        if (auto* list = std::get_if<QList<TemplateEntity>>(&result)) {
            m_recent = *list;
        } else {
            const auto& failure = std::get<core::Failure>(result);
            if (m_error.isEmpty()) m_error = failure.message();
        }
        checkAllLoaded();
    });

    // Categories
    (*m_getCategories)([this](auto result) {
        if (auto* list = std::get_if<QList<CategoryEntity>>(&result)) {
            m_categories = *list;
        } else {
            const auto& failure = std::get<core::Failure>(result);
            if (m_error.isEmpty()) m_error = failure.message();
        }
        checkAllLoaded();
    });
}

void HomeNotifier::refresh()
{
    load();
}

void HomeNotifier::checkAllLoaded()
{
    --m_pendingRequests;
    if (m_pendingRequests <= 0) {
        m_isLoading = false;
        emit dataChanged();
        emit isLoadingChanged();
        if (!m_error.isEmpty()) emit errorChanged();
    }
}

} // namespace gopost::template_browser
