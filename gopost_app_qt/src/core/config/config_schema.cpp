#include "core/config/config_schema.h"

namespace gopost::config {

void ConfigSchema::populateDefaults(ConfigLayer& layer) {
    layer.set("editor.timeline.defaultZoom",              1.0);
    layer.set("editor.timeline.trackHeight",              80);
    layer.set("editor.playback.targetFps",                30);
    layer.set("editor.playback.loopPlayback",             false);
    layer.set("editor.autosave.enabled",                  true);
    layer.set("editor.autosave.intervalSeconds",          120);
    layer.set("editor.export.defaultCodec",               std::string("h264"));
    layer.set("editor.export.defaultResolution",          std::string("1080p"));
    layer.set("editor.export.defaultBitrate",             8000000);
    layer.set("performance.frameCache.maxFrames",         120);
    layer.set("performance.thumbnailAtlas.size",          256);
    layer.set("performance.gpu.preferHardwareDecode",     true);
    layer.set("app.theme",                                std::string("dark"));
    layer.set("app.language",                             std::string("en"));
    layer.set("app.recentProjectsMax",                    10);
}

} // namespace gopost::config
