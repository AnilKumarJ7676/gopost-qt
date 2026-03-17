#pragma once

#include <QList>
#include <functional>

#include "template_browser/domain/entities/template_entity.h"
#include "template_browser/domain/repositories/template_repository.h"

namespace gopost::template_browser {

class GetFeaturedTemplatesUseCase {
public:
    explicit GetFeaturedTemplatesUseCase(TemplateReadRepository* repository)
        : m_repository(repository) {}

    void operator()(std::function<void(TemplateResult<QList<TemplateEntity>>)> callback) const;

private:
    TemplateReadRepository* m_repository;
};

class GetTrendingTemplatesUseCase {
public:
    explicit GetTrendingTemplatesUseCase(TemplateReadRepository* repository)
        : m_repository(repository) {}

    void operator()(int limit, std::function<void(TemplateResult<QList<TemplateEntity>>)> callback) const;

private:
    TemplateReadRepository* m_repository;
};

class GetRecentTemplatesUseCase {
public:
    explicit GetRecentTemplatesUseCase(TemplateReadRepository* repository)
        : m_repository(repository) {}

    void operator()(int limit, std::function<void(TemplateResult<QList<TemplateEntity>>)> callback) const;

private:
    TemplateReadRepository* m_repository;
};

} // namespace gopost::template_browser
