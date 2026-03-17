#pragma once

#include <QObject>
#include <QNetworkReply>

namespace gopost::core {

/// Maps network errors from QNetworkReply to application exceptions
/// (ServerException, NetworkException).
class ErrorInterceptor : public QObject {
    Q_OBJECT
public:
    explicit ErrorInterceptor(QObject* parent = nullptr);
    ~ErrorInterceptor() override = default;

    /// Inspect the reply for errors and throw the appropriate
    /// application exception. Call this after the reply is finished.
    /// Throws NetworkException for timeouts/connection errors,
    /// ServerException for bad HTTP responses.
    void onResponse(QNetworkReply* reply);
};

} // namespace gopost::core
