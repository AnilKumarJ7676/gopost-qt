#include "rendering_bridge/image_editor_engine_native.h"

#include <QFile>
#include <QImage>

#include <algorithm>
#include <cstring>

// The actual native struct layouts — must match gopost_engine.h
// These are minimal placeholders; the real definitions come from the
// linked gopost_engine C library headers.

namespace gopost::rendering {

// =========================================================================
// Enum conversion helpers
// =========================================================================

int GopostImageEditorEngineNative::toColorSpace(CanvasColorSpace cs) {
    switch (cs) {
    case CanvasColorSpace::Srgb:      return 0;
    case CanvasColorSpace::DisplayP3: return 1;
    case CanvasColorSpace::AdobeRgb:  return 2;
    }
    return 0;
}

LayerType GopostImageEditorEngineNative::fromLayerType(int t) {
    switch (t) {
    case 0: return LayerType::Image;
    case 1: return LayerType::SolidColor;
    case 2: return LayerType::Text;
    case 3: return LayerType::Shape;
    case 4: return LayerType::Group;
    case 5: return LayerType::Adjustment;
    case 6: return LayerType::Gradient;
    case 7: return LayerType::Sticker;
    default: return LayerType::Image;
    }
}

int GopostImageEditorEngineNative::toBlendMode(BlendMode m) {
    switch (m) {
    case BlendMode::Normal:   return 0;
    case BlendMode::Multiply: return 1;
    case BlendMode::Screen:   return 2;
    case BlendMode::Overlay:  return 3;
    }
    return 0;
}

BlendMode GopostImageEditorEngineNative::fromBlendMode(int m) {
    switch (m) {
    case 0: return BlendMode::Normal;
    case 1: return BlendMode::Multiply;
    case 2: return BlendMode::Screen;
    case 3: return BlendMode::Overlay;
    default: return BlendMode::Normal;
    }
}

int GopostImageEditorEngineNative::toExportFormat(ExportFormat f) {
    switch (f) {
    case ExportFormat::Jpeg: return 1;
    case ExportFormat::Png:  return 2;
    case ExportFormat::Webp: return 3;
    }
    return 1;
}

int GopostImageEditorEngineNative::toExportResolution(ExportResolution r) {
    switch (r) {
    case ExportResolution::Original:       return 0;
    case ExportResolution::Res4k:          return 1;
    case ExportResolution::Res1080p:       return 2;
    case ExportResolution::Res720p:        return 3;
    case ExportResolution::InstagramSquare: return 4;
    case ExportResolution::InstagramStory:  return 5;
    case ExportResolution::Custom:          return 6;
    }
    return 0;
}

// =========================================================================
// Error checking
// =========================================================================

void GopostImageEditorEngineNative::checkErr(int err) {
    if (err != 0) {
        const char* msg = gopost_error_string(err);
        throw EngineException(msg ? msg : "Unknown engine error");
    }
}

void GopostImageEditorEngineNative::ensureInitialized() {
    if (!initialized_ || enginePtr_ == nullptr) {
        throw EngineException("GopostImageEditorEngineNative not initialized");
    }
}

GopostCanvas* GopostImageEditorEngineNative::getCanvas(int canvasId) {
    auto it = canvases_.find(canvasId);
    if (it == canvases_.end()) return nullptr;
    return it->second;
}

// =========================================================================
// Constructor / Destructor
// =========================================================================

GopostImageEditorEngineNative::GopostImageEditorEngineNative() = default;

GopostImageEditorEngineNative::~GopostImageEditorEngineNative() {
    dispose();
}

void GopostImageEditorEngineNative::initialize() {
    if (initialized_) return;

    GopostEngineConfig config{};
    config.threadCount = 4;
    config.framePoolSizeMb = 128;
    config.enableGpu = 0;
    config.logLevel = 2;

    GopostEngine* ptr = nullptr;
    int err = gopost_engine_create(&ptr, &config);
    if (err != 0) {
        throw EngineException(gopost_error_string(err));
    }

    enginePtr_ = ptr;
    initialized_ = true;
}

void GopostImageEditorEngineNative::dispose() {
    if (!initialized_) return;
    for (auto& [id, canvasPtr] : canvases_) {
        gopost_canvas_destroy(canvasPtr);
    }
    canvases_.clear();
    gopost_engine_destroy(enginePtr_);
    enginePtr_ = nullptr;
    initialized_ = false;
}

// =========================================================================
// CanvasManager
// =========================================================================

int GopostImageEditorEngineNative::createCanvas(const CanvasConfig& config) {
    if (!initialized_) initialize();

    GopostCanvasConfig nativeConfig{};
    nativeConfig.width = config.width;
    nativeConfig.height = config.height;
    nativeConfig.dpi = static_cast<float>(config.dpi);
    nativeConfig.colorSpace = toColorSpace(config.colorSpace);
    nativeConfig.bgR = static_cast<float>(config.bgR);
    nativeConfig.bgG = static_cast<float>(config.bgG);
    nativeConfig.bgB = static_cast<float>(config.bgB);
    nativeConfig.bgA = static_cast<float>(config.bgA);
    nativeConfig.transparentBg = config.transparentBackground ? 1 : 0;

    GopostCanvas* canvasPtr = nullptr;
    int err = gopost_canvas_create(enginePtr_, &nativeConfig, &canvasPtr);
    if (err != 0) checkErr(err);

    int id = nextCanvasId_++;
    canvases_[id] = canvasPtr;
    return id;
}

void GopostImageEditorEngineNative::destroyCanvas(int canvasId) {
    ensureInitialized();
    auto it = canvases_.find(canvasId);
    if (it != canvases_.end()) {
        gopost_canvas_destroy(it->second);
        canvases_.erase(it);
    }
}

CanvasSize GopostImageEditorEngineNative::getCanvasSize(int canvasId) {
    ensureInitialized();
    auto* canvas = getCanvas(canvasId);
    if (!canvas) throw EngineException("Canvas not found");

    int w = 0, h = 0;
    float dpi = 0.0f;
    checkErr(gopost_canvas_get_size(canvas, &w, &h, &dpi));
    return {w, h, static_cast<double>(dpi)};
}

void GopostImageEditorEngineNative::resizeCanvas(int canvasId, int width,
                                                  int height) {
    ensureInitialized();
    auto* canvas = getCanvas(canvasId);
    if (!canvas) throw EngineException("Canvas not found");
    checkErr(gopost_canvas_resize(canvas, width, height));
}

// =========================================================================
// LayerManager
// =========================================================================

int GopostImageEditorEngineNative::addImageLayer(int canvasId,
                                                  const QByteArray& rgbaPixels,
                                                  int width, int height,
                                                  int index) {
    ensureInitialized();
    auto* canvas = getCanvas(canvasId);
    if (!canvas) throw EngineException("Canvas not found");

    int outId = 0;
    checkErr(gopost_canvas_add_image_layer(
        canvas,
        reinterpret_cast<const uint8_t*>(rgbaPixels.constData()),
        width, height, index, &outId));
    return outId;
}

int GopostImageEditorEngineNative::addSolidLayer(int canvasId, double r,
                                                  double g, double b, double a,
                                                  int width, int height,
                                                  int index) {
    ensureInitialized();
    auto* canvas = getCanvas(canvasId);
    if (!canvas) throw EngineException("Canvas not found");

    int outId = 0;
    checkErr(gopost_canvas_add_solid_layer(canvas, r, g, b, a, width, height,
                                           index, &outId));
    return outId;
}

int GopostImageEditorEngineNative::addGroupLayer(int canvasId,
                                                  const QString& name,
                                                  int index) {
    ensureInitialized();
    auto* canvas = getCanvas(canvasId);
    if (!canvas) throw EngineException("Canvas not found");

    auto nameUtf8 = name.toUtf8();
    int outId = 0;
    checkErr(gopost_canvas_add_group_layer(canvas, nameUtf8.constData(), index,
                                           &outId));
    return outId;
}

void GopostImageEditorEngineNative::removeLayer(int canvasId, int layerId) {
    ensureInitialized();
    auto* canvas = getCanvas(canvasId);
    if (!canvas) throw EngineException("Canvas not found");
    checkErr(gopost_canvas_remove_layer(canvas, layerId));
}

void GopostImageEditorEngineNative::reorderLayer(int canvasId, int layerId,
                                                  int newIndex) {
    ensureInitialized();
    auto* canvas = getCanvas(canvasId);
    if (!canvas) throw EngineException("Canvas not found");
    checkErr(gopost_canvas_reorder_layer(canvas, layerId, newIndex));
}

int GopostImageEditorEngineNative::duplicateLayer(int canvasId, int layerId) {
    ensureInitialized();
    auto* canvas = getCanvas(canvasId);
    if (!canvas) throw EngineException("Canvas not found");
    int outId = 0;
    checkErr(gopost_canvas_duplicate_layer(canvas, layerId, &outId));
    return outId;
}

int GopostImageEditorEngineNative::getLayerCount(int canvasId) {
    ensureInitialized();
    auto* canvas = getCanvas(canvasId);
    if (!canvas) throw EngineException("Canvas not found");
    int count = 0;
    checkErr(gopost_canvas_get_layer_count(canvas, &count));
    return count;
}

LayerInfo GopostImageEditorEngineNative::getLayerInfo(int canvasId,
                                                       int layerId) {
    ensureInitialized();
    auto* canvas = getCanvas(canvasId);
    if (!canvas) throw EngineException("Canvas not found");

    GopostLayerInfo nativeInfo{};
    checkErr(gopost_canvas_get_layer_info(canvas, layerId, &nativeInfo));

    LayerInfo info;
    info.id = nativeInfo.id;
    info.type = fromLayerType(nativeInfo.type);
    // Read fixed-length name from native struct
    info.name = QString::fromUtf8(nativeInfo.name,
                                  static_cast<int>(strnlen(nativeInfo.name, 128)));
    info.opacity = nativeInfo.opacity;
    info.blendMode = fromBlendMode(nativeInfo.blendMode);
    info.visible = nativeInfo.visible != 0;
    info.locked = nativeInfo.locked != 0;
    info.tx = nativeInfo.tx;
    info.ty = nativeInfo.ty;
    info.sx = nativeInfo.sx;
    info.sy = nativeInfo.sy;
    info.rotation = nativeInfo.rotation;
    info.contentWidth = nativeInfo.contentW;
    info.contentHeight = nativeInfo.contentH;
    return info;
}

std::vector<int> GopostImageEditorEngineNative::getLayerIds(int canvasId) {
    ensureInitialized();
    auto* canvas = getCanvas(canvasId);
    if (!canvas) throw EngineException("Canvas not found");
    int count = getLayerCount(canvasId);
    if (count == 0) return {};
    std::vector<int> ids(count);
    checkErr(gopost_canvas_get_layer_ids(canvas, ids.data(), count));
    return ids;
}

// =========================================================================
// LayerPropertyEditor
// =========================================================================

void GopostImageEditorEngineNative::setLayerVisible(int canvasId, int layerId,
                                                     bool visible) {
    ensureInitialized();
    auto* canvas = getCanvas(canvasId);
    if (!canvas) throw EngineException("Canvas not found");
    checkErr(gopost_layer_set_visible(canvas, layerId, visible ? 1 : 0));
}

void GopostImageEditorEngineNative::setLayerLocked(int canvasId, int layerId,
                                                    bool locked) {
    ensureInitialized();
    auto* canvas = getCanvas(canvasId);
    if (!canvas) throw EngineException("Canvas not found");
    checkErr(gopost_layer_set_locked(canvas, layerId, locked ? 1 : 0));
}

void GopostImageEditorEngineNative::setLayerOpacity(int canvasId, int layerId,
                                                     double opacity) {
    ensureInitialized();
    auto* canvas = getCanvas(canvasId);
    if (!canvas) throw EngineException("Canvas not found");
    checkErr(gopost_layer_set_opacity(canvas, layerId, opacity));
}

void GopostImageEditorEngineNative::setLayerBlendMode(int canvasId,
                                                       int layerId,
                                                       BlendMode blendMode) {
    ensureInitialized();
    auto* canvas = getCanvas(canvasId);
    if (!canvas) throw EngineException("Canvas not found");
    checkErr(gopost_layer_set_blend_mode(canvas, layerId,
                                         toBlendMode(blendMode)));
}

void GopostImageEditorEngineNative::setLayerName(int canvasId, int layerId,
                                                  const QString& name) {
    ensureInitialized();
    auto* canvas = getCanvas(canvasId);
    if (!canvas) throw EngineException("Canvas not found");
    auto nameUtf8 = name.toUtf8();
    checkErr(gopost_layer_set_name(canvas, layerId, nameUtf8.constData()));
}

void GopostImageEditorEngineNative::setLayerTransform(int canvasId,
                                                       int layerId, double tx,
                                                       double ty, double sx,
                                                       double sy,
                                                       double rotation) {
    ensureInitialized();
    auto* canvas = getCanvas(canvasId);
    if (!canvas) throw EngineException("Canvas not found");
    checkErr(gopost_layer_set_transform(canvas, layerId, tx, ty, sx, sy,
                                        rotation));
}

// =========================================================================
// CanvasRenderer
// =========================================================================

DecodedImage GopostImageEditorEngineNative::renderCanvas(int canvasId) {
    ensureInitialized();
    auto* canvas = getCanvas(canvasId);
    if (!canvas) throw EngineException("Canvas not found");

    GopostFrame* framePtr = nullptr;
    checkErr(gopost_canvas_render(canvas, &framePtr));
    if (!framePtr) throw EngineException("Render returned null frame");

    // Read frame data
    int w = framePtr->width;
    int h = framePtr->height;
    int stride = framePtr->stride;
    int size = h * stride;

    QByteArray pixels(size, '\0');
    if (framePtr->data && size > 0) {
        std::memcpy(pixels.data(), framePtr->data, size);
    }

    auto* engine = gopost_canvas_get_engine(canvas);
    if (engine) {
        gopost_frame_release(engine, framePtr);
    } else {
        gopost_render_frame_free(framePtr);
    }

    return {w, h, pixels};
}

void GopostImageEditorEngineNative::invalidateCanvas(int canvasId) {
    ensureInitialized();
    auto* canvas = getCanvas(canvasId);
    if (canvas) gopost_canvas_invalidate(canvas);
}

void GopostImageEditorEngineNative::invalidateLayer(int canvasId,
                                                     int layerId) {
    ensureInitialized();
    auto* canvas = getCanvas(canvasId);
    if (canvas) gopost_canvas_invalidate_layer(canvas, layerId);
}

// =========================================================================
// GpuTextureOutput (stub — GPU texture not yet wired in Qt)
// =========================================================================

GpuTextureResult GopostImageEditorEngineNative::renderToGpuTexture(
    int canvasId) {
    auto sz = getCanvasSize(canvasId);
    return {0, sz.width, sz.height};
}

void GopostImageEditorEngineNative::setViewport(int /*canvasId*/,
                                                 const ViewportState& /*vp*/) {
}

ViewportState GopostImageEditorEngineNative::getViewport(int /*canvasId*/) {
    return {};
}

// =========================================================================
// ImageDecoder
// =========================================================================

ImageInfo GopostImageEditorEngineNative::probeImage(const QByteArray& data) {
    ensureInitialized();
    GopostImageInfo native{};
    checkErr(gopost_image_probe(
        reinterpret_cast<const uint8_t*>(data.constData()),
        static_cast<int>(data.size()), &native));

    static constexpr ImageFormat formatMap[] = {
        ImageFormat::Jpeg, // 0 — unused
        ImageFormat::Jpeg, // 1
        ImageFormat::Png,  // 2
        ImageFormat::Webp, // 3
        ImageFormat::Heic, // 4
        ImageFormat::Bmp,  // 5
        ImageFormat::Gif,  // 6
        ImageFormat::Tiff, // 7
        ImageFormat::Tga,  // 8
    };

    ImageInfo info;
    info.width = native.width;
    info.height = native.height;
    info.channels = native.channels;
    info.format = (native.format >= 1 && native.format <= 8)
                      ? formatMap[native.format]
                      : ImageFormat::Jpeg;
    info.hasAlpha = native.hasAlpha != 0;
    return info;
}

DecodedImage GopostImageEditorEngineNative::decodeImage(
    const QByteArray& data) {
    // Use Qt's QImage for decoding
    QImage img;
    img.loadFromData(reinterpret_cast<const uchar*>(data.constData()),
                     static_cast<int>(data.size()));
    if (img.isNull()) throw EngineException("Failed to decode image");

    img = img.convertToFormat(QImage::Format_RGBA8888);
    QByteArray pixels(reinterpret_cast<const char*>(img.constBits()),
                      img.sizeInBytes());
    return {img.width(), img.height(), pixels};
}

DecodedImage GopostImageEditorEngineNative::decodeImageResized(
    const QByteArray& data, int maxWidth, int maxHeight) {
    QImage img;
    img.loadFromData(reinterpret_cast<const uchar*>(data.constData()),
                     static_cast<int>(data.size()));
    if (img.isNull()) throw EngineException("Failed to decode image");

    if (img.width() > maxWidth || img.height() > maxHeight) {
        img = img.scaled(maxWidth, maxHeight, Qt::KeepAspectRatio,
                         Qt::SmoothTransformation);
    }
    img = img.convertToFormat(QImage::Format_RGBA8888);
    QByteArray pixels(reinterpret_cast<const char*>(img.constBits()),
                      img.sizeInBytes());
    return {img.width(), img.height(), pixels};
}

DecodedImage GopostImageEditorEngineNative::decodeImageFile(
    const QString& path) {
    QString filePath = path;
    if (path.startsWith(QStringLiteral("file://"))) {
        filePath = QUrl(path).toLocalFile();
    }

    QImage img(filePath);
    if (img.isNull()) throw EngineException("Failed to load image file");

    img = img.convertToFormat(QImage::Format_RGBA8888);
    QByteArray pixels(reinterpret_cast<const char*>(img.constBits()),
                      img.sizeInBytes());
    return {img.width(), img.height(), pixels};
}

// =========================================================================
// ImageEncoder
// =========================================================================

QByteArray GopostImageEditorEngineNative::encodeJpeg(const DecodedImage& image,
                                                      int quality) {
    ensureInitialized();
    // Use the native encoder via C API
    GopostFrame frame{};
    frame.width = image.width;
    frame.height = image.height;
    frame.format = 0; // RGBA8
    frame.stride = image.width * 4;
    frame.dataSize = static_cast<int>(image.pixels.size());
    frame.data = const_cast<uint8_t*>(
        reinterpret_cast<const uint8_t*>(image.pixels.constData()));

    GopostJpegOpts opts{};
    opts.quality = quality;
    opts.progressive = 0;

    uint8_t* outData = nullptr;
    uint64_t outSize = 0;
    int err = gopost_image_encode_jpeg(&frame, &opts, &outData, &outSize);
    if (err != 0) checkErr(err);

    QByteArray result(reinterpret_cast<const char*>(outData),
                      static_cast<int>(outSize));
    gopost_image_encode_free(outData);
    return result;
}

QByteArray GopostImageEditorEngineNative::encodePng(const DecodedImage& image) {
    ensureInitialized();
    GopostFrame frame{};
    frame.width = image.width;
    frame.height = image.height;
    frame.format = 0;
    frame.stride = image.width * 4;
    frame.dataSize = static_cast<int>(image.pixels.size());
    frame.data = const_cast<uint8_t*>(
        reinterpret_cast<const uint8_t*>(image.pixels.constData()));

    GopostPngOpts opts{};
    opts.compressionLevel = 6;

    uint8_t* outData = nullptr;
    uint64_t outSize = 0;
    int err = gopost_image_encode_png(&frame, &opts, &outData, &outSize);
    if (err != 0) checkErr(err);

    QByteArray result(reinterpret_cast<const char*>(outData),
                      static_cast<int>(outSize));
    gopost_image_encode_free(outData);
    return result;
}

QByteArray GopostImageEditorEngineNative::encodeWebp(const DecodedImage& image,
                                                      int quality,
                                                      bool lossless) {
    ensureInitialized();
    GopostFrame frame{};
    frame.width = image.width;
    frame.height = image.height;
    frame.format = 0;
    frame.stride = image.width * 4;
    frame.dataSize = static_cast<int>(image.pixels.size());
    frame.data = const_cast<uint8_t*>(
        reinterpret_cast<const uint8_t*>(image.pixels.constData()));

    GopostWebpOpts opts{};
    opts.quality = quality;
    opts.lossless = lossless ? 1 : 0;

    uint8_t* outData = nullptr;
    uint64_t outSize = 0;
    int err = gopost_image_encode_webp(&frame, &opts, &outData, &outSize);
    if (err != 0) checkErr(err);

    QByteArray result(reinterpret_cast<const char*>(outData),
                      static_cast<int>(outSize));
    gopost_image_encode_free(outData);
    return result;
}

void GopostImageEditorEngineNative::encodeToFile(const DecodedImage& image,
                                                  const QString& path,
                                                  ImageFormat format,
                                                  int quality) {
    ensureInitialized();
    GopostFrame frame{};
    frame.width = image.width;
    frame.height = image.height;
    frame.format = 0;
    frame.stride = image.width * 4;
    frame.dataSize = static_cast<int>(image.pixels.size());
    frame.data = const_cast<uint8_t*>(
        reinterpret_cast<const uint8_t*>(image.pixels.constData()));

    static const std::map<ImageFormat, int> formatMap = {
        {ImageFormat::Jpeg, 1}, {ImageFormat::Png, 2}, {ImageFormat::Webp, 3},
        {ImageFormat::Heic, 4}, {ImageFormat::Bmp, 5},
    };

    auto pathUtf8 = path.toUtf8();
    auto it = formatMap.find(format);
    int nativeFormat = (it != formatMap.end()) ? it->second : 1;
    checkErr(gopost_image_encode_to_file(&frame, pathUtf8.constData(),
                                         nativeFormat, quality));
}

// =========================================================================
// EffectRegistry
// =========================================================================

void GopostImageEditorEngineNative::initEffects() {
    ensureInitialized();
    gopost_effects_init(enginePtr_);
}

void GopostImageEditorEngineNative::shutdownEffects() {
    ensureInitialized();
    gopost_effects_shutdown(enginePtr_);
}

std::vector<EffectDef> GopostImageEditorEngineNative::getAllEffects() {
    ensureInitialized();
    int count = 0;
    gopost_effects_get_count(enginePtr_, &count);

    std::vector<EffectDef> defs;
    defs.reserve(count);
    for (int i = 0; i < count; i++) {
        GopostEffectDef native{};
        gopost_effects_get_def(enginePtr_, i, &native);
        // Parse native → EffectDef (structure depends on C layout)
        EffectDef def;
        def.id = QString::fromUtf8(native.id, static_cast<int>(strnlen(native.id, 64)));
        def.displayName = QString::fromUtf8(native.displayName,
                                            static_cast<int>(strnlen(native.displayName, 64)));
        int catIdx = std::clamp(native.category, 0,
                                static_cast<int>(EffectCategory::Preset));
        def.category = static_cast<EffectCategory>(catIdx);
        def.gpuAccelerated = native.gpuAccelerated != 0;
        defs.push_back(std::move(def));
    }
    return defs;
}

std::optional<EffectDef> GopostImageEditorEngineNative::findEffect(
    const QString& effectId) {
    ensureInitialized();
    auto idUtf8 = effectId.toUtf8();
    GopostEffectDef native{};
    int rc = gopost_effects_find(enginePtr_, idUtf8.constData(), &native);
    if (rc != 0) return std::nullopt;

    EffectDef def;
    def.id = QString::fromUtf8(native.id, static_cast<int>(strnlen(native.id, 64)));
    def.displayName = QString::fromUtf8(native.displayName,
                                        static_cast<int>(strnlen(native.displayName, 64)));
    int catIdx = std::clamp(native.category, 0,
                            static_cast<int>(EffectCategory::Preset));
    def.category = static_cast<EffectCategory>(catIdx);
    def.gpuAccelerated = native.gpuAccelerated != 0;
    return def;
}

std::vector<EffectDef> GopostImageEditorEngineNative::getEffectsByCategory(
    EffectCategory category) {
    ensureInitialized();
    constexpr int maxDefs = 64;
    std::vector<GopostEffectDef> nativeDefs(maxDefs);
    int count = 0;
    gopost_effects_list_category(enginePtr_, static_cast<int>(category),
                                 nativeDefs.data(), maxDefs, &count);

    std::vector<EffectDef> result;
    result.reserve(count);
    for (int i = 0; i < count; i++) {
        EffectDef def;
        def.id = QString::fromUtf8(nativeDefs[i].id,
                                   static_cast<int>(strnlen(nativeDefs[i].id, 64)));
        def.displayName = QString::fromUtf8(
            nativeDefs[i].displayName,
            static_cast<int>(strnlen(nativeDefs[i].displayName, 64)));
        int catIdx = std::clamp(nativeDefs[i].category, 0,
                                static_cast<int>(EffectCategory::Preset));
        def.category = static_cast<EffectCategory>(catIdx);
        def.gpuAccelerated = nativeDefs[i].gpuAccelerated != 0;
        result.push_back(std::move(def));
    }
    return result;
}

// =========================================================================
// LayerEffectManager
// =========================================================================

int GopostImageEditorEngineNative::addEffectToLayer(int canvasId, int layerId,
                                                     const QString& effectId) {
    ensureInitialized();
    auto* canvas = getCanvas(canvasId);
    if (!canvas) throw EngineException("Canvas not found");
    auto idUtf8 = effectId.toUtf8();
    int outId = 0;
    gopost_layer_add_effect(canvas, layerId, idUtf8.constData(), &outId);
    return outId;
}

void GopostImageEditorEngineNative::removeEffectFromLayer(int canvasId,
                                                           int layerId,
                                                           int instanceId) {
    ensureInitialized();
    auto* canvas = getCanvas(canvasId);
    if (!canvas) return;
    gopost_layer_remove_effect(canvas, layerId, instanceId);
}

std::vector<EffectInstance> GopostImageEditorEngineNative::getLayerEffects(
    int canvasId, int layerId) {
    ensureInitialized();
    auto* canvas = getCanvas(canvasId);
    if (!canvas) return {};
    int count = 0;
    gopost_layer_get_effect_count(canvas, layerId, &count);

    std::vector<EffectInstance> result;
    result.reserve(count);
    for (int i = 0; i < count; i++) {
        GopostEffectInstance native{};
        gopost_layer_get_effect(canvas, layerId, i, &native);
        EffectInstance inst;
        inst.instanceId = native.instanceId;
        inst.effectId = QString::fromUtf8(native.effectId,
                                          static_cast<int>(strnlen(native.effectId, 64)));
        inst.enabled = native.enabled != 0;
        inst.mix = native.mix;
        result.push_back(std::move(inst));
    }
    return result;
}

void GopostImageEditorEngineNative::setEffectEnabled(int canvasId, int layerId,
                                                      int instanceId,
                                                      bool enabled) {
    ensureInitialized();
    auto* canvas = getCanvas(canvasId);
    if (!canvas) return;
    gopost_effect_set_enabled(canvas, layerId, instanceId, enabled ? 1 : 0);
}

void GopostImageEditorEngineNative::setEffectMix(int canvasId, int layerId,
                                                  int instanceId, double mix) {
    ensureInitialized();
    auto* canvas = getCanvas(canvasId);
    if (!canvas) return;
    gopost_effect_set_mix(canvas, layerId, instanceId, mix);
}

void GopostImageEditorEngineNative::setEffectParam(int canvasId, int layerId,
                                                    int instanceId,
                                                    const QString& paramId,
                                                    double value) {
    ensureInitialized();
    auto* canvas = getCanvas(canvasId);
    if (!canvas) return;
    auto idUtf8 = paramId.toUtf8();
    gopost_effect_set_param(canvas, layerId, instanceId, idUtf8.constData(),
                            value);
}

double GopostImageEditorEngineNative::getEffectParam(int canvasId, int layerId,
                                                      int instanceId,
                                                      const QString& paramId) {
    ensureInitialized();
    auto* canvas = getCanvas(canvasId);
    if (!canvas) return 0.0;
    auto idUtf8 = paramId.toUtf8();
    float val = 0.0f;
    gopost_effect_get_param(canvas, layerId, instanceId, idUtf8.constData(),
                            &val);
    return static_cast<double>(val);
}

// =========================================================================
// PresetFilterEngine
// =========================================================================

std::vector<PresetFilterInfo> GopostImageEditorEngineNative::getPresetFilters() {
    ensureInitialized();
    int count = 0;
    gopost_preset_get_count(enginePtr_, &count);

    std::vector<PresetFilterInfo> result;
    result.reserve(count);
    for (int i = 0; i < count; i++) {
        char name[128] = {};
        char cat[64] = {};
        gopost_preset_get_info(enginePtr_, i, name, 128, cat, 64);
        result.push_back({i, QString::fromUtf8(name), QString::fromUtf8(cat)});
    }
    return result;
}

std::optional<DecodedImage> GopostImageEditorEngineNative::applyPreset(
    int canvasId, int layerId, int presetIndex, double intensity) {
    ensureInitialized();
    auto* canvas = getCanvas(canvasId);
    if (!canvas) return std::nullopt;

    int count = 0;
    checkErr(gopost_preset_get_count(enginePtr_, &count));
    if (presetIndex < 0 || presetIndex >= count) return std::nullopt;

    GopostFrame* framePtr = nullptr;
    checkErr(gopost_canvas_render(canvas, &framePtr));
    if (!framePtr) return std::nullopt;

    int err = gopost_preset_apply(enginePtr_, framePtr, presetIndex, intensity);
    if (err != 0) {
        auto* engine = gopost_canvas_get_engine(canvas);
        if (engine) gopost_frame_release(engine, framePtr);
        else gopost_render_frame_free(framePtr);
        checkErr(err);
    }

    int w = framePtr->width;
    int h = framePtr->height;
    int stride = framePtr->stride;
    int size = h * stride;

    QByteArray pixels(size, '\0');
    if (framePtr->data && size > 0) {
        std::memcpy(pixels.data(), framePtr->data, size);
    }

    auto* engine = gopost_canvas_get_engine(canvas);
    if (engine) gopost_frame_release(engine, framePtr);
    else gopost_render_frame_free(framePtr);

    return DecodedImage{w, h, pixels};
}

// =========================================================================
// TextManager (stub)
// =========================================================================

void GopostImageEditorEngineNative::initText() {}
void GopostImageEditorEngineNative::shutdownText() {}

std::vector<QString> GopostImageEditorEngineNative::getAvailableFonts() {
    return {QStringLiteral("System")};
}

int GopostImageEditorEngineNative::addTextLayer(int /*canvasId*/,
                                                 const TextConfig& /*config*/,
                                                 int /*maxWidth*/,
                                                 int /*index*/) {
    return 0;
}

void GopostImageEditorEngineNative::updateTextLayer(
    int /*canvasId*/, int /*layerId*/, const TextConfig& /*config*/,
    int /*maxWidth*/) {}

// =========================================================================
// ImageExporter
// =========================================================================

ExportResult GopostImageEditorEngineNative::exportToFile(
    int canvasId, const ExportConfig& config, const QString& outputPath,
    std::function<void(double)> onProgress) {
    if (!initialized_) initialize();
    auto* canvas = getCanvas(canvasId);
    if (!canvas) throw EngineException("Canvas not found");

    GopostExportConfig nativeConfig{};
    nativeConfig.format = toExportFormat(config.format);
    nativeConfig.quality = config.quality;
    nativeConfig.resolution = toExportResolution(config.resolution);
    nativeConfig.customWidth = config.customWidth;
    nativeConfig.customHeight = config.customHeight;
    nativeConfig.dpi = static_cast<float>(config.dpi);
    nativeConfig.embedColorProfile = 0;

    auto pathUtf8 = outputPath.toUtf8();

    // Progress callback
    // Note: in C++ we pass a lambda via a static — same pattern as FFI.
    static std::function<void(double)> sProgressCallback;
    sProgressCallback = onProgress;

    auto progressCb = +[](float progress, void* /*userData*/) -> int {
        if (sProgressCallback) sProgressCallback(static_cast<double>(progress));
        return 0;
    };

    int err = gopost_export_to_file(canvas, &nativeConfig,
                                    pathUtf8.constData(),
                                    reinterpret_cast<void*>(progressCb),
                                    nullptr);
    sProgressCallback = nullptr;
    if (err != 0) checkErr(err);

    int fileSz = estimateFileSize(canvasId, config);
    auto sz = getCanvasSize(canvasId);
    return {outputPath, fileSz, sz.width, sz.height};
}

int GopostImageEditorEngineNative::estimateFileSize(int canvasId,
                                                     const ExportConfig& config) {
    if (!initialized_) initialize();
    auto* canvas = getCanvas(canvasId);
    if (!canvas) return 0;

    GopostExportConfig nativeConfig{};
    nativeConfig.format = toExportFormat(config.format);
    nativeConfig.quality = config.quality;
    nativeConfig.resolution = toExportResolution(config.resolution);
    nativeConfig.customWidth = config.customWidth;
    nativeConfig.customHeight = config.customHeight;
    nativeConfig.dpi = static_cast<float>(config.dpi);
    nativeConfig.embedColorProfile = 0;

    uint64_t outSize = 0;
    int err = gopost_export_estimate_size(canvas, &nativeConfig, &outSize);
    if (err != 0) return 0;
    return static_cast<int>(outSize);
}

// =========================================================================
// MaskManager (stub)
// =========================================================================

void GopostImageEditorEngineNative::addMask(int, int, MaskType) {}
void GopostImageEditorEngineNative::removeMask(int, int) {}
bool GopostImageEditorEngineNative::hasMask(int, int) { return false; }
void GopostImageEditorEngineNative::invertMask(int, int) {}
void GopostImageEditorEngineNative::setMaskEnabled(int, int, bool) {}
void GopostImageEditorEngineNative::maskPaint(int, int, double, double, double,
                                               double, MaskBrushMode, double) {
}
void GopostImageEditorEngineNative::maskFill(int, int, int) {}

// =========================================================================
// ProjectManager (stub)
// =========================================================================

void GopostImageEditorEngineNative::saveProject(int, const QString&) {}
int GopostImageEditorEngineNative::loadProject(const QString&) { return 0; }

} // namespace gopost::rendering
