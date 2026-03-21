#pragma once

#include <QString>

namespace gopost::core::engines {

class CoreEngine {
public:
    virtual ~CoreEngine() = default;

    virtual void initialize() = 0;
    virtual void shutdown() = 0;

    [[nodiscard]] virtual bool isInitialized() const = 0;
    [[nodiscard]] virtual QString engineName() const = 0;
};

} // namespace gopost::core::engines
