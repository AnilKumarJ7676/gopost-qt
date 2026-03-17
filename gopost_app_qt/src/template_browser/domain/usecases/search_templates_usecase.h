#pragma once

#include <QString>
#include <functional>

#include "template_browser/domain/entities/paginated_result.h"
#include "template_browser/domain/entities/template_entity.h"
#include "template_browser/domain/entities/template_filter.h"
#include "template_browser/domain/repositories/template_repository.h"

namespace gopost::template_browser {

class SearchTemplatesUseCase {
public:
    explicit SearchTemplatesUseCase(TemplateReadRepository* repository)
        : m_repository(repository) {}

    void operator()(const QString& query,
                    const TemplateFilter& filter,
                    std::function<void(TemplateResult<PaginatedResult<TemplateEntity>>)> callback) const;

private:
    TemplateReadRepository* m_repository;
};

} // namespace gopost::template_browser
