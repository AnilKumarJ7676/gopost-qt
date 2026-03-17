#pragma once

#include <QByteArray>
#include <QList>
#include <QString>
#include <functional>
#include <optional>
#include <variant>

#include "core/error/failures.h"
#include "template_browser/domain/entities/category_entity.h"
#include "template_browser/domain/entities/paginated_result.h"
#include "template_browser/domain/entities/template_access.h"
#include "template_browser/domain/entities/template_entity.h"
#include "template_browser/domain/entities/template_filter.h"

namespace gopost::template_browser {

template <typename T>
using TemplateResult = std::variant<T, core::Failure>;

/// Read-only operations for browsing and searching templates.
/// ISP: Separated from write/admin operations.
class TemplateReadRepository {
public:
    virtual ~TemplateReadRepository() = default;

    virtual void getTemplates(
        const TemplateFilter& filter,
        std::function<void(TemplateResult<PaginatedResult<TemplateEntity>>)> callback) = 0;

    virtual void getTemplateById(
        const QString& id,
        std::function<void(TemplateResult<TemplateEntity>)> callback) = 0;

    virtual void getTemplatesByCategory(
        const QString& categoryId,
        const TemplateFilter& filter,
        std::function<void(TemplateResult<PaginatedResult<TemplateEntity>>)> callback) = 0;

    virtual void getFeaturedTemplates(
        std::function<void(TemplateResult<QList<TemplateEntity>>)> callback) = 0;

    virtual void getTrendingTemplates(
        int limit,
        std::function<void(TemplateResult<QList<TemplateEntity>>)> callback) = 0;

    virtual void getRecentTemplates(
        int limit,
        std::function<void(TemplateResult<QList<TemplateEntity>>)> callback) = 0;
};

/// Category-specific read operations.
class CategoryRepository {
public:
    virtual ~CategoryRepository() = default;

    virtual void getCategories(
        std::function<void(TemplateResult<QList<CategoryEntity>>)> callback) = 0;

    virtual void getCategoryById(
        const QString& id,
        std::function<void(TemplateResult<CategoryEntity>)> callback) = 0;
};

/// Template access/download operations requiring authentication.
class TemplateAccessRepository {
public:
    virtual ~TemplateAccessRepository() = default;

    virtual void requestAccess(
        const QString& templateId,
        std::function<void(TemplateResult<TemplateAccess>)> callback) = 0;

    virtual void downloadTemplate(
        const QString& url,
        const QString& savePath,
        std::function<void(int received, int total)> onProgress,
        std::function<void(std::optional<core::Failure>)> callback) = 0;

    virtual void downloadTemplateToBytes(
        const QString& url,
        std::function<void(int received, int total)> onProgress,
        std::function<void(TemplateResult<QByteArray>)> callback) = 0;
};

/// Composite interface for convenience when a single dependency is preferred.
class TemplateRepository
    : public TemplateReadRepository
    , public CategoryRepository
    , public TemplateAccessRepository {
public:
    ~TemplateRepository() override = default;
};

} // namespace gopost::template_browser
