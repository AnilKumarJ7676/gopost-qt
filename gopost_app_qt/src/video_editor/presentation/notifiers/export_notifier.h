#pragma once

#include <QObject>
#include <QString>
#include <QThread>
#include <QElapsedTimer>
#include <QVariantList>
#include <QVariantMap>
#include <optional>
#include <memory>

#include "video_editor/domain/models/export_config.h"

namespace gopost::video_editor {

class FFmpegExportService;
class TimelineNotifier;

// ---------------------------------------------------------------------------
// ExportState
// ---------------------------------------------------------------------------
struct ExportState {
    ExportPresetId selectedPreset = ExportPresetId::GeneralHD;
    double progress    = 0.0;   // 0..1
    bool   isExporting = false;
    QString result;              // output path when done
    QString error;
    ExportPhase phase  = ExportPhase::Preparing;
    ExportResult exportResult;   // final result data
};

// ---------------------------------------------------------------------------
// ExportNotifier -- preset selection, progress tracking, export lifecycle
//
// Converted 1:1 from export_notifier.dart.
// Now wired to FFmpegExportService for real FFmpeg-based exports.
// ---------------------------------------------------------------------------
class ExportNotifier : public QObject {
    Q_OBJECT

    Q_PROPERTY(int    selectedPreset READ selectedPresetInt NOTIFY stateChanged)
    Q_PROPERTY(double progress       READ progress          NOTIFY stateChanged)
    Q_PROPERTY(bool   isExporting    READ isExporting       NOTIFY stateChanged)
    Q_PROPERTY(QString result        READ result            NOTIFY stateChanged)
    Q_PROPERTY(QString error         READ error             NOTIFY stateChanged)

    // --- Additional properties for VideoExportScreen.qml ---
    Q_PROPERTY(QVariantList presets       READ presetsVariant    NOTIFY stateChanged)
    Q_PROPERTY(bool   hasResult          READ hasResult         NOTIFY stateChanged)
    Q_PROPERTY(QString presetInfo        READ presetInfo        NOTIFY stateChanged)
    Q_PROPERTY(QString estimatedSize     READ estimatedSize     NOTIFY stateChanged)
    Q_PROPERTY(double progressPercent    READ progressPercent   NOTIFY stateChanged)
    Q_PROPERTY(QString phaseLabel        READ phaseLabel        NOTIFY stateChanged)
    Q_PROPERTY(QString estimatedRemaining READ estimatedRemaining NOTIFY stateChanged)
    Q_PROPERTY(QString elapsed           READ elapsed           NOTIFY stateChanged)
    Q_PROPERTY(QString resultFileName    READ resultFileName    NOTIFY stateChanged)
    Q_PROPERTY(QVariantList resultInfo   READ resultInfoVariant NOTIFY stateChanged)

public:
    explicit ExportNotifier(QObject* parent = nullptr);
    ~ExportNotifier() override;

    // Existing property accessors
    int     selectedPresetInt() const { return static_cast<int>(state_.selectedPreset); }
    double  progress()          const { return state_.progress; }
    bool    isExporting()       const { return state_.isExporting; }
    QString result()            const { return state_.result; }
    QString error()             const { return state_.error; }

    // New property accessors
    QVariantList presetsVariant()    const;
    bool         hasResult()         const;
    QString      presetInfo()        const;
    QString      estimatedSize()     const;
    double       progressPercent()   const;
    QString      phaseLabel()        const;
    QString      estimatedRemaining() const;
    QString      elapsed()           const;
    QString      resultFileName()    const;
    QVariantList resultInfoVariant()  const;

    /// Set the timeline notifier for accessing the current project.
    void setTimelineNotifier(TimelineNotifier* tn) { timelineNotifier_ = tn; }

    Q_INVOKABLE void selectPreset(int presetId);
    Q_INVOKABLE void startExport(const QString& outputPath);
    Q_INVOKABLE void startExport();  // no-arg overload: uses default output path
    Q_INVOKABLE void cancelExport();
    Q_INVOKABLE void reset();
    Q_INVOKABLE void shareResult();

signals:
    void stateChanged();

private:
    ExportState state_;
    QElapsedTimer elapsedTimer_;
    std::unique_ptr<FFmpegExportService> exportService_;
    TimelineNotifier* timelineNotifier_ = nullptr;
    QThread* exportThread_ = nullptr;

    void onExportFinished(const QString& outputPath);
    void onExportError(const QString& errorMsg);

    static QString formatDuration(qint64 ms);
};

} // namespace gopost::video_editor
