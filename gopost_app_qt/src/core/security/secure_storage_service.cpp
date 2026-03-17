#include "core/security/secure_storage_service.h"

namespace gopost::core {

SecureStorageServiceImpl::SecureStorageServiceImpl(QObject* parent)
    : SecureStorageService(parent)
    , m_settings(QStringLiteral("GoPost"), QStringLiteral("SecureStorage"))
{
}

std::optional<QString> SecureStorageServiceImpl::read(const QString& key)
{
    if (!m_settings.contains(key)) {
        return std::nullopt;
    }
    return m_settings.value(key).toString();
}

void SecureStorageServiceImpl::write(const QString& key, const QString& value)
{
    m_settings.setValue(key, value);
    m_settings.sync();
}

void SecureStorageServiceImpl::delete_(const QString& key)
{
    m_settings.remove(key);
    m_settings.sync();
}

void SecureStorageServiceImpl::deleteAll()
{
    m_settings.clear();
    m_settings.sync();
}

} // namespace gopost::core
