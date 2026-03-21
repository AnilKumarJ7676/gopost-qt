#pragma once

#include "core/config/config_value.h"

#include <QString>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace gopost::config {

class ConfigLayer {
public:
    bool load(const QString& filePath);
    bool save(const QString& filePath) const;

    std::optional<ConfigValue> get(const std::string& key) const;
    void set(const std::string& key, ConfigValue value);
    void remove(const std::string& key);
    bool contains(const std::string& key) const;
    std::vector<std::string> keys() const;

    const std::unordered_map<std::string, ConfigValue>& data() const { return data_; }

private:
    std::unordered_map<std::string, ConfigValue> data_;
};

} // namespace gopost::config
