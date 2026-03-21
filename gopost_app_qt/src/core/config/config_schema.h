#pragma once

#include "core/config/config_layer.h"

namespace gopost::config {

class ConfigSchema {
public:
    static void populateDefaults(ConfigLayer& layer);
};

} // namespace gopost::config
