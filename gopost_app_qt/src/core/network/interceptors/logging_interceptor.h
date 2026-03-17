#pragma once

#include <QObject>
#include <QNetworkReply>
#include <QNetworkRequest>

namespace gopost::core {

/// Logs HTTP requests and responses using AppLogger.
class LoggingInterceptor : public QObject {
    Q_OBJECT
public:
    explicit LoggingInterceptor(QObject* parent = nullptr);
    ~LoggingInterceptor() override = default;

    /// Log the outgoing request.
    void onRequest(const QNetworkRequest& request, const QByteArray& verb);

    /// Log the incoming response or error.
    void onResponse(QNetworkReply* reply);
};

} // namespace gopost::core
