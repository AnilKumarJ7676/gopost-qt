#pragma once

#include <QString>
#include <functional>

#include "template_browser/domain/entities/template_entity.h"
#include "template_browser/domain/repositories/template_repository.h"

namespace gopost::template_browser {

class GetTemplateDetailUseCase {
public:
    explicit GetTemplateDetailUseCase(TemplateReadRepository* repository)
        : m_repository(repository) {}

    void operator()(const QString& id,
                    std::function<void(TemplateResult<TemplateEntity>)> callback) const;

private:
    TemplateReadRepository* m_repository;
};

} // namespace gopost::template_browser
