#include "template_browser/domain/usecases/search_templates_usecase.h"

namespace gopost::template_browser {

void SearchTemplatesUseCase::operator()(
    const QString& query,
    const TemplateFilter& filter,
    std::function<void(TemplateResult<PaginatedResult<TemplateEntity>>)> callback) const
{
    auto searchFilter = filter.copyWith(
        query,                   // query
        std::nullopt,            // type
        std::nullopt,            // categoryId
        std::nullopt,            // isPremium
        std::nullopt,            // sortBy
        std::nullopt,            // limit
        std::nullopt             // cursor
    );
    m_repository->getTemplates(searchFilter, std::move(callback));
}

} // namespace gopost::template_browser
