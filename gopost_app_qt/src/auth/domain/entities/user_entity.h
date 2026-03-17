#pragma once

#include <QObject>
#include <QString>
#include <optional>

namespace gopost::auth {

class UserEntity {
    Q_GADGET
    Q_PROPERTY(QString id MEMBER m_id)
    Q_PROPERTY(QString email MEMBER m_email)
    Q_PROPERTY(QString name MEMBER m_name)
    Q_PROPERTY(QString avatarUrl MEMBER m_avatarUrl)
    Q_PROPERTY(QString role MEMBER m_role)

public:
    UserEntity() = default;

    UserEntity(const QString& id,
               const QString& email,
               const QString& name,
               const QString& avatarUrl = {},
               const QString& role = QStringLiteral("user"))
        : m_id(id)
        , m_email(email)
        , m_name(name)
        , m_avatarUrl(avatarUrl)
        , m_role(role) {}

    const QString& id() const { return m_id; }
    const QString& email() const { return m_email; }
    const QString& name() const { return m_name; }
    const QString& avatarUrl() const { return m_avatarUrl; }
    const QString& role() const { return m_role; }

    bool isValid() const { return !m_id.isEmpty(); }

    bool operator==(const UserEntity& other) const {
        return m_id == other.m_id
            && m_email == other.m_email
            && m_name == other.m_name
            && m_avatarUrl == other.m_avatarUrl
            && m_role == other.m_role;
    }

    bool operator!=(const UserEntity& other) const { return !(*this == other); }

private:
    QString m_id;
    QString m_email;
    QString m_name;
    QString m_avatarUrl;
    QString m_role = QStringLiteral("user");
};

} // namespace gopost::auth

Q_DECLARE_METATYPE(gopost::auth::UserEntity)
