#include "template_browser/domain/usecases/get_template_detail_usecase.h"

namespace gopost::template_browser {

void GetTemplateDetailUseCase::operator()(
    const QString& id,
    std::function<void(TemplateResult<TemplateEntity>)> callback) const
{
    m_repository->getTemplateById(id, std::move(callback));
}

} // namespace gopost::template_browser
