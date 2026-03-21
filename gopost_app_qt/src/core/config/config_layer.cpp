#include "core/config/config_layer.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>

namespace gopost::config {

bool ConfigLayer::load(const QString& filePath) {
    QFile f(filePath);
    if (!f.open(QIODevice::ReadOnly)) return false;

    auto doc = QJsonDocument::fromJson(f.readAll());
    if (!doc.isObject()) return false;

    auto obj = doc.object();
    data_.clear();
    for (auto it = obj.begin(); it != obj.end(); ++it)
        data_[it.key().toStdString()] = configValueFromJson(it.value());

    return true;
}

bool ConfigLayer::save(const QString& filePath) const {
    QDir().mkpath(QFileInfo(filePath).absolutePath());

    QJsonObject obj;
    for (const auto& [k, v] : data_)
        obj[QString::fromStdString(k)] = configValueToJson(v);

    QFile f(filePath);
    if (!f.open(QIODevice::WriteOnly)) return false;
    f.write(QJsonDocument(obj).toJson(QJsonDocument::Indented));
    return true;
}

std::optional<ConfigValue> ConfigLayer::get(const std::string& key) const {
    auto it = data_.find(key);
    if (it != data_.end()) return it->second;
    return std::nullopt;
}

void ConfigLayer::set(const std::string& key, ConfigValue value) {
    data_[key] = std::move(value);
}

void ConfigLayer::remove(const std::string& key) {
    data_.erase(key);
}

bool ConfigLayer::contains(const std::string& key) const {
    return data_.count(key) > 0;
}

std::vector<std::string> ConfigLayer::keys() const {
    std::vector<std::string> result;
    result.reserve(data_.size());
    for (const auto& [k, _] : data_)
        result.push_back(k);
    return result;
}

} // namespace gopost::config
