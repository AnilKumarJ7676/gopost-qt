#include "core/config/configuration_engine_new.h"
#include "core/config/config_schema.h"
#include "core/config/schema_migrator.h"

#include <QStandardPaths>

namespace gopost::config {

ConfigurationEngineNew::ConfigurationEngineNew(ILogger* logger,
                                                 const QString& baseDir)
    : logger_(logger)
{
    QString dir = baseDir;
    if (dir.isEmpty())
        dir = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);

    store_ = std::make_unique<LayeredConfigStore>(dir);

    // Populate defaults from schema
    ConfigSchema::populateDefaults(store_->defaults());

    // Load user settings
    store_->loadUser();

    // Check schema version and migrate if needed
    int version = configValueToInt(store_->userLayer().get("meta.schemaVersion").value_or(ConfigValue(1)));
    if (version < SchemaMigrator::currentVersion()) {
        if (logger_)
            logger_->info(QStringLiteral("Config"),
                          QStringLiteral("Migrating config schema from v%1 to v%2")
                              .arg(version).arg(SchemaMigrator::currentVersion()));

        SchemaMigrator::migrate(store_->userLayer(), version,
                                SchemaMigrator::currentVersion());
        store_->saveUser();
    }

    if (logger_)
        logger_->info(QStringLiteral("Config"),
                      QStringLiteral("Configuration engine initialized"));
}

int ConfigurationEngineNew::getInt(const QString& key, int defaultValue) const {
    return store_->getInt(key.toStdString(), defaultValue);
}

double ConfigurationEngineNew::getDouble(const QString& key, double defaultValue) const {
    return store_->getDouble(key.toStdString(), defaultValue);
}

bool ConfigurationEngineNew::getBool(const QString& key, bool defaultValue) const {
    return store_->getBool(key.toStdString(), defaultValue);
}

QString ConfigurationEngineNew::getString(const QString& key,
                                           const QString& defaultValue) const {
    auto s = store_->getString(key.toStdString(), defaultValue.toStdString());
    return QString::fromStdString(s);
}

void ConfigurationEngineNew::setInt(const QString& key, int value) {
    store_->set(key.toStdString(), value);
}

void ConfigurationEngineNew::setDouble(const QString& key, double value) {
    store_->set(key.toStdString(), value);
}

void ConfigurationEngineNew::setBool(const QString& key, bool value) {
    store_->set(key.toStdString(), value);
}

void ConfigurationEngineNew::setString(const QString& key, const QString& value) {
    store_->set(key.toStdString(), value.toStdString());
}

void ConfigurationEngineNew::pushScope(const QString& scope) {
    store_->pushScope(scope);
}

void ConfigurationEngineNew::popScope() {
    store_->popScope();
}

void ConfigurationEngineNew::save() {
    store_->saveUser();
}

void ConfigurationEngineNew::load() {
    store_->loadUser();
}

void ConfigurationEngineNew::migrate(int fromVersion, int toVersion) {
    SchemaMigrator::migrate(store_->userLayer(), fromVersion, toVersion);
    store_->saveUser();
}

int ConfigurationEngineNew::schemaVersion() const {
    return configValueToInt(store_->userLayer().get("meta.schemaVersion").value_or(ConfigValue(1)));
}

} // namespace gopost::config
