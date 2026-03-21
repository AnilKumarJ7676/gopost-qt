#pragma once

#include <QJsonValue>
#include <QString>
#include <string>
#include <variant>

namespace gopost::config {

using ConfigValue = std::variant<int, double, bool, std::string>;

inline QJsonValue configValueToJson(const ConfigValue& v) {
    return std::visit([](const auto& val) -> QJsonValue {
        using T = std::decay_t<decltype(val)>;
        if constexpr (std::is_same_v<T, int>)         return val;
        else if constexpr (std::is_same_v<T, double>)  return val;
        else if constexpr (std::is_same_v<T, bool>)    return val;
        else if constexpr (std::is_same_v<T, std::string>)
            return QString::fromStdString(val);
    }, v);
}

inline ConfigValue configValueFromJson(const QJsonValue& j) {
    if (j.isBool())   return j.toBool();
    if (j.isDouble()) {
        double d = j.toDouble();
        if (d == static_cast<int>(d) && !j.isString())
            return static_cast<int>(d);
        return d;
    }
    if (j.isString()) return j.toString().toStdString();
    return 0;
}

inline int configValueToInt(const ConfigValue& v, int def = 0) {
    if (auto* p = std::get_if<int>(&v)) return *p;
    if (auto* p = std::get_if<double>(&v)) return static_cast<int>(*p);
    return def;
}

inline double configValueToDouble(const ConfigValue& v, double def = 0.0) {
    if (auto* p = std::get_if<double>(&v)) return *p;
    if (auto* p = std::get_if<int>(&v)) return static_cast<double>(*p);
    return def;
}

inline bool configValueToBool(const ConfigValue& v, bool def = false) {
    if (auto* p = std::get_if<bool>(&v)) return *p;
    return def;
}

inline std::string configValueToString(const ConfigValue& v, const std::string& def = {}) {
    if (auto* p = std::get_if<std::string>(&v)) return *p;
    return def;
}

} // namespace gopost::config
