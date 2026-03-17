#include "template_browser/domain/usecases/request_template_access_usecase.h"

namespace gopost::template_browser {

void RequestTemplateAccessUseCase::operator()(
    const QString& templateId,
    std::function<void(TemplateResult<TemplateAccess>)> callback) const
{
    m_repository->requestAccess(templateId, std::move(callback));
}

} // namespace gopost::template_browser
