#pragma once

#include "core/interfaces/IConfigStore.h"
#include "core/interfaces/ILogger.h"
#include "core/config/layered_config_store.h"

#include <memory>

namespace gopost::config {

using core::interfaces::IConfigStore;
using core::interfaces::ILogger;

class ConfigurationEngineNew : public IConfigStore {
public:
    explicit ConfigurationEngineNew(ILogger* logger = nullptr,
                                     const QString& baseDir = {});

    // IConfigStore
    [[nodiscard]] int getInt(const QString& key, int defaultValue = 0) const override;
    [[nodiscard]] double getDouble(const QString& key, double defaultValue = 0.0) const override;
    [[nodiscard]] bool getBool(const QString& key, bool defaultValue = false) const override;
    [[nodiscard]] QString getString(const QString& key,
                                    const QString& defaultValue = {}) const override;

    void setInt(const QString& key, int value) override;
    void setDouble(const QString& key, double value) override;
    void setBool(const QString& key, bool value) override;
    void setString(const QString& key, const QString& value) override;

    void pushScope(const QString& scope) override;
    void popScope() override;

    void save() override;
    void load() override;
    void migrate(int fromVersion, int toVersion) override;

    int schemaVersion() const;

private:
    ILogger* logger_;
    std::unique_ptr<LayeredConfigStore> store_;
};

} // namespace gopost::config
