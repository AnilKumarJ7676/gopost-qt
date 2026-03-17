#pragma once

#include <QByteArray>
#include <QString>

#include <cstdint>
#include <functional>
#include <map>
#include <optional>
#include <stdexcept>
#include <vector>

namespace gopost::rendering {

// =========================================================================
// Exception
// =========================================================================

class EngineException : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

// =========================================================================
// Enums
// =========================================================================

enum class BlendMode { Normal, Multiply, Screen, Overlay };

enum class LayerType {
    Image,
    SolidColor,
    Text,
    Shape,
    Group,
    Adjustment,
    Gradient,
    Sticker
};

enum class CanvasColorSpace { Srgb, DisplayP3, AdobeRgb };

enum class ImageFormat { Jpeg, Png, Webp, Heic, Bmp, Gif, Tiff, Tga };

enum class ExportFormat { Jpeg, Png, Webp };

enum class ExportResolution {
    Original,
    Res4k,
    Res1080p,
    Res720p,
    InstagramSquare,
    InstagramStory,
    Custom
};

enum class EffectCategory {
    Adjustment,
    Color,
    Blur,
    Stylize,
    Distort,
    Preset
};

enum class FontStyle { Normal, Bold, Italic, BoldItalic };

enum class TextAlignment { Left, Center, Right, Justify };

enum class MaskType { None, Raster, Vector, Clipping };

enum class MaskBrushMode { Paint, Erase };

enum class VideoTrackType { Video, Audio, Title, Effect, Subtitle };

enum class VideoClipSourceType { Video, Image, Title, Color };

enum class VideoMaskType { Rectangle, Ellipse, Bezier, Freehand };

enum class ShapeType { Rectangle, Ellipse, Polygon, Star, Line, Arrow, Path };

enum class StabilizationMethod { Translation, TranslationRotation, Perspective };

enum class AiSegmentationType { Background, Person, Object, Sky };

enum class ProxyResolution { Quarter, Half, ThreeQuarter };

enum class DecoderPriority { High, Medium, Low };

enum class ThumbnailJobStatus { Queued, InProgress, Completed, Failed, Cancelled };

// =========================================================================
// Data Structs — Image Editor
// =========================================================================

struct GpuCapabilities {
    QString renderer;
    bool supportsCompute = false;
    int maxTextureSize = 0;
    int maxFramebufferSize = 0;
};

struct TemplateMetadata {
    QString templateId;
    QString name;
    QString description;
    int width = 0;
    int height = 0;
    std::optional<int> durationMs;
    int layerCount = 0;
    int version = 0;
};

struct EngineConfig {
    int threadCount = 4;
    int framePoolSizeMb = 128;
    bool enableGpu = true;
    int logLevel = 2;
};

struct HwDecoderInfo {
    bool available = false;
    QString deviceName;
    int maxWidth = 0;
    int maxHeight = 0;
};

struct CanvasConfig {
    int width = 0;
    int height = 0;
    double dpi = 72.0;
    CanvasColorSpace colorSpace = CanvasColorSpace::Srgb;
    double bgR = 1.0;
    double bgG = 1.0;
    double bgB = 1.0;
    double bgA = 1.0;
    bool transparentBackground = false;
};

struct LayerInfo {
    int id = 0;
    LayerType type = LayerType::Image;
    QString name;
    double opacity = 1.0;
    BlendMode blendMode = BlendMode::Normal;
    bool visible = true;
    bool locked = false;
    double tx = 0.0;
    double ty = 0.0;
    double sx = 1.0;
    double sy = 1.0;
    double rotation = 0.0;
    int contentWidth = 0;
    int contentHeight = 0;
};

struct ViewportState {
    double panX = 0.0;
    double panY = 0.0;
    double zoom = 1.0;
    double rotation = 0.0;
};

struct DecodedImage {
    int width = 0;
    int height = 0;
    QByteArray pixels; // RGBA
};

struct ImageInfo {
    int width = 0;
    int height = 0;
    int channels = 0;
    ImageFormat format = ImageFormat::Jpeg;
    bool hasAlpha = false;
};

struct CanvasSize {
    int width = 0;
    int height = 0;
    double dpi = 72.0;
};

struct GpuTextureResult {
    int textureHandle = 0;
    int width = 0;
    int height = 0;
};

// =========================================================================
// Effect / Filter Data Structs
// =========================================================================

struct EffectParamDef {
    QString id;
    QString displayName;
    double defaultValue = 0.0;
    double minValue = 0.0;
    double maxValue = 1.0;
};

struct EffectDef {
    QString id;
    QString displayName;
    EffectCategory category = EffectCategory::Adjustment;
    std::vector<EffectParamDef> params;
    bool gpuAccelerated = false;
};

struct EffectInstance {
    int instanceId = 0;
    QString effectId;
    bool enabled = true;
    double mix = 1.0;
    std::map<QString, double> paramValues;
};

struct PresetFilterInfo {
    int index = 0;
    QString name;
    QString category;
};

// =========================================================================
// Text Data Structs
// =========================================================================

struct TextConfig {
    QString text;
    QString fontFamily = QStringLiteral("System");
    FontStyle style = FontStyle::Normal;
    double fontSize = 24.0;
    double colorR = 1.0;
    double colorG = 1.0;
    double colorB = 1.0;
    double colorA = 1.0;
    TextAlignment alignment = TextAlignment::Left;
    double lineHeight = 1.2;
    double letterSpacing = 0.0;
    bool hasShadow = false;
    double shadowColorR = 0.0;
    double shadowColorG = 0.0;
    double shadowColorB = 0.0;
    double shadowColorA = 0.5;
    double shadowOffsetX = 2.0;
    double shadowOffsetY = 2.0;
    double shadowBlur = 4.0;
    bool hasOutline = false;
    double outlineColorR = 0.0;
    double outlineColorG = 0.0;
    double outlineColorB = 0.0;
    double outlineColorA = 1.0;
    double outlineWidth = 1.0;
};

// =========================================================================
// Export Data Structs
// =========================================================================

struct ExportConfig {
    ExportFormat format = ExportFormat::Jpeg;
    int quality = 85;
    ExportResolution resolution = ExportResolution::Original;
    int customWidth = 0;
    int customHeight = 0;
    double dpi = 72.0;
};

struct ExportResult {
    QString filePath;
    int fileSize = 0;
    int width = 0;
    int height = 0;
};

// =========================================================================
// Video Timeline Data Structs
// =========================================================================

struct TimelineConfig {
    double frameRate = 30.0;
    int width = 1920;
    int height = 1080;
    int colorSpace = 0;
};

struct TimelineRange {
    double inTime = 0.0;
    double outTime = 0.0;
};

struct SourceRange {
    double sourceIn = 0.0;
    double sourceOut = 0.0;
};

struct ClipDescriptor {
    int trackIndex = 0;
    VideoClipSourceType sourceType = VideoClipSourceType::Video;
    QString sourcePath;
    TimelineRange timelineRange;
    SourceRange sourceRange;
    double speed = 1.0;
    double opacity = 1.0;
    int blendMode = 0;
    int effectHash = 0;
};

struct MediaInfo {
    double durationSeconds = 0.0;
    int width = 0;
    int height = 0;
    double frameRate = 0.0;
    int frameCount = 0;
    bool hasAudio = false;
    int audioSampleRate = 0;
    int audioChannels = 0;
    double audioDurationSeconds = 0.0;
};

struct VideoExportConfig {
    int width = 0;
    int height = 0;
    double frameRate = 30.0;
    int videoCodec = 0;    // 0=H.264, 1=H.265, 2=VP9
    int videoBitrateBps = 0;
    int audioCodec = 0;    // 0=AAC, 1=Opus
    int audioBitrateKbps = 0;
    int container = 0;     // 0=MP4, 1=MOV, 2=WebM
    QString outputPath;
};

// =========================================================================
// Phase 3: Engine Effect definitions (video engine registry)
// =========================================================================

struct EngineEffectParamDef {
    QString id;
    QString displayName;
    double defaultValue = 0.0;
    double minValue = 0.0;
    double maxValue = 1.0;
    bool keyframeable = true;
};

struct EngineEffectDef {
    QString id;
    QString displayName;
    QString category;
    std::vector<EngineEffectParamDef> params;
    bool gpuAccelerated = false;
};

// =========================================================================
// Phase 4: Masking & Tracking data models
// =========================================================================

struct MaskPoint {
    double x = 0.0;
    double y = 0.0;
    double handleInX = 0.0;
    double handleInY = 0.0;
    double handleOutX = 0.0;
    double handleOutY = 0.0;
};

struct MaskData {
    VideoMaskType type = VideoMaskType::Rectangle;
    std::vector<MaskPoint> points;
    double feather = 0.0;
    double opacity = 1.0;
    bool inverted = false;
    double expansion = 0.0;
};

struct TrackPoint {
    double time = 0.0;
    double x = 0.0;
    double y = 0.0;
    double confidence = 1.0;
};

struct StabilizationConfig {
    StabilizationMethod method = StabilizationMethod::TranslationRotation;
    double smoothness = 0.5;
    bool cropToStable = true;
};

// =========================================================================
// Phase 5: Text, Shape, Audio Effect data models
// =========================================================================

struct TextLayerData {
    QString text;
    QString fontFamily = QStringLiteral("Inter");
    QString fontStyle = QStringLiteral("Regular");
    double fontSize = 48.0;
    uint32_t fillColor = 0xFFFFFFFF;
    bool fillEnabled = true;
    uint32_t strokeColor = 0xFF000000;
    double strokeWidth = 0.0;
    bool strokeEnabled = false;
    TextAlignment alignment = TextAlignment::Center;
    double tracking = 0.0;
    double leading = 0.0;
    double positionX = 0.5;
    double positionY = 0.5;
    double rotation = 0.0;
    double scaleX = 1.0;
    double scaleY = 1.0;
};

struct ShapeData {
    ShapeType type = ShapeType::Rectangle;
    double x = 0.5;
    double y = 0.5;
    double width = 0.3;
    double height = 0.3;
    double rotation = 0.0;
    uint32_t fillColor = 0xFFFFFFFF;
    bool fillEnabled = true;
    uint32_t strokeColor = 0xFF000000;
    double strokeWidth = 2.0;
    bool strokeEnabled = true;
    double cornerRadius = 0.0;
    int sides = 6;
    double innerRadius = 0.5;
    std::vector<MaskPoint> pathPoints;
};

struct AudioEffectDef {
    QString id;
    QString displayName;
    QString category;
    std::vector<EngineEffectParamDef> params;
};

// =========================================================================
// Phase 6: AI, Proxy, Multi-Cam data models
// =========================================================================

struct AiSegmentationConfig {
    AiSegmentationType type = AiSegmentationType::Background;
    double edgeFeather = 2.0;
    bool refineEdges = true;
};

struct ProxyConfig {
    ProxyResolution resolution = ProxyResolution::Quarter;
    int videoCodec = 0;
    int bitrateBps = 2000000;
};

struct CameraAngle {
    QString name;
    QString sourcePath;
    double syncOffset = 0.0;
};

struct MultiCamConfig {
    QString name;
    std::vector<CameraAngle> angles;
    double durationSec = 0.0;
};

// =========================================================================
// Decoder Pool & Thumbnail Generator
// =========================================================================

struct NativeThumbnailResult {
    QByteArray jpegData;
    int width = 0;
    int height = 0;
    double timestamp = 0.0;
};

struct NativeThumbnailRequest {
    QString sourcePath;
    double sourceDuration = 0.0;
    int count = 0;
    int thumbWidth = 160;
    int thumbHeight = 90;
    DecoderPriority priority = DecoderPriority::Medium;
};

// Forward declarations for engine classes defined in engine_api.h
class ImageEditorEngine;
class VideoTimelineEngine;

} // namespace gopost::rendering
