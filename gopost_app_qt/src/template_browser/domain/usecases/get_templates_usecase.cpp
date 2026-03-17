#include "template_browser/domain/usecases/get_templates_usecase.h"

namespace gopost::template_browser {

void GetTemplatesUseCase::operator()(
    const TemplateFilter& filter,
    std::function<void(TemplateResult<PaginatedResult<TemplateEntity>>)> callback) const
{
    m_repository->getTemplates(filter, std::move(callback));
}

} // namespace gopost::template_browser
