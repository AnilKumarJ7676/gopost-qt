#pragma once

#include <QString>

namespace gopost::video_editor {

/// Local model for a clip on the video timeline (S8-07).
struct VideoClipInfo {
    int clipId{0};
    QString sourcePath;
    bool isVideo{true};
    QString displayName;
    double durationSeconds{0.0};
    double timelineIn{0.0};
    double timelineOut{0.0};
};

} // namespace gopost::video_editor
