#pragma once

#include <QString>
#include <stdexcept>
#include <optional>

namespace gopost::core {

class ServerException : public std::runtime_error {
public:
    explicit ServerException(const QString& message,
                             std::optional<int> statusCode = std::nullopt,
                             const QString& code = {})
        : std::runtime_error(message.toStdString())
        , m_message(message)
        , m_statusCode(statusCode)
        , m_code(code) {}

    const QString& message() const { return m_message; }
    std::optional<int> statusCode() const { return m_statusCode; }
    const QString& code() const { return m_code; }

    QString toString() const {
        return QStringLiteral("ServerException(%1): %2")
            .arg(m_statusCode.value_or(0))
            .arg(m_message);
    }

private:
    QString m_message;
    std::optional<int> m_statusCode;
    QString m_code;
};

class NetworkException : public std::runtime_error {
public:
    explicit NetworkException(const QString& message = QStringLiteral("No internet connection"))
        : std::runtime_error(message.toStdString())
        , m_message(message) {}

    const QString& message() const { return m_message; }

    QString toString() const {
        return QStringLiteral("NetworkException: %1").arg(m_message);
    }

private:
    QString m_message;
};

class CacheException : public std::runtime_error {
public:
    explicit CacheException(const QString& message = QStringLiteral("Cache operation failed"))
        : std::runtime_error(message.toStdString())
        , m_message(message) {}

    const QString& message() const { return m_message; }

    QString toString() const {
        return QStringLiteral("CacheException: %1").arg(m_message);
    }

private:
    QString m_message;
};

class AuthException : public std::runtime_error {
public:
    explicit AuthException(const QString& message)
        : std::runtime_error(message.toStdString())
        , m_message(message) {}

    const QString& message() const { return m_message; }

    QString toString() const {
        return QStringLiteral("AuthException: %1").arg(m_message);
    }

private:
    QString m_message;
};

} // namespace gopost::core
