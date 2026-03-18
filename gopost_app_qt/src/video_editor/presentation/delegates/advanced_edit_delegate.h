#pragma once

#include <optional>
#include <QString>
#include <vector>

#include "video_editor/domain/models/video_project.h"
#include "video_editor/presentation/delegates/timeline_operations.h"

namespace gopost::video_editor {

// ---------------------------------------------------------------------------
// Speed ramp preset data
// ---------------------------------------------------------------------------
struct SpeedRampPreset {
    QString name;
    std::vector<std::pair<double, double>> keyframes; // (normalised time, speed)
};

// ---------------------------------------------------------------------------
// AdvancedEditDelegate — NLE edits, speed, freeze, markers, multi-cam, AI
//
// Converted 1:1 from advanced_edit_delegate.dart.
// Plain C++ class (not QObject).
// ---------------------------------------------------------------------------
class AdvancedEditDelegate {
public:
    explicit AdvancedEditDelegate(TimelineOperations* ops);
    ~AdvancedEditDelegate();

    // ---- NLE edits ---------------------------------------------------------
    void insertEdit(int clipId, double atTime);
    void overwriteEdit(int clipId, double atTime);
    void rollEdit(int clipId, bool leftEdge, double delta);
    void slipEdit(int clipId, double delta);
    void slideEdit(int clipId, double delta);

    // ---- rate stretch -------------------------------------------------------
    void rateStretch(int clipId, double newDuration);

    // ---- duplicate ----------------------------------------------------------
    void duplicateClip(int clipId);

    // ---- speed controls -----------------------------------------------------
    void setClipSpeed(int clipId, double speed);
    void reverseClip(int clipId);
    void freezeFrame(int clipId);

    // ---- speed ramp ---------------------------------------------------------
    void applySpeedRamp(int clipId, const SpeedRampPreset& preset);
    void addSpeedRampPoint(int clipId, double normalizedTime, double speed);
    void removeSpeedRampPoint(int clipId, double time);
    void moveSpeedRampPoint(int clipId, double oldTime, double newTime, double newSpeed);
    void clearSpeedRamp(int clipId);
    static std::vector<SpeedRampPreset> speedRampPresets();

    // ---- markers ------------------------------------------------------------
    void addMarker(MarkerType type, const QString& label = {});
    void addMarkerAtPosition(double positionSeconds, MarkerType type,
                             const QString& label, const QString& color = {});
    void addClipMarker(int clipId, MarkerType type, const QString& label);
    void addMarkerRegion(double startSeconds, double endSeconds,
                         MarkerType type, const QString& label);
    void removeMarker(int markerId);
    void updateMarker(int markerId, const QString& label);
    void updateMarkerFull(int markerId, const QString& label,
                          const QString& notes, const QString& color, int type);
    void updateMarkerPosition(int markerId, double positionSeconds);
    void navigateToMarker(int markerId);
    void navigateToNextMarker();
    void navigateToPreviousMarker();

    // ---- project dimensions -------------------------------------------------
    void setProjectDimensions(int width, int height);

    // ---- engine pass-through ------------------------------------------------
    void setEngineEffect(int clipId, const QString& effectJson);
    void setEngineMask(int clipId, const QString& maskJson);
    void setEngineTracking(int clipId, const QString& trackingJson);
    void setEngineStabilization(int clipId, bool enabled);
    void setEngineText(int clipId, const QString& textJson);
    void setEngineShape(int clipId, const QString& shapeJson);
    void setEngineAudioEffect(int clipId, const QString& audioFxJson);

    // ---- AI segmentation ----------------------------------------------------
    void runAiSegmentation(int clipId);

    // ---- proxy management ---------------------------------------------------
    void generateProxyForClip(int clipId);
    void toggleProxyPlayback();

    // ---- multi-cam ----------------------------------------------------------
    void enableMultiCam(const std::vector<int>& clipIds);
    void switchMultiCamAngle(int angleIndex);

private:
    TimelineOperations* ops_;
    int nextMarkerId() const;
};

} // namespace gopost::video_editor
