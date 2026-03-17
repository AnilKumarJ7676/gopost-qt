#include "template_browser/domain/usecases/get_featured_templates_usecase.h"

namespace gopost::template_browser {

void GetFeaturedTemplatesUseCase::operator()(
    std::function<void(TemplateResult<QList<TemplateEntity>>)> callback) const
{
    m_repository->getFeaturedTemplates(std::move(callback));
}

void GetTrendingTemplatesUseCase::operator()(
    int limit,
    std::function<void(TemplateResult<QList<TemplateEntity>>)> callback) const
{
    m_repository->getTrendingTemplates(limit, std::move(callback));
}

void GetRecentTemplatesUseCase::operator()(
    int limit,
    std::function<void(TemplateResult<QList<TemplateEntity>>)> callback) const
{
    m_repository->getRecentTemplates(limit, std::move(callback));
}

} // namespace gopost::template_browser
