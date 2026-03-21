#include "core/config/schema_migrator.h"

namespace gopost::config {

void SchemaMigrator::migrate(ConfigLayer& layer, int fromVersion, int toVersion) {
    for (int v = fromVersion; v < toVersion; ++v) {
        switch (v) {
        case 1: migrate_v1_to_v2(layer); break;
        default: break;
        }
    }
    // Update version stamp
    layer.set("meta.schemaVersion", toVersion);
}

void SchemaMigrator::migrate_v1_to_v2(ConfigLayer& layer) {
    // Rename "editor.fps" → "editor.playback.targetFps"
    auto fps = layer.get("editor.fps");
    if (fps.has_value()) {
        layer.set("editor.playback.targetFps", *fps);
        layer.remove("editor.fps");
    }

    // Rename "editor.codec" → "editor.export.defaultCodec"
    auto codec = layer.get("editor.codec");
    if (codec.has_value()) {
        layer.set("editor.export.defaultCodec", *codec);
        layer.remove("editor.codec");
    }
}

} // namespace gopost::config
