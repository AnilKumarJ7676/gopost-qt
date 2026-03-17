#pragma once

#include <QObject>
#include <QString>
#include <QSettings>
#include <optional>

namespace gopost::core {

/// Abstract interface for secure key-value storage.
class SecureStorageService : public QObject {
    Q_OBJECT
public:
    explicit SecureStorageService(QObject* parent = nullptr)
        : QObject(parent) {}
    ~SecureStorageService() override = default;

    virtual std::optional<QString> read(const QString& key) = 0;
    virtual void write(const QString& key, const QString& value) = 0;
    virtual void delete_(const QString& key) = 0;
    virtual void deleteAll() = 0;
};

/// Implementation of SecureStorageService using QSettings
/// (platform-appropriate encrypted storage equivalent).
class SecureStorageServiceImpl : public SecureStorageService {
    Q_OBJECT
public:
    explicit SecureStorageServiceImpl(QObject* parent = nullptr);
    ~SecureStorageServiceImpl() override = default;

    std::optional<QString> read(const QString& key) override;
    void write(const QString& key, const QString& value) override;
    void delete_(const QString& key) override;
    void deleteAll() override;

private:
    QSettings m_settings;
};

} // namespace gopost::core
