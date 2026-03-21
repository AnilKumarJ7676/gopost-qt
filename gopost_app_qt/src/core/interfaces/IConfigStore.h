#pragma once

#include <QString>
#include <QVariant>

namespace gopost::core::interfaces {

class IConfigStore {
public:
    virtual ~IConfigStore() = default;

    // Type-safe getters
    [[nodiscard]] virtual int getInt(const QString& key, int defaultValue = 0) const = 0;
    [[nodiscard]] virtual double getDouble(const QString& key, double defaultValue = 0.0) const = 0;
    [[nodiscard]] virtual bool getBool(const QString& key, bool defaultValue = false) const = 0;
    [[nodiscard]] virtual QString getString(const QString& key,
                                            const QString& defaultValue = {}) const = 0;

    // Type-safe setters
    virtual void setInt(const QString& key, int value) = 0;
    virtual void setDouble(const QString& key, double value) = 0;
    virtual void setBool(const QString& key, bool value) = 0;
    virtual void setString(const QString& key, const QString& value) = 0;

    // Scoped config (push/pop namespace prefix)
    virtual void pushScope(const QString& scope) = 0;
    virtual void popScope() = 0;

    // Persistence
    virtual void save() = 0;
    virtual void load() = 0;

    // Schema migration
    virtual void migrate(int fromVersion, int toVersion) = 0;
};

} // namespace gopost::core::interfaces
