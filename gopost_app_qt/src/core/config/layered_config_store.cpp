#include "core/config/layered_config_store.h"

#include <QDir>

namespace gopost::config {

LayeredConfigStore::LayeredConfigStore(const QString& baseDir)
    : baseDir_(baseDir)
{
    userFilePath_ = baseDir_ + QStringLiteral("/config/user_settings.json");
}

ConfigLayer& LayeredConfigStore::defaults() { return defaults_; }
ConfigLayer& LayeredConfigStore::userLayer() { return user_; }

void LayeredConfigStore::pushScope(const QString& name) {
    scopes_.emplace_back();
    Q_UNUSED(name);
}

void LayeredConfigStore::popScope() {
    if (!scopes_.empty())
        scopes_.pop_back();
}

std::optional<ConfigValue> LayeredConfigStore::get(const std::string& key) const {
    // Walk top to bottom: scopes (newest first) → user → defaults
    for (auto it = scopes_.rbegin(); it != scopes_.rend(); ++it) {
        auto val = it->get(key);
        if (val.has_value()) return val;
    }
    auto val = user_.get(key);
    if (val.has_value()) return val;
    return defaults_.get(key);
}

int LayeredConfigStore::getInt(const std::string& key, int def) const {
    auto v = get(key);
    return v ? configValueToInt(*v, def) : def;
}

double LayeredConfigStore::getDouble(const std::string& key, double def) const {
    auto v = get(key);
    return v ? configValueToDouble(*v, def) : def;
}

bool LayeredConfigStore::getBool(const std::string& key, bool def) const {
    auto v = get(key);
    return v ? configValueToBool(*v, def) : def;
}

std::string LayeredConfigStore::getString(const std::string& key,
                                           const std::string& def) const {
    auto v = get(key);
    return v ? configValueToString(*v, def) : def;
}

void LayeredConfigStore::set(const std::string& key, ConfigValue value) {
    topMutable().set(key, std::move(value));
}

ConfigLayer& LayeredConfigStore::topMutable() {
    if (!scopes_.empty()) return scopes_.back();
    return user_;
}

void LayeredConfigStore::saveUser() const {
    user_.save(userFilePath_);
}

void LayeredConfigStore::loadUser() {
    user_.load(userFilePath_);
}

} // namespace gopost::config
