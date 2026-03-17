#include "rendering_bridge/stub_image_editor_engine.h"

#include <QImage>
#include <QUrl>

#include <algorithm>
#include <cmath>
#include <cstring>

namespace gopost::rendering {

// =========================================================================
// StubLayer helpers
// =========================================================================

LayerInfo StubImageEditorEngine::StubLayer::toLayerInfo() const {
    return LayerInfo{
        .id = id,
        .type = type,
        .name = name,
        .opacity = opacity,
        .blendMode = blendMode,
        .visible = visible,
        .locked = locked,
        .tx = tx,
        .ty = ty,
        .sx = sx,
        .sy = sy,
        .rotation = rotation,
        .contentWidth = contentWidth,
        .contentHeight = contentHeight,
    };
}

StubImageEditorEngine::StubLayer* StubImageEditorEngine::findLayer(
    int layerId) {
    for (auto& l : layers_) {
        if (l.id == layerId) return &l;
    }
    return nullptr;
}

// =========================================================================
// CanvasManager
// =========================================================================

int StubImageEditorEngine::createCanvas(const CanvasConfig& config) {
    currentCanvasId_ = nextCanvasId_++;
    canvasWidth_ = config.width;
    canvasHeight_ = config.height;
    canvasDpi_ = config.dpi;
    layers_.clear();
    return currentCanvasId_;
}

void StubImageEditorEngine::destroyCanvas(int canvasId) {
    if (currentCanvasId_ == canvasId) {
        currentCanvasId_ = 0;
        layers_.clear();
    }
}

CanvasSize StubImageEditorEngine::getCanvasSize(int /*canvasId*/) {
    return {canvasWidth_, canvasHeight_, canvasDpi_};
}

void StubImageEditorEngine::resizeCanvas(int /*canvasId*/, int width,
                                          int height) {
    canvasWidth_ = width;
    canvasHeight_ = height;
}

// =========================================================================
// LayerManager
// =========================================================================

int StubImageEditorEngine::addImageLayer(int /*canvasId*/,
                                          const QByteArray& rgbaPixels,
                                          int width, int height, int index) {
    int id = nextLayerId_++;
    StubLayer layer;
    layer.id = id;
    layer.type = LayerType::Image;
    layer.name = QStringLiteral("Layer %1").arg(layers_.size() + 1);
    layer.contentWidth = width;
    layer.contentHeight = height;
    layer.imagePixels = rgbaPixels;

    if (index < 0 || index >= static_cast<int>(layers_.size())) {
        layers_.push_back(std::move(layer));
    } else {
        layers_.insert(layers_.begin() + index, std::move(layer));
    }
    return id;
}

int StubImageEditorEngine::addSolidLayer(int /*canvasId*/, double r, double g,
                                          double b, double a, int width,
                                          int height, int index) {
    int id = nextLayerId_++;
    StubLayer layer;
    layer.id = id;
    layer.type = LayerType::SolidColor;
    layer.name = QStringLiteral("Solid %1").arg(layers_.size() + 1);
    layer.contentWidth = width;
    layer.contentHeight = height;
    layer.solidR = r;
    layer.solidG = g;
    layer.solidB = b;
    layer.solidA = a;

    if (index < 0 || index >= static_cast<int>(layers_.size())) {
        layers_.push_back(std::move(layer));
    } else {
        layers_.insert(layers_.begin() + index, std::move(layer));
    }
    return id;
}

int StubImageEditorEngine::addGroupLayer(int /*canvasId*/, const QString& name,
                                          int index) {
    int id = nextLayerId_++;
    StubLayer layer;
    layer.id = id;
    layer.type = LayerType::Group;
    layer.name = name.isEmpty()
                     ? QStringLiteral("Group %1").arg(layers_.size() + 1)
                     : name;

    if (index < 0 || index >= static_cast<int>(layers_.size())) {
        layers_.push_back(std::move(layer));
    } else {
        layers_.insert(layers_.begin() + index, std::move(layer));
    }
    return id;
}

void StubImageEditorEngine::removeLayer(int /*canvasId*/, int layerId) {
    layers_.erase(
        std::remove_if(layers_.begin(), layers_.end(),
                        [layerId](const StubLayer& l) {
                            return l.id == layerId;
                        }),
        layers_.end());
}

void StubImageEditorEngine::reorderLayer(int /*canvasId*/, int layerId,
                                          int newIndex) {
    auto it = std::find_if(layers_.begin(), layers_.end(),
                           [layerId](const StubLayer& l) {
                               return l.id == layerId;
                           });
    if (it == layers_.end()) return;

    StubLayer layer = std::move(*it);
    layers_.erase(it);
    int insert = std::clamp(newIndex, 0, static_cast<int>(layers_.size()));
    layers_.insert(layers_.begin() + insert, std::move(layer));
}

int StubImageEditorEngine::duplicateLayer(int /*canvasId*/, int layerId) {
    auto* layer = findLayer(layerId);
    if (!layer) return 0;

    int id = nextLayerId_++;
    StubLayer copy = *layer;
    copy.id = id;
    copy.name = layer->name + QStringLiteral(" copy");

    auto it = std::find_if(layers_.begin(), layers_.end(),
                           [layerId](const StubLayer& l) {
                               return l.id == layerId;
                           });
    if (it != layers_.end()) {
        layers_.insert(it + 1, std::move(copy));
    }
    return id;
}

int StubImageEditorEngine::getLayerCount(int /*canvasId*/) {
    return static_cast<int>(layers_.size());
}

LayerInfo StubImageEditorEngine::getLayerInfo(int /*canvasId*/, int layerId) {
    auto* layer = findLayer(layerId);
    if (!layer) throw EngineException("Layer not found");
    return layer->toLayerInfo();
}

std::vector<int> StubImageEditorEngine::getLayerIds(int /*canvasId*/) {
    std::vector<int> ids;
    ids.reserve(layers_.size());
    for (const auto& l : layers_) {
        ids.push_back(l.id);
    }
    return ids;
}

// =========================================================================
// LayerPropertyEditor
// =========================================================================

void StubImageEditorEngine::setLayerVisible(int, int layerId, bool visible) {
    if (auto* l = findLayer(layerId)) l->visible = visible;
}

void StubImageEditorEngine::setLayerLocked(int, int layerId, bool locked) {
    if (auto* l = findLayer(layerId)) l->locked = locked;
}

void StubImageEditorEngine::setLayerOpacity(int, int layerId, double opacity) {
    if (auto* l = findLayer(layerId)) l->opacity = opacity;
}

void StubImageEditorEngine::setLayerBlendMode(int, int layerId,
                                               BlendMode blendMode) {
    if (auto* l = findLayer(layerId)) l->blendMode = blendMode;
}

void StubImageEditorEngine::setLayerName(int, int layerId,
                                          const QString& name) {
    if (auto* l = findLayer(layerId)) l->name = name;
}

void StubImageEditorEngine::setLayerTransform(int, int layerId, double tx,
                                               double ty, double sx, double sy,
                                               double rotation) {
    if (auto* l = findLayer(layerId)) {
        l->tx = tx;
        l->ty = ty;
        l->sx = sx;
        l->sy = sy;
        l->rotation = rotation;
    }
}

// =========================================================================
// CanvasRenderer — software compositing
// =========================================================================

void StubImageEditorEngine::blendSolid(QByteArray& out, int cw, int ch,
                                        double r, double g, double b,
                                        double a) {
    int ir = std::clamp(static_cast<int>(r * 255.0 + 0.5), 0, 255);
    int ig = std::clamp(static_cast<int>(g * 255.0 + 0.5), 0, 255);
    int ib = std::clamp(static_cast<int>(b * 255.0 + 0.5), 0, 255);
    int ia = std::clamp(static_cast<int>(a * 255.0 + 0.5), 0, 255);

    auto* data = reinterpret_cast<uint8_t*>(out.data());
    int total = cw * ch * 4;
    for (int i = 0; i < total; i += 4) {
        double da = data[i + 3] / 255.0;
        double sa = ia / 255.0;
        double outA = sa + da * (1.0 - sa);
        if (outA <= 0) continue;
        data[i] = std::clamp(
            static_cast<int>((ir * sa + data[i] * da * (1 - sa)) / outA + 0.5),
            0, 255);
        data[i + 1] = std::clamp(
            static_cast<int>(
                (ig * sa + data[i + 1] * da * (1 - sa)) / outA + 0.5),
            0, 255);
        data[i + 2] = std::clamp(
            static_cast<int>(
                (ib * sa + data[i + 2] * da * (1 - sa)) / outA + 0.5),
            0, 255);
        data[i + 3] = std::clamp(static_cast<int>(outA * 255.0 + 0.5), 0, 255);
    }
}

void StubImageEditorEngine::blendImage(QByteArray& out, int cw, int ch,
                                        const QByteArray& src, int sw, int sh,
                                        double opacity) {
    if (sw <= 0 || sh <= 0) return;

    double scaleX = static_cast<double>(cw) / sw;
    double scaleY = static_cast<double>(ch) / sh;
    double scale = std::min(scaleX, scaleY);
    int dw = std::clamp(static_cast<int>(sw * scale + 0.5), 1, cw);
    int dh = std::clamp(static_cast<int>(sh * scale + 0.5), 1, ch);
    int left = static_cast<int>((cw - dw) / 2.0 + 0.5);
    int top = static_cast<int>((ch - dh) / 2.0 + 0.5);

    auto* outData = reinterpret_cast<uint8_t*>(out.data());
    auto* srcData = reinterpret_cast<const uint8_t*>(src.constData());

    for (int y = 0; y < ch; y++) {
        for (int x = 0; x < cw; x++) {
            int oi = (y * cw + x) * 4;
            if (x < left || x >= left + dw || y < top || y >= top + dh)
                continue;

            int sxi = std::clamp(
                static_cast<int>((x - left) * static_cast<double>(sw) / dw),
                0, sw - 1);
            int syi = std::clamp(
                static_cast<int>((y - top) * static_cast<double>(sh) / dh),
                0, sh - 1);
            int si = (syi * sw + sxi) * 4;

            double sa = (srcData[si + 3] / 255.0) * opacity;
            double da = outData[oi + 3] / 255.0;
            double outA = sa + da * (1.0 - sa);
            if (outA <= 0) continue;

            outData[oi] = std::clamp(
                static_cast<int>(
                    (srcData[si] * sa + outData[oi] * da * (1 - sa)) / outA +
                    0.5),
                0, 255);
            outData[oi + 1] = std::clamp(
                static_cast<int>(
                    (srcData[si + 1] * sa +
                     outData[oi + 1] * da * (1 - sa)) /
                        outA +
                    0.5),
                0, 255);
            outData[oi + 2] = std::clamp(
                static_cast<int>(
                    (srcData[si + 2] * sa +
                     outData[oi + 2] * da * (1 - sa)) /
                        outA +
                    0.5),
                0, 255);
            outData[oi + 3] =
                std::clamp(static_cast<int>(outA * 255.0 + 0.5), 0, 255);
        }
    }
}

DecodedImage StubImageEditorEngine::renderCanvas(int /*canvasId*/) {
    int w = std::clamp(canvasWidth_, 1, 8192);
    int h = std::clamp(canvasHeight_, 1, 8192);
    QByteArray out(w * h * 4, '\0');

    for (const auto& layer : layers_) {
        if (!layer.visible) continue;
        if (layer.type == LayerType::Image && !layer.imagePixels.isEmpty()) {
            blendImage(out, w, h, layer.imagePixels, layer.contentWidth,
                       layer.contentHeight, layer.opacity);
        } else if (layer.type == LayerType::SolidColor) {
            blendSolid(out, w, h, layer.solidR, layer.solidG, layer.solidB,
                       layer.solidA * layer.opacity);
        }
    }
    return {w, h, out};
}

void StubImageEditorEngine::invalidateCanvas(int) {}
void StubImageEditorEngine::invalidateLayer(int, int) {}

// =========================================================================
// GpuTextureOutput
// =========================================================================

GpuTextureResult StubImageEditorEngine::renderToGpuTexture(int) {
    return {0, canvasWidth_, canvasHeight_};
}

void StubImageEditorEngine::setViewport(int, const ViewportState&) {}

ViewportState StubImageEditorEngine::getViewport(int) { return viewport_; }

// =========================================================================
// ImageDecoder — uses QImage
// =========================================================================

ImageInfo StubImageEditorEngine::probeImage(const QByteArray&) {
    throw EngineException("Stub engine: probe not available");
}

DecodedImage StubImageEditorEngine::decodeImage(const QByteArray& data) {
    QImage img;
    img.loadFromData(reinterpret_cast<const uchar*>(data.constData()),
                     static_cast<int>(data.size()));
    if (img.isNull()) throw EngineException("Failed to decode image");

    img = img.convertToFormat(QImage::Format_RGBA8888);
    QByteArray pixels(reinterpret_cast<const char*>(img.constBits()),
                      img.sizeInBytes());
    return {img.width(), img.height(), pixels};
}

DecodedImage StubImageEditorEngine::decodeImageResized(const QByteArray& data,
                                                        int maxWidth,
                                                        int maxHeight) {
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

DecodedImage StubImageEditorEngine::decodeImageFile(const QString& path) {
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
// ImageEncoder (stubs — return empty data)
// =========================================================================

QByteArray StubImageEditorEngine::encodeJpeg(const DecodedImage&, int) {
    return {};
}
QByteArray StubImageEditorEngine::encodePng(const DecodedImage&) { return {}; }
QByteArray StubImageEditorEngine::encodeWebp(const DecodedImage&, int, bool) {
    return {};
}
void StubImageEditorEngine::encodeToFile(const DecodedImage&, const QString&,
                                          ImageFormat, int) {}

// =========================================================================
// EffectRegistry (stubs)
// =========================================================================

void StubImageEditorEngine::initEffects() {}
void StubImageEditorEngine::shutdownEffects() {}
std::vector<EffectDef> StubImageEditorEngine::getAllEffects() { return {}; }
std::optional<EffectDef> StubImageEditorEngine::findEffect(const QString&) {
    return std::nullopt;
}
std::vector<EffectDef> StubImageEditorEngine::getEffectsByCategory(
    EffectCategory) {
    return {};
}

// =========================================================================
// LayerEffectManager (stubs)
// =========================================================================

int StubImageEditorEngine::addEffectToLayer(int, int, const QString&) {
    return 0;
}
void StubImageEditorEngine::removeEffectFromLayer(int, int, int) {}
std::vector<EffectInstance> StubImageEditorEngine::getLayerEffects(int, int) {
    return {};
}
void StubImageEditorEngine::setEffectEnabled(int, int, int, bool) {}
void StubImageEditorEngine::setEffectMix(int, int, int, double) {}
void StubImageEditorEngine::setEffectParam(int, int, int, const QString&,
                                            double) {}
double StubImageEditorEngine::getEffectParam(int, int, int, const QString&) {
    return 0.0;
}

// =========================================================================
// PresetFilterEngine
// =========================================================================

std::vector<PresetFilterInfo> StubImageEditorEngine::getPresetFilters() {
    return {
        {0, QStringLiteral("Warm"), QStringLiteral("Color")},
        {1, QStringLiteral("Cool"), QStringLiteral("Color")},
        {2, QStringLiteral("Vintage"), QStringLiteral("Style")},
        {3, QStringLiteral("B&W"), QStringLiteral("Style")},
    };
}

std::optional<DecodedImage> StubImageEditorEngine::applyPreset(
    int canvasId, int /*layerId*/, int presetIndex, double intensity) {
    auto current = renderCanvas(canvasId);
    double t = std::clamp(intensity / 100.0, 0.0, 1.0);
    QByteArray out = current.pixels;
    auto* data = reinterpret_cast<uint8_t*>(out.data());
    int n = static_cast<int>(out.size()) / 4;

    for (int i = 0; i < n; i++) {
        int o = i * 4;
        int r = data[o];
        int g = data[o + 1];
        int b = data[o + 2];

        switch (std::clamp(presetIndex, 0, 3)) {
        case 0: // Warm
            r = std::clamp(static_cast<int>(r + (255 - r) * 0.1 * t + 0.5), 0,
                           255);
            b = std::clamp(static_cast<int>(b * (1 - 0.15 * t) + 0.5), 0, 255);
            break;
        case 1: // Cool
            b = std::clamp(static_cast<int>(b + (255 - b) * 0.1 * t + 0.5), 0,
                           255);
            r = std::clamp(static_cast<int>(r * (1 - 0.1 * t) + 0.5), 0, 255);
            break;
        case 2: // Vintage
            r = std::clamp(static_cast<int>(r * 1.1 + 10 * t + 0.5), 0, 255);
            g = std::clamp(static_cast<int>(g * 0.95 + 0.5), 0, 255);
            b = std::clamp(static_cast<int>(b * 0.9 - 5 * t + 0.5), 0, 255);
            break;
        case 3: { // B&W
            int gray = static_cast<int>(0.299 * r + 0.587 * g + 0.114 * b);
            r = std::clamp(static_cast<int>(gray * t + r * (1 - t) + 0.5), 0,
                           255);
            g = std::clamp(static_cast<int>(gray * t + g * (1 - t) + 0.5), 0,
                           255);
            b = std::clamp(static_cast<int>(gray * t + b * (1 - t) + 0.5), 0,
                           255);
            break;
        }
        }

        data[o] = static_cast<uint8_t>(r);
        data[o + 1] = static_cast<uint8_t>(g);
        data[o + 2] = static_cast<uint8_t>(b);
    }

    return DecodedImage{current.width, current.height, out};
}

// =========================================================================
// TextManager (stubs)
// =========================================================================

void StubImageEditorEngine::initText() {}
void StubImageEditorEngine::shutdownText() {}
std::vector<QString> StubImageEditorEngine::getAvailableFonts() {
    return {QStringLiteral("System")};
}
int StubImageEditorEngine::addTextLayer(int, const TextConfig&, int, int) {
    return 0;
}
void StubImageEditorEngine::updateTextLayer(int, int, const TextConfig&, int) {
}

// =========================================================================
// ImageExporter (stub)
// =========================================================================

ExportResult StubImageEditorEngine::exportToFile(int, const ExportConfig&,
                                                  const QString&,
                                                  std::function<void(double)>) {
    return {};
}

int StubImageEditorEngine::estimateFileSize(int, const ExportConfig&) {
    return 0;
}

// =========================================================================
// MaskManager (stubs)
// =========================================================================

void StubImageEditorEngine::addMask(int, int, MaskType) {}
void StubImageEditorEngine::removeMask(int, int) {}
bool StubImageEditorEngine::hasMask(int, int) { return false; }
void StubImageEditorEngine::invertMask(int, int) {}
void StubImageEditorEngine::setMaskEnabled(int, int, bool) {}
void StubImageEditorEngine::maskPaint(int, int, double, double, double, double,
                                       MaskBrushMode, double) {}
void StubImageEditorEngine::maskFill(int, int, int) {}

// =========================================================================
// ProjectManager (stubs)
// =========================================================================

void StubImageEditorEngine::saveProject(int, const QString&) {}
int StubImageEditorEngine::loadProject(const QString&) { return 0; }

} // namespace gopost::rendering
