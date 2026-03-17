#pragma once

#include <QString>
#include <QMap>
#include <optional>

namespace gopost::core {

class Failure {
public:
    Failure(const QString& message, const QString& code = {})
        : m_message(message), m_code(code) {}

    virtual ~Failure() = default;

    const QString& message() const { return m_message; }
    const QString& code() const { return m_code; }

    bool operator==(const Failure& other) const {
        return m_message == other.m_message && m_code == other.m_code;
    }

protected:
    QString m_message;
    QString m_code;
};

class ServerFailure : public Failure {
public:
    ServerFailure(const QString& message,
                  const QString& code = {},
                  std::optional<int> statusCode = std::nullopt)
        : Failure(message, code), m_statusCode(statusCode) {}

    std::optional<int> statusCode() const { return m_statusCode; }

    bool operator==(const ServerFailure& other) const {
        return Failure::operator==(other) && m_statusCode == other.m_statusCode;
    }

private:
    std::optional<int> m_statusCode;
};

class NetworkFailure : public Failure {
public:
    NetworkFailure(const QString& message = QStringLiteral("No internet connection"),
                   const QString& code = QStringLiteral("NETWORK_ERROR"))
        : Failure(message, code) {}
};

class CacheFailure : public Failure {
public:
    CacheFailure(const QString& message = QStringLiteral("Cache read/write failed"),
                 const QString& code = QStringLiteral("CACHE_ERROR"))
        : Failure(message, code) {}
};

class ValidationFailure : public Failure {
public:
    ValidationFailure(const QString& message,
                      const QString& code = QStringLiteral("VALIDATION_ERROR"),
                      const QMap<QString, QString>& fieldErrors = {})
        : Failure(message, code), m_fieldErrors(fieldErrors) {}

    const QMap<QString, QString>& fieldErrors() const { return m_fieldErrors; }

    bool operator==(const ValidationFailure& other) const {
        return Failure::operator==(other) && m_fieldErrors == other.m_fieldErrors;
    }

private:
    QMap<QString, QString> m_fieldErrors;
};

class AuthFailure : public Failure {
public:
    AuthFailure(const QString& message,
                const QString& code = QStringLiteral("AUTH_ERROR"))
        : Failure(message, code) {}
};

} // namespace gopost::core
