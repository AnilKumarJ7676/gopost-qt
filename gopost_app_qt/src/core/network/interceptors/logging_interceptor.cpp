#include "core/network/interceptors/logging_interceptor.h"
#include "core/logging/app_logger.h"

namespace gopost::core {

LoggingInterceptor::LoggingInterceptor(QObject* parent)
    : QObject(parent)
{
}

void LoggingInterceptor::onRequest(const QNetworkRequest& request,
                                    const QByteArray& verb)
{
    AppLogger::debug(
        QStringLiteral("-> %1 %2")
            .arg(QString::fromUtf8(verb))
            .arg(request.url().toString()));
}

void LoggingInterceptor::onResponse(QNetworkReply* reply)
{
    const int statusCode = reply->attribute(
        QNetworkRequest::HttpStatusCodeAttribute).toInt();
    const auto method = reply->request().attribute(
        QNetworkRequest::CustomVerbAttribute).toByteArray();
    const auto url = reply->request().url().toString();

    if (reply->error() == QNetworkReply::NoError) {
        AppLogger::debug(
            QStringLiteral("<- %1 %2 %3")
                .arg(statusCode)
                .arg(QString::fromUtf8(method))
                .arg(url));
    } else {
        AppLogger::error(
            QStringLiteral("x %1 %2 - %3")
                .arg(QString::fromUtf8(method))
                .arg(url)
                .arg(reply->errorString()));
    }
}

} // namespace gopost::core
