#include "core/network/interceptors/error_interceptor.h"
#include "core/error/exceptions.h"

#include <QJsonDocument>
#include <QJsonObject>

namespace gopost::core {

ErrorInterceptor::ErrorInterceptor(QObject* parent)
    : QObject(parent)
{
}

void ErrorInterceptor::onResponse(QNetworkReply* reply)
{
    if (reply->error() == QNetworkReply::NoError) {
        return;
    }

    switch (reply->error()) {
    case QNetworkReply::TimeoutError:
    case QNetworkReply::ConnectionRefusedError:
    case QNetworkReply::RemoteHostClosedError:
    case QNetworkReply::HostNotFoundError:
    case QNetworkReply::TemporaryNetworkFailureError:
    case QNetworkReply::NetworkSessionFailedError:
    case QNetworkReply::UnknownNetworkError:
        throw NetworkException(QStringLiteral("Connection timed out"));

    default: {
        const int statusCode = reply->attribute(
            QNetworkRequest::HttpStatusCodeAttribute).toInt();
        const auto body = reply->readAll();

        QString message = QStringLiteral("Server error");
        QString code;

        const auto doc = QJsonDocument::fromJson(body);
        if (doc.isObject()) {
            const auto obj = doc.object();
            const auto errorObj = obj[QStringLiteral("error")].toObject();
            if (!errorObj.isEmpty()) {
                const auto msgVal = errorObj[QStringLiteral("message")].toString();
                if (!msgVal.isEmpty()) {
                    message = msgVal;
                }
                code = errorObj[QStringLiteral("code")].toString();
            }
        }

        throw ServerException(message, statusCode, code);
    }
    }
}

} // namespace gopost::core
