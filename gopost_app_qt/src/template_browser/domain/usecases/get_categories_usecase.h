#pragma once

#include <QList>
#include <functional>

#include "template_browser/domain/entities/category_entity.h"
#include "template_browser/domain/repositories/template_repository.h"

namespace gopost::template_browser {

class GetCategoriesUseCase {
public:
    explicit GetCategoriesUseCase(CategoryRepository* repository)
        : m_repository(repository) {}

    void operator()(std::function<void(TemplateResult<QList<CategoryEntity>>)> callback) const;

private:
    CategoryRepository* m_repository;
};

} // namespace gopost::template_browser
