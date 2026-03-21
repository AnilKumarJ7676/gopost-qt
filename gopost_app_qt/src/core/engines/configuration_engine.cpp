#include "configuration_engine.h"
#include "platform_capability_engine.h"
#include "logging_engine.h"

#include <QCoreApplication>
#include <QProcessEnvironment>

namespace gopost::core::engines {

ConfigurationEngine::ConfigurationEngine(PlatformCapabilityEngine* platform,
                                         LoggingEngine* logging,
                                         QObject* parent)
    : QObject(parent)
    , m_platform(platform)
    , m_logging(logging) {}

ConfigurationEngine::~ConfigurationEngine() {
    shutdown();
}

void ConfigurationEngine::initialize() {
    if (m_initialized) return;

    m_settings = std::make_unique<QSettings>(
        QSettings::IniFormat, QSettings::UserScope,
        QCoreApplication::organizationName(),
        QCoreApplication::applicationName());

    // Load layers in order (lowest priority first)
    loadDefaults();
    loadPlatformDefaults();
    loadUserPreferences();
    loadEnvironmentOverrides();

    m_initialized = true;

    if (m_logging) {
        m_logging->info(QStringLiteral("Config"),
                        QStringLiteral("Configuration engine initialized, env=%1")
                            .arg(EnvironmentConfig::isProduction()
                                     ? QStringLiteral("production")
                                     : QStringLiteral("development")));
    }
}

void ConfigurationEngine::shutdown() {
    if (!m_initialized) return;
    m_settings.reset();
    m_initialized = false;
}

bool ConfigurationEngine::isInitialized() const {
    return m_initialized;
}

void ConfigurationEngine::loadDefaults() {
    m_defaults[QStringLiteral("platform.thumbnailMemoryCacheEntries")] = 500;
    m_defaults[QStringLiteral("platform.thumbnailDiskCacheBytes")] = 200 * 1024 * 1024;
    m_defaults[QStringLiteral("platform.maxDecoders")] = 4;
    m_defaults[QStringLiteral("platform.enableDiskCache")] = true;
    m_defaults[QStringLiteral("logging.globalLevel")] = 1; // Debug
    m_defaults[QStringLiteral("logging.maxFileSizeMb")] = 10;
    m_defaults[QStringLiteral("logging.maxFiles")] = 5;
    m_defaults[QStringLiteral("rendering.enableGpu")] = true;
    m_defaults[QStringLiteral("rendering.proxyMode")] = false;
}

void ConfigurationEngine::loadPlatformDefaults() {
    if (!m_platform) return;

    auto defaults = m_platform->adaptiveDefaults();
    m_platformDefaults[QStringLiteral("platform.thumbnailMemoryCacheEntries")] =
        defaults.thumbnailMemoryCacheEntries;
    m_platformDefaults[QStringLiteral("platform.thumbnailDiskCacheBytes")] =
        defaults.thumbnailDiskCacheBytes;
    m_platformDefaults[QStringLiteral("platform.maxDecoders")] =
        defaults.maxDecoders;
    m_platformDefaults[QStringLiteral("platform.enableDiskCache")] =
        defaults.enableDiskCache;
    m_platformDefaults[QStringLiteral("platform.recommendedFramePoolMb")] =
        m_platform->recommendedFramePoolMb();
}

void ConfigurationEngine::loadUserPreferences() {
    if (!m_settings) return;

    m_settings->beginGroup(QStringLiteral("preferences"));
    const auto keys = m_settings->allKeys();
    for (const auto& key : keys) {
        m_userPreferences[key] = m_settings->value(key);
    }
    m_settings->endGroup();
}

void ConfigurationEngine::loadEnvironmentOverrides() {
    const auto env = QProcessEnvironment::systemEnvironment();
    const QString prefix = QStringLiteral("GOPOST_CFG_");

    for (const auto& key : env.keys()) {
        if (key.startsWith(prefix)) {
            // Convert GOPOST_CFG_PLATFORM_MAX_DECODERS → platform.maxDecoders
            QString configKey = key.mid(prefix.length()).toLower().replace('_', '.');
            m_envOverrides[configKey] = env.value(key);
        }
    }
}

QVariant ConfigurationEngine::resolvedValue(const QString& key) const {
    // Priority: env > user > platform > defaults
    if (m_envOverrides.contains(key)) return m_envOverrides.value(key);
    if (m_userPreferences.contains(key)) return m_userPreferences.value(key);
    if (m_platformDefaults.contains(key)) return m_platformDefaults.value(key);
    return m_defaults.value(key);
}

QString ConfigurationEngine::getString(const QString& key, const QString& defaultValue) const {
    QVariant val = resolvedValue(key);
    return val.isValid() ? val.toString() : defaultValue;
}

int ConfigurationEngine::getInt(const QString& key, int defaultValue) const {
    QVariant val = resolvedValue(key);
    return val.isValid() ? val.toInt() : defaultValue;
}

double ConfigurationEngine::getDouble(const QString& key, double defaultValue) const {
    QVariant val = resolvedValue(key);
    return val.isValid() ? val.toDouble() : defaultValue;
}

bool ConfigurationEngine::getBool(const QString& key, bool defaultValue) const {
    QVariant val = resolvedValue(key);
    return val.isValid() ? val.toBool() : defaultValue;
}

void ConfigurationEngine::setUserPreference(const QString& key, const QVariant& value) {
    m_userPreferences[key] = value;

    if (m_settings) {
        m_settings->beginGroup(QStringLiteral("preferences"));
        m_settings->setValue(key, value);
        m_settings->endGroup();
        m_settings->sync();
    }

    QVariant resolved = resolvedValue(key);
    notifyObservers(key, resolved);
    emit configChanged(key, resolved);
}

void ConfigurationEngine::removeUserPreference(const QString& key) {
    m_userPreferences.remove(key);

    if (m_settings) {
        m_settings->beginGroup(QStringLiteral("preferences"));
        m_settings->remove(key);
        m_settings->endGroup();
        m_settings->sync();
    }

    QVariant resolved = resolvedValue(key);
    notifyObservers(key, resolved);
    emit configChanged(key, resolved);
}

int ConfigurationEngine::observe(const QString& key, ChangeCallback callback) {
    QMutexLocker lock(&m_mutex);
    int id = m_nextObserverId++;
    m_observers.insert(id, {key, std::move(callback)});
    return id;
}

void ConfigurationEngine::unobserve(int observerId) {
    QMutexLocker lock(&m_mutex);
    m_observers.remove(observerId);
}

void ConfigurationEngine::notifyObservers(const QString& key, const QVariant& value) {
    QMutexLocker lock(&m_mutex);
    for (auto it = m_observers.constBegin(); it != m_observers.constEnd(); ++it) {
        if (it->key == key && it->callback) {
            it->callback(key, value);
        }
    }
}

QVariantMap ConfigurationEngine::allResolved() const {
    QVariantMap result = m_defaults;

    // Merge layers in order
    for (auto it = m_platformDefaults.constBegin(); it != m_platformDefaults.constEnd(); ++it)
        result[it.key()] = it.value();
    for (auto it = m_userPreferences.constBegin(); it != m_userPreferences.constEnd(); ++it)
        result[it.key()] = it.value();
    for (auto it = m_envOverrides.constBegin(); it != m_envOverrides.constEnd(); ++it)
        result[it.key()] = it.value();

    return result;
}

AppEnvironment ConfigurationEngine::environment() const {
    return EnvironmentConfig::current();
}

bool ConfigurationEngine::isProduction() const {
    return EnvironmentConfig::isProduction();
}

bool ConfigurationEngine::isDevelopment() const {
    return EnvironmentConfig::isDevelopment();
}

} // namespace gopost::core::engines
