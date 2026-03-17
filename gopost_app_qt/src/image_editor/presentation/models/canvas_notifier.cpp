#include "image_editor/presentation/models/canvas_notifier.h"
#include "rendering_bridge/engine_api.h"

#include <QVariantList>
#include <QVariantMap>
#include <QDebug>

namespace gopost::image_editor {

CanvasNotifier::CanvasNotifier(rendering::ImageEditorEngine* engine,
                               QObject* parent)
    : QObject(parent)
    , m_engine(engine)
{
}

void CanvasNotifier::createCanvas(int width, int height, double dpi,
                                  bool transparentBackground)
{
    if (!m_engine) return;

    m_state.isLoading = true;
    m_state.error.clear();
    emit stateChanged();

    try {
        rendering::CanvasConfig config;
        config.width = width;
        config.height = height;
        config.dpi = dpi;
        config.transparentBackground = transparentBackground;

        int id = m_engine->createCanvas(config);

        m_state.hasCanvas = true;
        m_state.canvasId = id;
        m_state.canvasWidth = width;
        m_state.canvasHeight = height;
        m_state.canvas = CanvasEntity(id, width, height, dpi,
                                       rendering::CanvasColorSpace::Srgb,
                                       transparentBackground);
        m_state.isLoading = false;
        emit stateChanged();
    } catch (const std::exception& e) {
        m_state.isLoading = false;
        m_state.error = QString::fromStdString(e.what());
        emit stateChanged();
    }
}

void CanvasNotifier::destroyCanvas()
{
    if (!m_state.hasCanvas || !m_engine) return;

    try {
        m_engine->destroyCanvas(m_state.canvasId);
        m_state = CanvasState{};
        emit stateChanged();
    } catch (const std::exception& e) {
        m_state.error = QString::fromStdString(e.what());
        emit stateChanged();
    }
}

int CanvasNotifier::addImageLayer(const QByteArray& pixels, int width, int height)
{
    if (!m_state.hasCanvas || !m_engine) return -1;

    m_state.isLoading = true;
    emit stateChanged();

    try {
        const int canvasId = m_state.canvasId;
        const bool isFirstLayer = m_state.layers.isEmpty();

        if (isFirstLayer) {
            m_engine->resizeCanvas(canvasId, width, height);
            m_state.canvasWidth = width;
            m_state.canvasHeight = height;
            if (m_state.canvas) {
                m_state.canvas = m_state.canvas->copyWith(width, height);
            }
        }

        int layerId = m_engine->addImageLayer(canvasId, pixels, width, height);
        rendering::LayerInfo info = m_engine->getLayerInfo(canvasId, layerId);
        m_state.layers.append(info);
        m_state.selectedLayerId = layerId;
        m_state.isLoading = false;
        emit stateChanged();
        return layerId;
    } catch (const std::exception& e) {
        m_state.isLoading = false;
        m_state.error = QString::fromStdString(e.what());
        emit stateChanged();
        return -1;
    }
}

int CanvasNotifier::addSolidLayer(double r, double g, double b, double a)
{
    if (!m_state.hasCanvas || !m_engine) return -1;

    m_state.isLoading = true;
    emit stateChanged();

    try {
        int layerId = m_engine->addSolidLayer(
            m_state.canvasId, r, g, b, a,
            m_state.canvasWidth, m_state.canvasHeight);
        rendering::LayerInfo info = m_engine->getLayerInfo(m_state.canvasId, layerId);
        m_state.layers.append(info);
        m_state.selectedLayerId = layerId;
        m_state.isLoading = false;
        emit stateChanged();
        return layerId;
    } catch (const std::exception& e) {
        m_state.isLoading = false;
        m_state.error = QString::fromStdString(e.what());
        emit stateChanged();
        return -1;
    }
}

void CanvasNotifier::removeLayer(int layerId)
{
    if (!m_state.hasCanvas || !m_engine) return;

    try {
        m_engine->removeLayer(m_state.canvasId, layerId);
        m_state.layers.erase(
            std::remove_if(m_state.layers.begin(), m_state.layers.end(),
                           [layerId](const rendering::LayerInfo& l) {
                               return l.id == layerId;
                           }),
            m_state.layers.end());
        if (m_state.selectedLayerId == layerId) {
            m_state.selectedLayerId = -1;
        }
        emit stateChanged();
    } catch (const std::exception& e) {
        m_state.error = QString::fromStdString(e.what());
        emit stateChanged();
    }
}

void CanvasNotifier::reorderLayer(int layerId, int newIndex)
{
    if (!m_state.hasCanvas || !m_engine) return;

    try {
        m_engine->reorderLayer(m_state.canvasId, layerId, newIndex);
        refreshLayers();
    } catch (const std::exception& e) {
        m_state.error = QString::fromStdString(e.what());
        emit stateChanged();
    }
}

void CanvasNotifier::duplicateLayer(int layerId)
{
    if (!m_state.hasCanvas || !m_engine) return;

    try {
        int newId = m_engine->duplicateLayer(m_state.canvasId, layerId);
        rendering::LayerInfo info = m_engine->getLayerInfo(m_state.canvasId, newId);
        m_state.layers.append(info);
        m_state.selectedLayerId = newId;
        emit stateChanged();
    } catch (const std::exception& e) {
        m_state.error = QString::fromStdString(e.what());
        emit stateChanged();
    }
}

void CanvasNotifier::selectLayer(int layerId)
{
    m_state.selectedLayerId = layerId;
    emit stateChanged();
}

void CanvasNotifier::setSelectedLayerId(int layerId)
{
    selectLayer(layerId);
}

void CanvasNotifier::setLayerOpacity(int layerId, double opacity)
{
    if (!m_state.hasCanvas || !m_engine) return;

    try {
        m_engine->setLayerOpacity(m_state.canvasId, layerId, opacity);
        updateLayerInState(layerId, [opacity](rendering::LayerInfo& l) {
            l.opacity = opacity;
        });
    } catch (const std::exception& e) {
        m_state.error = QStringLiteral("Failed to set opacity: ") +
                        QString::fromStdString(e.what());
        emit stateChanged();
    }
}

void CanvasNotifier::setLayerVisible(int layerId, bool visible)
{
    if (!m_state.hasCanvas || !m_engine) return;

    try {
        m_engine->setLayerVisible(m_state.canvasId, layerId, visible);
        updateLayerInState(layerId, [visible](rendering::LayerInfo& l) {
            l.visible = visible;
        });
    } catch (const std::exception& e) {
        m_state.error = QStringLiteral("Failed to set visibility: ") +
                        QString::fromStdString(e.what());
        emit stateChanged();
    }
}

void CanvasNotifier::setLayerBlendMode(int layerId, int mode)
{
    if (!m_state.hasCanvas || !m_engine) return;

    try {
        auto blendMode = static_cast<rendering::BlendMode>(mode);
        m_engine->setLayerBlendMode(m_state.canvasId, layerId, blendMode);
        updateLayerInState(layerId, [blendMode](rendering::LayerInfo& l) {
            l.blendMode = blendMode;
        });
    } catch (const std::exception& e) {
        m_state.error = QStringLiteral("Failed to set blend mode: ") +
                        QString::fromStdString(e.what());
        emit stateChanged();
    }
}

void CanvasNotifier::updateViewport(double px, double py, double z)
{
    m_state.panX = px;
    m_state.panY = py;
    m_state.zoom = z;
    emit viewportChanged();
}

void CanvasNotifier::refreshPreview()
{
    m_state.revision++;
    emit stateChanged();
}

bool CanvasNotifier::replaceWithImage(const rendering::DecodedImage& image)
{
    if (!m_state.hasCanvas || !m_engine) return false;

    m_state.isLoading = true;
    m_state.error.clear();
    emit stateChanged();

    try {
        const int canvasId = m_state.canvasId;

        // Remove all existing layers
        for (const auto& layer : m_state.layers) {
            m_engine->removeLayer(canvasId, layer.id);
        }

        // Resize canvas to match image
        m_engine->resizeCanvas(canvasId, image.width, image.height);

        // Add the new image layer
        int layerId = m_engine->addImageLayer(canvasId, image.pixels,
                                               image.width, image.height);
        rendering::LayerInfo info = m_engine->getLayerInfo(canvasId, layerId);

        m_state.canvasWidth = image.width;
        m_state.canvasHeight = image.height;
        if (m_state.canvas) {
            m_state.canvas = m_state.canvas->copyWith(image.width, image.height);
        }
        m_state.layers.clear();
        m_state.layers.append(info);
        m_state.selectedLayerId = layerId;
        m_state.isLoading = false;
        m_state.revision++;
        emit stateChanged();
        return true;
    } catch (const std::exception& e) {
        m_state.isLoading = false;
        m_state.error = QString::fromStdString(e.what());
        emit stateChanged();
        return false;
    }
}

QVariantList CanvasNotifier::layerList() const
{
    QVariantList result;
    for (const auto& layer : m_state.layers) {
        QVariantMap map;
        map[QStringLiteral("id")] = layer.id;
        map[QStringLiteral("type")] = static_cast<int>(layer.type);
        map[QStringLiteral("name")] = layer.name;
        map[QStringLiteral("opacity")] = layer.opacity;
        map[QStringLiteral("blendMode")] = static_cast<int>(layer.blendMode);
        map[QStringLiteral("visible")] = layer.visible;
        map[QStringLiteral("locked")] = layer.locked;
        map[QStringLiteral("contentWidth")] = layer.contentWidth;
        map[QStringLiteral("contentHeight")] = layer.contentHeight;
        result.append(map);
    }
    return result;
}

QVariantMap CanvasNotifier::layerAt(int index) const
{
    QVariantMap map;
    if (index < 0 || index >= m_state.layers.size()) return map;

    const auto& layer = m_state.layers[index];
    map[QStringLiteral("id")] = layer.id;
    map[QStringLiteral("type")] = static_cast<int>(layer.type);
    map[QStringLiteral("name")] = layer.name;
    map[QStringLiteral("opacity")] = layer.opacity;
    map[QStringLiteral("blendMode")] = static_cast<int>(layer.blendMode);
    map[QStringLiteral("visible")] = layer.visible;
    map[QStringLiteral("locked")] = layer.locked;
    map[QStringLiteral("contentWidth")] = layer.contentWidth;
    map[QStringLiteral("contentHeight")] = layer.contentHeight;
    return map;
}

void CanvasNotifier::refreshLayers()
{
    if (!m_state.hasCanvas || !m_engine) return;

    try {
        auto ids = m_engine->getLayerIds(m_state.canvasId);
        m_state.layers.clear();
        for (int id : ids) {
            m_state.layers.append(
                m_engine->getLayerInfo(m_state.canvasId, id));
        }
        emit stateChanged();
    } catch (const std::exception& e) {
        qWarning() << "CanvasNotifier::refreshLayers failed:" << e.what();
    }
}

void CanvasNotifier::updateLayerInState(
    int layerId, std::function<void(rendering::LayerInfo&)> updater)
{
    for (auto& layer : m_state.layers) {
        if (layer.id == layerId) {
            updater(layer);
            break;
        }
    }
    emit stateChanged();
}

} // namespace gopost::image_editor
