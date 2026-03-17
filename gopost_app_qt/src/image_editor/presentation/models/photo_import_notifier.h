#pragma once

#include <QObject>
#include <QString>

#include "rendering_bridge/engine_types.h"

namespace gopost::rendering {
class ImageEditorEngine;
}

namespace gopost::image_editor {

// -------------------------------------------------------------------------
// PhotoImportNotifier — manages the photo import workflow
// -------------------------------------------------------------------------

class PhotoImportNotifier : public QObject {
    Q_OBJECT

    Q_PROPERTY(int status READ status NOTIFY stateChanged)
    Q_PROPERTY(QString selectedPath READ selectedPath NOTIFY stateChanged)
    Q_PROPERTY(QString error READ error NOTIFY stateChanged)
    Q_PROPERTY(bool hasDecodedImage READ hasDecodedImage NOTIFY stateChanged)

public:
    enum ImportStatus {
        Idle = 0,
        Picking,
        Decoding,
        Done,
        Error
    };
    Q_ENUM(ImportStatus)

    explicit PhotoImportNotifier(rendering::ImageEditorEngine* engine,
                                 QObject* parent = nullptr);

    int status() const { return m_status; }
    QString selectedPath() const { return m_selectedPath; }
    QString error() const { return m_error; }
    bool hasDecodedImage() const { return m_decodedImage.has_value(); }

    const std::optional<rendering::DecodedImage>& decodedImage() const {
        return m_decodedImage;
    }

    Q_INVOKABLE void importFromPath(const QString& path);
    Q_INVOKABLE void reset();

signals:
    void stateChanged();

private:
    rendering::ImageEditorEngine* m_engine = nullptr;

    ImportStatus m_status = Idle;
    QString m_selectedPath;
    std::optional<rendering::DecodedImage> m_decodedImage;
    QString m_error;
};

} // namespace gopost::image_editor
