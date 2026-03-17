#include "template_browser/domain/usecases/get_categories_usecase.h"

namespace gopost::template_browser {

void GetCategoriesUseCase::operator()(
    std::function<void(TemplateResult<QList<CategoryEntity>>)> callback) const
{
    m_repository->getCategories(std::move(callback));
}

} // namespace gopost::template_browser
