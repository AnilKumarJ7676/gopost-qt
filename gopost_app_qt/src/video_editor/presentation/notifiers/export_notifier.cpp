#include "video_editor/presentation/notifiers/export_notifier.h"
#include "video_editor/data/services/ffmpeg_export_service.h"
#include "video_editor/presentation/notifiers/timeline_notifier.h"

#include <QDebug>
#include <QDesktopServices>
#include <QDir>
#include <QDateTime>
#include <QFileInfo>
#include <QStandardPaths>
#include <QUrl>
#include <QtConcurrent>
#include <stdexcept>

namespace gopost::video_editor {

ExportNotifier::ExportNotifier(QObject* parent)
    : QObject(parent)
    , exportService_(std::make_unique<FFmpegExportService>())
{
    qDebug() << "[Export] ExportNotifier created";
}

ExportNotifier::~ExportNotifier() {
    if (state_.isExporting) {
        exportService_->cancel();
    }
}

void ExportNotifier::selectPreset(int presetId) {
    state_.selectedPreset = static_cast<ExportPresetId>(presetId);
    emit stateChanged();
}

void ExportNotifier::startExport() {
    auto preset = ExportPreset::byId(state_.selectedPreset);
    auto ext = containerFormatExtension(preset.container);
    auto videosDir = QStandardPaths::writableLocation(QStandardPaths::MoviesLocation);
    QDir().mkpath(videosDir);
    auto defaultPath = videosDir + QStringLiteral("/gopost_export_")
                       + QDateTime::currentDateTime().toString(QStringLiteral("yyyyMMdd_HHmmss"))
                       + ext;
    qDebug() << "[Export] startExport (no-arg) -> defaultPath:" << defaultPath;
    startExport(defaultPath);
}

void ExportNotifier::startExport(const QString& outputPath) {
    if (state_.isExporting) return;

    // Get project from timeline notifier
    if (!timelineNotifier_) {
        state_.error = QStringLiteral("No timeline available for export");
        emit stateChanged();
        qWarning() << "[Export] No timelineNotifier set";
        return;
    }

    const auto& tlState = timelineNotifier_->currentState();
    if (!tlState.project) {
        state_.error = QStringLiteral("No project loaded");
        emit stateChanged();
        return;
    }

    state_.isExporting = true;
    state_.progress    = 0.0;
    state_.phase       = ExportPhase::Preparing;
    state_.result.clear();
    state_.error.clear();
    elapsedTimer_.start();
    emit stateChanged();

    // Resolve output path
    QString resolvedPath = outputPath;
    if (resolvedPath.isEmpty()) {
        auto preset = ExportPreset::byId(state_.selectedPreset);
        auto ext = containerFormatExtension(preset.container);
        auto videosDir = QStandardPaths::writableLocation(QStandardPaths::MoviesLocation);
        QDir().mkpath(videosDir);
        resolvedPath = videosDir + QStringLiteral("/gopost_export") + ext;
    }

    // Copy project data for thread safety
    auto project = *tlState.project;
    auto preset = ExportPreset::byId(state_.selectedPreset);
    auto* service = exportService_.get();

    qDebug() << "[ExportNotifier] Starting export to:" << resolvedPath
             << "preset:" << static_cast<int>(state_.selectedPreset);

    // Run export on background thread
    QtConcurrent::run([this, service, project, preset, resolvedPath]() {
        try {
            service->exportProject(project, preset, resolvedPath,
                [this](double percent) {
                    // Update progress on main thread
                    QMetaObject::invokeMethod(this, [this, percent]() {
                        if (!state_.isExporting) return;
                        state_.progress = std::clamp(percent / 100.0, 0.0, 0.99);
                        // Infer phase from progress
                        if (percent < 5.0)
                            state_.phase = ExportPhase::Preparing;
                        else if (percent < 90.0)
                            state_.phase = ExportPhase::Encoding;
                        else if (percent < 98.0)
                            state_.phase = ExportPhase::Muxing;
                        else
                            state_.phase = ExportPhase::Finalizing;
                        emit stateChanged();
                    }, Qt::QueuedConnection);
                });

            QMetaObject::invokeMethod(this, [this, resolvedPath]() {
                onExportFinished(resolvedPath);
            }, Qt::QueuedConnection);

        } catch (const std::exception& e) {
            QString errorMsg = QString::fromStdString(e.what());
            QMetaObject::invokeMethod(this, [this, errorMsg]() {
                onExportError(errorMsg);
            }, Qt::QueuedConnection);
        }
    });
}

void ExportNotifier::cancelExport() {
    if (!state_.isExporting) return;
    exportService_->cancel();
    state_.isExporting = false;
    state_.error = QStringLiteral("Export cancelled");
    qDebug() << "[ExportNotifier] Export cancelled";
    emit stateChanged();
}

void ExportNotifier::reset() {
    if (state_.isExporting) {
        exportService_->cancel();
    }
    state_ = ExportState{};
    emit stateChanged();
}

void ExportNotifier::onExportFinished(const QString& outputPath) {
    state_.isExporting = false;
    state_.progress = 1.0;
    state_.phase = ExportPhase::Done;
    state_.result = outputPath;

    // Populate result data
    QFileInfo fi(outputPath);
    state_.exportResult.filePath = outputPath;
    state_.exportResult.fileSizeBytes = fi.size();
    state_.exportResult.exportTime = std::chrono::milliseconds(elapsedTimer_.elapsed());

    auto preset = ExportPreset::byId(state_.selectedPreset);
    state_.exportResult.width = preset.width;
    state_.exportResult.height = preset.height;

    qDebug() << "[Export] Export completed:" << outputPath
             << "size:" << state_.exportResult.fileSizeFormatted()
             << "time:" << formatDuration(elapsedTimer_.elapsed());
    emit stateChanged();
}

void ExportNotifier::onExportError(const QString& errorMsg) {
    state_.isExporting = false;
    state_.phase = ExportPhase::Failed;
    state_.error = errorMsg;
    qWarning() << "[Export] Export failed:" << errorMsg;
    emit stateChanged();
}

// ---------------------------------------------------------------------------
// New property implementations for VideoExportScreen.qml
// ---------------------------------------------------------------------------

QVariantList ExportNotifier::presetsVariant() const {
    QVariantList result;
    static const QStringList iconChars = {
        QStringLiteral("\U0001F4F1"), // Instagram
        QStringLiteral("\U0001F3B5"), // TikTok
        QStringLiteral("\u25B6"),     // YouTube 4K
        QStringLiteral("\u25B6"),     // YouTube 1080p
        QStringLiteral("\U0001F3A5"), // General HD
        QStringLiteral("\u2699"),     // Custom
    };
    const auto& presets = ExportPreset::all();
    for (int i = 0; i < presets.size(); ++i) {
        const auto& p = presets[i];
        QVariantMap m;
        m[QStringLiteral("id")] = static_cast<int>(p.id);
        m[QStringLiteral("name")] = p.name;
        m[QStringLiteral("description")] = p.description;
        m[QStringLiteral("width")] = p.width;
        m[QStringLiteral("height")] = p.height;
        m[QStringLiteral("videoCodec")] = videoCodecLabel(p.videoCodec);
        m[QStringLiteral("bitrate")] = QStringLiteral("%1 Mbps").arg(p.videoBitrateMbps);
        m[QStringLiteral("iconChar")] = (i < iconChars.size()) ? iconChars[i] : QStringLiteral("\U0001F3A5");
        m[QStringLiteral("isSelected")] = (p.id == state_.selectedPreset);
        result.append(m);
    }
    return result;
}

bool ExportNotifier::hasResult() const {
    return !state_.result.isEmpty() && !state_.isExporting;
}

QString ExportNotifier::presetInfo() const {
    auto preset = ExportPreset::byId(state_.selectedPreset);
    return QStringLiteral("%1x%2 %3 @ %4 Mbps")
        .arg(preset.width).arg(preset.height)
        .arg(videoCodecLabel(preset.videoCodec))
        .arg(preset.videoBitrateMbps);
}

QString ExportNotifier::estimatedSize() const {
    if (!timelineNotifier_) return QStringLiteral("--");
    auto preset = ExportPreset::byId(state_.selectedPreset);
    // Rough estimate: bitrate * duration
    double durationSec = timelineNotifier_->totalDuration();
    double sizeMB = (preset.videoBitrateMbps * durationSec) / 8.0;
    if (sizeMB < 1.0)
        return QStringLiteral("< 1 MB");
    if (sizeMB >= 1024.0)
        return QStringLiteral("%1 GB").arg(sizeMB / 1024.0, 0, 'f', 1);
    return QStringLiteral("%1 MB").arg(sizeMB, 0, 'f', 0);
}

double ExportNotifier::progressPercent() const {
    return std::clamp(state_.progress * 100.0, 0.0, 100.0);
}

QString ExportNotifier::phaseLabel() const {
    switch (state_.phase) {
    case ExportPhase::Preparing:  return QStringLiteral("Preparing...");
    case ExportPhase::Encoding:   return QStringLiteral("Encoding video...");
    case ExportPhase::Muxing:     return QStringLiteral("Muxing audio...");
    case ExportPhase::Finalizing: return QStringLiteral("Finalizing...");
    case ExportPhase::Done:       return QStringLiteral("Export complete");
    case ExportPhase::Failed:     return QStringLiteral("Export failed");
    case ExportPhase::Cancelled:  return QStringLiteral("Export cancelled");
    }
    return QString();
}

QString ExportNotifier::estimatedRemaining() const {
    if (!state_.isExporting || state_.progress <= 0.01)
        return QStringLiteral("Calculating...");
    qint64 elapsedMs = elapsedTimer_.elapsed();
    qint64 totalEstMs = static_cast<qint64>(elapsedMs / state_.progress);
    qint64 remainingMs = totalEstMs - elapsedMs;
    return formatDuration(std::max<qint64>(0, remainingMs));
}

QString ExportNotifier::elapsed() const {
    if (!state_.isExporting && !hasResult())
        return QStringLiteral("00:00");
    return formatDuration(elapsedTimer_.elapsed());
}

QString ExportNotifier::resultFileName() const {
    if (state_.result.isEmpty()) return QString();
    return QFileInfo(state_.result).fileName();
}

QVariantList ExportNotifier::resultInfoVariant() const {
    QVariantList result;
    if (!hasResult()) return result;

    const auto& r = state_.exportResult;
    auto addRow = [&](const QString& label, const QString& value) {
        QVariantMap row;
        row[QStringLiteral("label")] = label;
        row[QStringLiteral("value")] = value;
        result.append(row);
    };

    addRow(QStringLiteral("File"), QFileInfo(r.filePath).fileName());
    addRow(QStringLiteral("Size"), r.fileSizeFormatted());
    addRow(QStringLiteral("Resolution"), QStringLiteral("%1x%2").arg(r.width).arg(r.height));
    addRow(QStringLiteral("Export time"), formatDuration(r.exportTime.count()));

    auto preset = ExportPreset::byId(state_.selectedPreset);
    addRow(QStringLiteral("Codec"), videoCodecLabel(preset.videoCodec));
    addRow(QStringLiteral("Bitrate"), QStringLiteral("%1 Mbps").arg(preset.videoBitrateMbps));

    return result;
}

void ExportNotifier::shareResult() {
    if (state_.result.isEmpty()) {
        qWarning() << "[Export] shareResult: no result file";
        return;
    }
    qDebug() << "[Export] shareResult: opening" << state_.result;
    QDesktopServices::openUrl(QUrl::fromLocalFile(QFileInfo(state_.result).absolutePath()));
}

QString ExportNotifier::formatDuration(qint64 ms) {
    qint64 totalSec = ms / 1000;
    int min = static_cast<int>(totalSec / 60);
    int sec = static_cast<int>(totalSec % 60);
    return QStringLiteral("%1:%2")
        .arg(min, 2, 10, QLatin1Char('0'))
        .arg(sec, 2, 10, QLatin1Char('0'));
}

} // namespace gopost::video_editor
