#include "template_browser/data/repositories/template_repository_impl.h"

namespace gopost::template_browser {

TemplateRepositoryImpl::TemplateRepositoryImpl(TemplateRemoteDataSource* remote)
    : m_remote(remote)
{
}

void TemplateRepositoryImpl::getTemplates(
    const TemplateFilter& filter,
    std::function<void(TemplateResult<PaginatedResult<TemplateEntity>>)> callback)
{
    m_remote->getTemplates(filter, [cb = std::move(callback)](auto result) {
        if (auto* resp = std::get_if<PaginatedTemplateResponse>(&result)) {
            QList<TemplateEntity> entities;
            entities.reserve(resp->templates.size());
            for (const auto& m : resp->templates) {
                entities.append(m.toEntity());
            }
            cb(PaginatedResult<TemplateEntity>(entities, resp->nextCursor, resp->hasMore, resp->total));
        } else {
            cb(core::ServerFailure(std::get<QString>(result)));
        }
    });
}

void TemplateRepositoryImpl::getTemplateById(
    const QString& id,
    std::function<void(TemplateResult<TemplateEntity>)> callback)
{
    m_remote->getTemplateById(id, [cb = std::move(callback)](auto result) {
        if (auto* model = std::get_if<TemplateModel>(&result)) {
            cb(model->toEntity());
        } else {
            cb(core::ServerFailure(std::get<QString>(result)));
        }
    });
}

void TemplateRepositoryImpl::getTemplatesByCategory(
    const QString& categoryId,
    const TemplateFilter& filter,
    std::function<void(TemplateResult<PaginatedResult<TemplateEntity>>)> callback)
{
    m_remote->getTemplatesByCategory(categoryId, filter, [cb = std::move(callback)](auto result) {
        if (auto* resp = std::get_if<PaginatedTemplateResponse>(&result)) {
            QList<TemplateEntity> entities;
            entities.reserve(resp->templates.size());
            for (const auto& m : resp->templates) {
                entities.append(m.toEntity());
            }
            cb(PaginatedResult<TemplateEntity>(entities, resp->nextCursor, resp->hasMore, resp->total));
        } else {
            cb(core::ServerFailure(std::get<QString>(result)));
        }
    });
}

void TemplateRepositoryImpl::getFeaturedTemplates(
    std::function<void(TemplateResult<QList<TemplateEntity>>)> callback)
{
    TemplateFilter filter;
    filter = filter.copyWith(
        std::nullopt, std::nullopt, std::nullopt, std::nullopt,
        TemplateSortBy::Popular, 5);
    m_remote->getTemplates(filter, [cb = std::move(callback)](auto result) {
        if (auto* resp = std::get_if<PaginatedTemplateResponse>(&result)) {
            QList<TemplateEntity> entities;
            entities.reserve(resp->templates.size());
            for (const auto& m : resp->templates) {
                entities.append(m.toEntity());
            }
            cb(entities);
        } else {
            cb(core::ServerFailure(std::get<QString>(result)));
        }
    });
}

void TemplateRepositoryImpl::getTrendingTemplates(
    int limit,
    std::function<void(TemplateResult<QList<TemplateEntity>>)> callback)
{
    TemplateFilter filter;
    filter = filter.copyWith(
        std::nullopt, std::nullopt, std::nullopt, std::nullopt,
        TemplateSortBy::Trending, limit);
    m_remote->getTemplates(filter, [cb = std::move(callback)](auto result) {
        if (auto* resp = std::get_if<PaginatedTemplateResponse>(&result)) {
            QList<TemplateEntity> entities;
            entities.reserve(resp->templates.size());
            for (const auto& m : resp->templates) {
                entities.append(m.toEntity());
            }
            cb(entities);
        } else {
            cb(core::ServerFailure(std::get<QString>(result)));
        }
    });
}

void TemplateRepositoryImpl::getRecentTemplates(
    int limit,
    std::function<void(TemplateResult<QList<TemplateEntity>>)> callback)
{
    TemplateFilter filter;
    filter = filter.copyWith(
        std::nullopt, std::nullopt, std::nullopt, std::nullopt,
        TemplateSortBy::Newest, limit);
    m_remote->getTemplates(filter, [cb = std::move(callback)](auto result) {
        if (auto* resp = std::get_if<PaginatedTemplateResponse>(&result)) {
            QList<TemplateEntity> entities;
            entities.reserve(resp->templates.size());
            for (const auto& m : resp->templates) {
                entities.append(m.toEntity());
            }
            cb(entities);
        } else {
            cb(core::ServerFailure(std::get<QString>(result)));
        }
    });
}

void TemplateRepositoryImpl::getCategories(
    std::function<void(TemplateResult<QList<CategoryEntity>>)> callback)
{
    m_remote->getCategories([cb = std::move(callback)](auto result) {
        if (auto* models = std::get_if<QList<CategoryModel>>(&result)) {
            QList<CategoryEntity> entities;
            entities.reserve(models->size());
            for (const auto& m : *models) {
                entities.append(m.toEntity());
            }
            cb(entities);
        } else {
            cb(core::ServerFailure(std::get<QString>(result)));
        }
    });
}

void TemplateRepositoryImpl::getCategoryById(
    const QString& id,
    std::function<void(TemplateResult<CategoryEntity>)> callback)
{
    m_remote->getCategoryById(id, [cb = std::move(callback)](auto result) {
        if (auto* model = std::get_if<CategoryModel>(&result)) {
            cb(model->toEntity());
        } else {
            cb(core::ServerFailure(std::get<QString>(result)));
        }
    });
}

void TemplateRepositoryImpl::requestAccess(
    const QString& templateId,
    std::function<void(TemplateResult<TemplateAccess>)> callback)
{
    m_remote->requestAccess(templateId, [cb = std::move(callback)](auto result) {
        if (auto* model = std::get_if<TemplateAccessModel>(&result)) {
            cb(model->toEntity());
        } else {
            cb(core::ServerFailure(std::get<QString>(result)));
        }
    });
}

void TemplateRepositoryImpl::downloadTemplate(
    const QString& url,
    const QString& savePath,
    std::function<void(int received, int total)> onProgress,
    std::function<void(std::optional<core::Failure>)> callback)
{
    m_remote->downloadFile(url, savePath, std::move(onProgress),
        [cb = std::move(callback)](auto errorOpt) {
            if (errorOpt.has_value()) {
                cb(core::ServerFailure(*errorOpt));
            } else {
                cb(std::nullopt);
            }
        });
}

void TemplateRepositoryImpl::downloadTemplateToBytes(
    const QString& url,
    std::function<void(int received, int total)> onProgress,
    std::function<void(TemplateResult<QByteArray>)> callback)
{
    m_remote->downloadToBytes(url, std::move(onProgress),
        [cb = std::move(callback)](auto result) {
            if (auto* bytes = std::get_if<QByteArray>(&result)) {
                cb(*bytes);
            } else {
                cb(core::ServerFailure(std::get<QString>(result)));
            }
        });
}

} // namespace gopost::template_browser
