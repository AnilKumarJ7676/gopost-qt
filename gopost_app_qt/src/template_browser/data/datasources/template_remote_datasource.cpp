#include "template_browser/data/datasources/template_remote_datasource.h"

#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QNetworkReply>
#include <QFile>

#include "core/network/http_client.h"

namespace gopost::template_browser {

TemplateRemoteDataSourceImpl::TemplateRemoteDataSourceImpl(
    core::HttpClient* httpClient, QObject* parent)
    : QObject(parent)
    , m_httpClient(httpClient)
{
}

void TemplateRemoteDataSourceImpl::getTemplates(
    const TemplateFilter& filter,
    std::function<void(std::variant<PaginatedTemplateResponse, QString>)> callback)
{
    auto* reply = m_httpClient->get(QStringLiteral("/templates"), filter.toQueryParameters());
    connect(reply, &QNetworkReply::finished, this, [this, reply, cb = std::move(callback)]() {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) {
            const auto doc = QJsonDocument::fromJson(reply->readAll());
            cb(mapNetworkError(reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt(),
                               doc.object()));
            return;
        }
        const auto doc = QJsonDocument::fromJson(reply->readAll());
        cb(parsePaginatedResponse(doc.object()));
    });
}

void TemplateRemoteDataSourceImpl::getTemplateById(
    const QString& id,
    std::function<void(std::variant<TemplateModel, QString>)> callback)
{
    auto* reply = m_httpClient->get(QStringLiteral("/templates/%1").arg(id));
    connect(reply, &QNetworkReply::finished, this, [this, reply, cb = std::move(callback)]() {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) {
            const auto doc = QJsonDocument::fromJson(reply->readAll());
            cb(mapNetworkError(reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt(),
                               doc.object()));
            return;
        }
        const auto doc = QJsonDocument::fromJson(reply->readAll());
        const auto data = doc.object()[QStringLiteral("data")].toObject();
        cb(TemplateModel::fromJson(data));
    });
}

void TemplateRemoteDataSourceImpl::getTemplatesByCategory(
    const QString& categoryId,
    const TemplateFilter& filter,
    std::function<void(std::variant<PaginatedTemplateResponse, QString>)> callback)
{
    auto* reply = m_httpClient->get(
        QStringLiteral("/categories/%1/templates").arg(categoryId),
        filter.toQueryParameters());
    connect(reply, &QNetworkReply::finished, this, [this, reply, cb = std::move(callback)]() {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) {
            const auto doc = QJsonDocument::fromJson(reply->readAll());
            cb(mapNetworkError(reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt(),
                               doc.object()));
            return;
        }
        const auto doc = QJsonDocument::fromJson(reply->readAll());
        cb(parsePaginatedResponse(doc.object()));
    });
}

void TemplateRemoteDataSourceImpl::getCategories(
    std::function<void(std::variant<QList<CategoryModel>, QString>)> callback)
{
    auto* reply = m_httpClient->get(QStringLiteral("/categories"));
    connect(reply, &QNetworkReply::finished, this, [this, reply, cb = std::move(callback)]() {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) {
            const auto doc = QJsonDocument::fromJson(reply->readAll());
            cb(mapNetworkError(reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt(),
                               doc.object()));
            return;
        }
        const auto doc = QJsonDocument::fromJson(reply->readAll());
        const auto list = doc.object()[QStringLiteral("data")].toArray();
        QList<CategoryModel> models;
        models.reserve(list.size());
        for (const auto& val : list) {
            models.append(CategoryModel::fromJson(val.toObject()));
        }
        cb(models);
    });
}

void TemplateRemoteDataSourceImpl::getCategoryById(
    const QString& id,
    std::function<void(std::variant<CategoryModel, QString>)> callback)
{
    auto* reply = m_httpClient->get(QStringLiteral("/categories/%1").arg(id));
    connect(reply, &QNetworkReply::finished, this, [this, reply, cb = std::move(callback)]() {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) {
            const auto doc = QJsonDocument::fromJson(reply->readAll());
            cb(mapNetworkError(reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt(),
                               doc.object()));
            return;
        }
        const auto doc = QJsonDocument::fromJson(reply->readAll());
        const auto data = doc.object()[QStringLiteral("data")].toObject();
        cb(CategoryModel::fromJson(data));
    });
}

void TemplateRemoteDataSourceImpl::requestAccess(
    const QString& templateId,
    std::function<void(std::variant<TemplateAccessModel, QString>)> callback)
{
    auto* reply = m_httpClient->post(QStringLiteral("/templates/%1/access").arg(templateId));
    connect(reply, &QNetworkReply::finished, this, [this, reply, cb = std::move(callback)]() {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) {
            const auto doc = QJsonDocument::fromJson(reply->readAll());
            cb(mapNetworkError(reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt(),
                               doc.object()));
            return;
        }
        const auto doc = QJsonDocument::fromJson(reply->readAll());
        const auto data = doc.object()[QStringLiteral("data")].toObject();
        cb(TemplateAccessModel::fromJson(data));
    });
}

void TemplateRemoteDataSourceImpl::downloadFile(
    const QString& url,
    const QString& savePath,
    std::function<void(int received, int total)> onProgress,
    std::function<void(std::optional<QString>)> callback)
{
    auto* reply = m_httpClient->get(url);
    if (onProgress) {
        connect(reply, &QNetworkReply::downloadProgress, this,
            [prog = std::move(onProgress)](qint64 received, qint64 total) {
                prog(static_cast<int>(received), static_cast<int>(total));
            });
    }
    connect(reply, &QNetworkReply::finished, this, [reply, savePath, cb = std::move(callback)]() {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) {
            cb(reply->errorString());
            return;
        }
        QFile file(savePath);
        if (!file.open(QIODevice::WriteOnly)) {
            cb(QStringLiteral("Failed to open file for writing: %1").arg(savePath));
            return;
        }
        file.write(reply->readAll());
        file.close();
        cb(std::nullopt);
    });
}

void TemplateRemoteDataSourceImpl::downloadToBytes(
    const QString& url,
    std::function<void(int received, int total)> onProgress,
    std::function<void(std::variant<QByteArray, QString>)> callback)
{
    auto* reply = m_httpClient->get(url);
    if (onProgress) {
        connect(reply, &QNetworkReply::downloadProgress, this,
            [prog = std::move(onProgress)](qint64 received, qint64 total) {
                prog(static_cast<int>(received), static_cast<int>(total));
            });
    }
    connect(reply, &QNetworkReply::finished, this, [reply, cb = std::move(callback)]() {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) {
            cb(reply->errorString());
            return;
        }
        cb(reply->readAll());
    });
}

PaginatedTemplateResponse TemplateRemoteDataSourceImpl::parsePaginatedResponse(const QJsonObject& root)
{
    PaginatedTemplateResponse result;
    const auto list = root[QStringLiteral("data")].toArray();
    const auto pagination = root[QStringLiteral("pagination")].toObject();

    result.templates.reserve(list.size());
    for (const auto& val : list) {
        result.templates.append(TemplateModel::fromJson(val.toObject()));
    }

    if (pagination.contains(QStringLiteral("next_cursor"))
        && !pagination[QStringLiteral("next_cursor")].isNull()) {
        result.nextCursor = pagination[QStringLiteral("next_cursor")].toString();
    }
    result.hasMore = pagination[QStringLiteral("has_more")].toBool(false);
    result.total = pagination[QStringLiteral("total")].toInt(list.size());

    return result;
}

QString TemplateRemoteDataSourceImpl::mapNetworkError(int statusCode, const QJsonObject& data)
{
    const auto error = data[QStringLiteral("error")].toObject();
    QString message = error[QStringLiteral("message")].toString(QStringLiteral("Server error"));

    switch (statusCode) {
    case 401:
        if (message == QStringLiteral("Server error"))
            message = QStringLiteral("Authentication required");
        break;
    case 403:
        if (message == QStringLiteral("Server error"))
            message = QStringLiteral("Subscription required to access this template");
        break;
    case 429:
        if (message == QStringLiteral("Server error"))
            message = QStringLiteral("Too many requests. Please try again later");
        break;
    default:
        break;
    }

    return message;
}

} // namespace gopost::template_browser
