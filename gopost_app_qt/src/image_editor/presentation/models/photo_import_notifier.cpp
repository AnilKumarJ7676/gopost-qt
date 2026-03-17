#include "image_editor/presentation/models/photo_import_notifier.h"
#include "rendering_bridge/engine_api.h"

namespace gopost::image_editor {

PhotoImportNotifier::PhotoImportNotifier(rendering::ImageEditorEngine* engine,
                                         QObject* parent)
    : QObject(parent)
    , m_engine(engine)
{
}

void PhotoImportNotifier::importFromPath(const QString& path)
{
    if (!m_engine) return;

    m_status = Decoding;
    m_selectedPath = path;
    m_error.clear();
    emit stateChanged();

    try {
        auto decoded = m_engine->decodeImageFile(path);
        m_status = Done;
        m_decodedImage = decoded;
        emit stateChanged();
    } catch (const std::exception& e) {
        m_status = Error;
        m_error = QString::fromStdString(e.what());
        emit stateChanged();
    }
}

void PhotoImportNotifier::reset()
{
    m_status = Idle;
    m_selectedPath.clear();
    m_decodedImage.reset();
    m_error.clear();
    emit stateChanged();
}

} // namespace gopost::image_editor
