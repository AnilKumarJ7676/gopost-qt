#pragma once

#include "core/config/config_layer.h"

namespace gopost::config {

class SchemaMigrator {
public:
    static constexpr int kCurrentVersion = 2;

    static int currentVersion() { return kCurrentVersion; }
    static void migrate(ConfigLayer& layer, int fromVersion, int toVersion);

private:
    static void migrate_v1_to_v2(ConfigLayer& layer);
};

} // namespace gopost::config
