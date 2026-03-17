#pragma once

#include "template_browser/domain/repositories/template_repository.h"
#include "template_browser/data/datasources/template_remote_datasource.h"

namespace gopost::template_browser {

/// OCP: Extend behavior by adding new datasources (cache) without modifying
/// existing code. LSP: Substitutable for any TemplateRepository consumer.
class TemplateRepositoryImpl : public TemplateRepository {
public:
    explicit TemplateRepositoryImpl(TemplateRemoteDataSource* remote);
    ~TemplateRepositoryImpl() override = default;

    // TemplateReadRepository
    void getTemplates(
        const TemplateFilter& filter,
        std::function<void(TemplateResult<PaginatedResult<TemplateEntity>>)> callback) override;

    void getTemplateById(
        const QString& id,
        std::function<void(TemplateResult<TemplateEntity>)> callback) override;

    void getTemplatesByCategory(
        const QString& categoryId,
        const TemplateFilter& filter,
        std::function<void(TemplateResult<PaginatedResult<TemplateEntity>>)> callback) override;

    void getFeaturedTemplates(
        std::function<void(TemplateResult<QList<TemplateEntity>>)> callback) override;

    void getTrendingTemplates(
        int limit,
        std::function<void(TemplateResult<QList<TemplateEntity>>)> callback) override;

    void getRecentTemplates(
        int limit,
        std::function<void(TemplateResult<QList<TemplateEntity>>)> callback) override;

    // CategoryRepository
    void getCategories(
        std::function<void(TemplateResult<QList<CategoryEntity>>)> callback) override;

    void getCategoryById(
        const QString& id,
        std::function<void(TemplateResult<CategoryEntity>)> callback) override;

    // TemplateAccessRepository
    void requestAccess(
        const QString& templateId,
        std::function<void(TemplateResult<TemplateAccess>)> callback) override;

    void downloadTemplate(
        const QString& url,
        const QString& savePath,
        std::function<void(int received, int total)> onProgress,
        std::function<void(std::optional<core::Failure>)> callback) override;

    void downloadTemplateToBytes(
        const QString& url,
        std::function<void(int received, int total)> onProgress,
        std::function<void(TemplateResult<QByteArray>)> callback) override;

private:
    TemplateRemoteDataSource* m_remote;
};

} // namespace gopost::template_browser
