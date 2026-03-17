#pragma once

#include <QString>
#include <functional>

#include "template_browser/domain/entities/template_access.h"
#include "template_browser/domain/repositories/template_repository.h"

namespace gopost::template_browser {

class RequestTemplateAccessUseCase {
public:
    explicit RequestTemplateAccessUseCase(TemplateAccessRepository* repository)
        : m_repository(repository) {}

    void operator()(const QString& templateId,
                    std::function<void(TemplateResult<TemplateAccess>)> callback) const;

private:
    TemplateAccessRepository* m_repository;
};

} // namespace gopost::template_browser
