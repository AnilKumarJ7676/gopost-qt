#pragma once

#include <QObject>
#include <QList>
#include <QByteArray>
#include <QString>
#include <functional>
#include <optional>

#include "template_browser/data/models/category_model.h"
#include "template_browser/data/models/template_access_model.h"
#include "template_browser/data/models/template_model.h"
#include "template_browser/domain/entities/template_filter.h"

namespace gopost::core { class HttpClient; }

namespace gopost::template_browser {

struct PaginatedTemplateResponse {
    QList<TemplateModel> templates;
    std::optional<QString> nextCursor;
    bool hasMore = false;
    int total = 0;
};

/// Abstract datasource interface for template API calls.
class TemplateRemoteDataSource {
public:
    virtual ~TemplateRemoteDataSource() = default;

    virtual void getTemplates(
        const TemplateFilter& filter,
        std::function<void(std::variant<PaginatedTemplateResponse, QString>)> callback) = 0;

    virtual void getTemplateById(
        const QString& id,
        std::function<void(std::variant<TemplateModel, QString>)> callback) = 0;

    virtual void getTemplatesByCategory(
        const QString& categoryId,
        const TemplateFilter& filter,
        std::function<void(std::variant<PaginatedTemplateResponse, QString>)> callback) = 0;

    virtual void getCategories(
        std::function<void(std::variant<QList<CategoryModel>, QString>)> callback) = 0;

    virtual void getCategoryById(
        const QString& id,
        std::function<void(std::variant<CategoryModel, QString>)> callback) = 0;

    virtual void requestAccess(
        const QString& templateId,
        std::function<void(std::variant<TemplateAccessModel, QString>)> callback) = 0;

    virtual void downloadFile(
        const QString& url,
        const QString& savePath,
        std::function<void(int received, int total)> onProgress,
        std::function<void(std::optional<QString>)> callback) = 0;

    virtual void downloadToBytes(
        const QString& url,
        std::function<void(int received, int total)> onProgress,
        std::function<void(std::variant<QByteArray, QString>)> callback) = 0;
};

/// Concrete implementation using HttpClient for API calls.
class TemplateRemoteDataSourceImpl : public QObject, public TemplateRemoteDataSource {
    Q_OBJECT
public:
    explicit TemplateRemoteDataSourceImpl(core::HttpClient* httpClient, QObject* parent = nullptr);
    ~TemplateRemoteDataSourceImpl() override = default;

    void getTemplates(
        const TemplateFilter& filter,
        std::function<void(std::variant<PaginatedTemplateResponse, QString>)> callback) override;

    void getTemplateById(
        const QString& id,
        std::function<void(std::variant<TemplateModel, QString>)> callback) override;

    void getTemplatesByCategory(
        const QString& categoryId,
        const TemplateFilter& filter,
        std::function<void(std::variant<PaginatedTemplateResponse, QString>)> callback) override;

    void getCategories(
        std::function<void(std::variant<QList<CategoryModel>, QString>)> callback) override;

    void getCategoryById(
        const QString& id,
        std::function<void(std::variant<CategoryModel, QString>)> callback) override;

    void requestAccess(
        const QString& templateId,
        std::function<void(std::variant<TemplateAccessModel, QString>)> callback) override;

    void downloadFile(
        const QString& url,
        const QString& savePath,
        std::function<void(int received, int total)> onProgress,
        std::function<void(std::optional<QString>)> callback) override;

    void downloadToBytes(
        const QString& url,
        std::function<void(int received, int total)> onProgress,
        std::function<void(std::variant<QByteArray, QString>)> callback) override;

private:
    PaginatedTemplateResponse parsePaginatedResponse(const QJsonObject& data);
    QString mapNetworkError(int statusCode, const QJsonObject& data);

    core::HttpClient* m_httpClient;
};

} // namespace gopost::template_browser
