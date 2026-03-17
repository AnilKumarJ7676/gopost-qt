#pragma once

#include <QObject>
#include <QList>
#include <QString>
#include <QByteArray>
#include <optional>

#include "rendering_bridge/engine_types.h"
#include "image_editor/domain/entities/canvas_entity.h"

namespace gopost::image_editor {

// -------------------------------------------------------------------------
// CanvasState — immutable value object exposed to QML
// -------------------------------------------------------------------------

struct CanvasState {
    Q_GADGET
    Q_PROPERTY(bool hasCanvas MEMBER hasCanvas)
    Q_PROPERTY(int canvasId MEMBER canvasId)
    Q_PROPERTY(int canvasWidth MEMBER canvasWidth)
    Q_PROPERTY(int canvasHeight MEMBER canvasHeight)
    Q_PROPERTY(int selectedLayerId MEMBER selectedLayerId)
    Q_PROPERTY(bool isLoading MEMBER isLoading)
    Q_PROPERTY(QString error MEMBER error)
    Q_PROPERTY(int revision MEMBER revision)
    Q_PROPERTY(int layerCount READ layerCount)

public:
    bool hasCanvas = false;
    int canvasId = 0;
    int canvasWidth = 0;
    int canvasHeight = 0;
    int selectedLayerId = -1;
    bool isLoading = false;
    QString error;
    int revision = 0;

    double panX = 0.0;
    double panY = 0.0;
    double zoom = 1.0;

    std::optional<CanvasEntity> canvas;
    QList<rendering::LayerInfo> layers;

    int layerCount() const { return layers.size(); }

    rendering::LayerInfo* selectedLayer() {
        if (selectedLayerId < 0) return nullptr;
        for (auto& l : layers) {
            if (l.id == selectedLayerId) return &l;
        }
        return nullptr;
    }
};

// -------------------------------------------------------------------------
// CanvasNotifier — QObject managing canvas lifecycle and layer state
// -------------------------------------------------------------------------

class CanvasNotifier : public QObject {
    Q_OBJECT

    // Canvas properties
    Q_PROPERTY(bool hasCanvas READ hasCanvas NOTIFY stateChanged)
    Q_PROPERTY(int canvasId READ canvasId NOTIFY stateChanged)
    Q_PROPERTY(int canvasWidth READ canvasWidth NOTIFY stateChanged)
    Q_PROPERTY(int canvasHeight READ canvasHeight NOTIFY stateChanged)
    Q_PROPERTY(int selectedLayerId READ selectedLayerId WRITE setSelectedLayerId NOTIFY stateChanged)
    Q_PROPERTY(bool isLoading READ isLoading NOTIFY stateChanged)
    Q_PROPERTY(QString error READ error NOTIFY stateChanged)
    Q_PROPERTY(int revision READ revision NOTIFY stateChanged)
    Q_PROPERTY(int layerCount READ layerCount NOTIFY stateChanged)

    // Viewport properties
    Q_PROPERTY(double panX READ panX NOTIFY viewportChanged)
    Q_PROPERTY(double panY READ panY NOTIFY viewportChanged)
    Q_PROPERTY(double zoom READ zoom NOTIFY viewportChanged)

public:
    explicit CanvasNotifier(rendering::ImageEditorEngine* engine,
                            QObject* parent = nullptr);

    // Property getters
    bool hasCanvas() const { return m_state.hasCanvas; }
    int canvasId() const { return m_state.canvasId; }
    int canvasWidth() const { return m_state.canvasWidth; }
    int canvasHeight() const { return m_state.canvasHeight; }
    int selectedLayerId() const { return m_state.selectedLayerId; }
    bool isLoading() const { return m_state.isLoading; }
    QString error() const { return m_state.error; }
    int revision() const { return m_state.revision; }
    int layerCount() const { return m_state.layers.size(); }
    double panX() const { return m_state.panX; }
    double panY() const { return m_state.panY; }
    double zoom() const { return m_state.zoom; }

    const CanvasState& state() const { return m_state; }
    const QList<rendering::LayerInfo>& layers() const { return m_state.layers; }

    // Invokable methods
    Q_INVOKABLE void createCanvas(int width, int height, double dpi = 72.0,
                                  bool transparentBackground = true);
    Q_INVOKABLE void destroyCanvas();
    Q_INVOKABLE int addImageLayer(const QByteArray& pixels, int width, int height);
    Q_INVOKABLE int addSolidLayer(double r, double g, double b, double a);
    Q_INVOKABLE void removeLayer(int layerId);
    Q_INVOKABLE void reorderLayer(int layerId, int newIndex);
    Q_INVOKABLE void duplicateLayer(int layerId);
    Q_INVOKABLE void selectLayer(int layerId);
    Q_INVOKABLE void setSelectedLayerId(int layerId);
    Q_INVOKABLE void setLayerOpacity(int layerId, double opacity);
    Q_INVOKABLE void setLayerVisible(int layerId, bool visible);
    Q_INVOKABLE void setLayerBlendMode(int layerId, int mode);
    Q_INVOKABLE void updateViewport(double panX, double panY, double zoom);
    Q_INVOKABLE void refreshPreview();
    Q_INVOKABLE bool replaceWithImage(const rendering::DecodedImage& image);

    // Layer info for QML
    Q_INVOKABLE QVariantList layerList() const;
    Q_INVOKABLE QVariantMap layerAt(int index) const;

signals:
    void stateChanged();
    void viewportChanged();

private:
    void refreshLayers();
    void updateLayerInState(int layerId,
                            std::function<void(rendering::LayerInfo&)> updater);

    rendering::ImageEditorEngine* m_engine = nullptr;
    CanvasState m_state;
};

} // namespace gopost::image_editor
