#pragma once

#include "core/config/config_layer.h"

#include <QString>
#include <optional>
#include <string>
#include <vector>

namespace gopost::config {

class LayeredConfigStore {
public:
    explicit LayeredConfigStore(const QString& baseDir);

    // Layer management
    ConfigLayer& defaults();
    ConfigLayer& userLayer();
    void pushScope(const QString& name);
    void popScope();

    // Getters — walk top to bottom
    std::optional<ConfigValue> get(const std::string& key) const;
    int getInt(const std::string& key, int def = 0) const;
    double getDouble(const std::string& key, double def = 0.0) const;
    bool getBool(const std::string& key, bool def = false) const;
    std::string getString(const std::string& key, const std::string& def = {}) const;

    // Setters — write to topmost mutable layer
    void set(const std::string& key, ConfigValue value);

    // Persistence
    void saveUser() const;
    void loadUser();

    QString userFilePath() const { return userFilePath_; }

private:
    ConfigLayer& topMutable();

    QString baseDir_;
    QString userFilePath_;

    ConfigLayer defaults_;
    ConfigLayer user_;
    std::vector<ConfigLayer> scopes_;
};

} // namespace gopost::config
