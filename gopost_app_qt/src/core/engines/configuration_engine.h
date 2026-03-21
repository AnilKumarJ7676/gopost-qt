#pragma once

#include "core_engine.h"
#include "core/config/app_environment.h"

#include <QObject>
#include <QHash>
#include <QMutex>
#include <QSettings>
#include <QVariant>
#include <QVariantMap>
#include <functional>

namespace gopost::platform { class PlatformCapabilityEngine; }

namespace gopost::core::engines {

class LoggingEngine;

class ConfigurationEngine : public QObject, public CoreEngine {
    Q_OBJECT
public:
    explicit ConfigurationEngine(platform::PlatformCapabilityEngine* platform,
                                 LoggingEngine* logging,
                                 QObject* parent = nullptr);
    ~ConfigurationEngine() override;

    // CoreEngine
    void initialize() override;
    void shutdown() override;
    [[nodiscard]] bool isInitialized() const override;
    [[nodiscard]] QString engineName() const override { return QStringLiteral("Config"); }

    // Type-safe getters with defaults
    [[nodiscard]] QString getString(const QString& key, const QString& defaultValue = {}) const;
    [[nodiscard]] int getInt(const QString& key, int defaultValue = 0) const;
    [[nodiscard]] double getDouble(const QString& key, double defaultValue = 0.0) const;
    [[nodiscard]] bool getBool(const QString& key, bool defaultValue = false) const;

    // Resolved value across all layers
    [[nodiscard]] QVariant resolvedValue(const QString& key) const;

    // Write to user preferences layer (persisted via QSettings)
    void setUserPreference(const QString& key, const QVariant& value);
    void removeUserPreference(const QString& key);

    // Observable config changes
    using ChangeCallback = std::function<void(const QString& key, const QVariant& newValue)>;
    int observe(const QString& key, ChangeCallback callback);
    void unobserve(int observerId);

    // Bulk access for QML
    Q_INVOKABLE QVariantMap allResolved() const;

    // Environment (wraps EnvironmentConfig)
    [[nodiscard]] AppEnvironment environment() const;
    [[nodiscard]] bool isProduction() const;
    [[nodiscard]] bool isDevelopment() const;

signals:
    void configChanged(const QString& key, const QVariant& value);

private:
    void loadDefaults();
    void loadPlatformDefaults();
    void loadUserPreferences();
    void loadEnvironmentOverrides();
    void notifyObservers(const QString& key, const QVariant& value);

    platform::PlatformCapabilityEngine* m_platform = nullptr;
    LoggingEngine* m_logging = nullptr;
    bool m_initialized = false;

    // Layer storage (priority: env > user > platform > defaults)
    QVariantMap m_defaults;
    QVariantMap m_platformDefaults;
    QVariantMap m_userPreferences;
    QVariantMap m_envOverrides;

    // QSettings for persistence
    std::unique_ptr<QSettings> m_settings;

    // Observers
    mutable QMutex m_mutex;
    int m_nextObserverId = 1;
    struct Observer {
        QString key;
        ChangeCallback callback;
    };
    QHash<int, Observer> m_observers;
};

} // namespace gopost::core::engines
