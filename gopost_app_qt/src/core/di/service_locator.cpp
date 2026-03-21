#include "core/di/service_locator.h"
#include "core/navigation/router.h"
#include "core/config/app_environment.h"
#include "core/security/secure_storage_service.h"
#include "core/network/interceptors/auth_interceptor.h"
#include "core/network/http_client.h"

// Full type definitions required for unique_ptr destructor
#include "auth/presentation/models/auth_notifier.h"
#include "image_editor/presentation/models/canvas_notifier.h"
#include "image_editor/presentation/models/tool_notifier.h"
#include "image_editor/presentation/models/undo_redo_notifier.h"
#include "image_editor/presentation/models/filter_notifier.h"
#include "image_editor/presentation/models/adjustment_notifier.h"
#include "image_editor/presentation/models/photo_import_notifier.h"
#include "image_editor/presentation/models/text_notifier.h"
#include "video_editor/presentation/notifiers/timeline_notifier.h"
#include "video_editor/presentation/notifiers/media_pool_notifier.h"
#include "video_editor/presentation/notifiers/editor_layout_notifier.h"
#include "video_editor/presentation/notifiers/export_notifier.h"
#include "video_editor/presentation/notifiers/project_list_notifier.h"
#include "video_editor/presentation/notifiers/proxy_provider.h"
#include "video_editor/presentation/notifiers/thumbnail_provider.h"
#include "video_editor/presentation/notifiers/video_frame_provider.h"
#include "video_editor/presentation/notifiers/video_thumbnail_provider.h"
#include "video_editor/presentation/notifiers/audio_waveform_provider.h"
#include "template_browser/presentation/models/home_notifier.h"
#include "template_browser/presentation/models/template_list_notifier.h"
#include "template_browser/presentation/models/template_search_notifier.h"
#include "template_browser/presentation/models/download_notifier.h"
#include "go_craft/presentation/models/go_craft_notifier.h"

#include <QQmlContext>

namespace gopost::core {

ServiceLocator::ServiceLocator(QObject* parent)
    : QObject(parent) {}

ServiceLocator::~ServiceLocator() = default;

void ServiceLocator::initialize() {
    // Phase 1: Core services
    m_router = std::make_unique<Router>(this);

    // Phase 4: Auth
    m_authNotifier = std::make_unique<auth::AuthNotifier>(nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, this);

    // Phase 5-6: Image Editor notifiers
    m_canvasNotifier = std::make_unique<image_editor::CanvasNotifier>(nullptr, this);
    m_toolNotifier = std::make_unique<image_editor::ToolNotifier>(this);
    m_undoRedoNotifier = std::make_unique<image_editor::UndoRedoNotifier>(this);
    m_filterNotifier = std::make_unique<image_editor::FilterNotifier>(nullptr, nullptr, this);
    m_adjustmentNotifier = std::make_unique<image_editor::AdjustmentNotifier>(nullptr, this);
    m_photoImportNotifier = std::make_unique<image_editor::PhotoImportNotifier>(nullptr, this);
    m_textToolNotifier = std::make_unique<image_editor::TextNotifier>(nullptr, this);

    // Phase 7-8: Video Editor notifiers
    m_timelineNotifier = std::make_unique<video_editor::TimelineNotifier>(this);
    m_mediaPoolNotifier = std::make_unique<video_editor::MediaPoolNotifier>(this);
    m_editorLayoutNotifier = std::make_unique<video_editor::EditorLayoutNotifier>(this);
    m_exportNotifier = std::make_unique<video_editor::ExportNotifier>(this);
    m_projectListNotifier = std::make_unique<video_editor::ProjectListNotifier>(this);
    m_proxyProvider = std::make_unique<video_editor::ProxyProvider>(this);
    m_thumbnailProvider = std::make_unique<video_editor::ThumbnailProvider>(this);

    // Wire export notifier to timeline for project access
    m_exportNotifier->setTimelineNotifier(m_timelineNotifier.get());

    // Phase 9: Template Browser notifiers
    m_homeNotifier = std::make_unique<template_browser::HomeNotifier>(nullptr, nullptr, nullptr, nullptr, this);
    m_templateListNotifier = std::make_unique<template_browser::TemplateListNotifier>(nullptr, this);
    m_templateSearchNotifier = std::make_unique<template_browser::TemplateSearchNotifier>(nullptr, this);
    m_downloadNotifier = std::make_unique<template_browser::DownloadNotifier>(nullptr, nullptr, this);

    // Phase 10: GoCraft notifier
    m_goCraftNotifier = std::make_unique<go_craft::GoCraftNotifier>(nullptr, nullptr, this);

    // Wire auth state -> router access control
    connect(m_authNotifier.get(), &auth::AuthNotifier::stateChanged,
            this, [this]() {
                m_router->setCanAccessApp(m_authNotifier->canAccessApp());
            });

    // Wire media pool -> timeline: add imported media as clips
    connect(m_mediaPoolNotifier.get(), &video_editor::MediaPoolNotifier::addToTimelineRequested,
            this, [this](const QString& /*assetId*/, const QString& filePath,
                         const QString& mediaType, const QString& displayName,
                         double duration) {
                if (!m_timelineNotifier) {
                    qWarning() << "[ServiceLocator] addToTimeline: no timelineNotifier!";
                    return;
                }
                qDebug() << "[ServiceLocator] addToTimeline:" << filePath
                         << "type:" << mediaType << "duration:" << duration;
                int sourceType = 0; // Video
                int targetTrack = -1;
                if (mediaType == QStringLiteral("video") || mediaType == QStringLiteral("image")) {
                    sourceType = (mediaType == QStringLiteral("image")) ? 1 : 0;
                    // Find first Video track
                    auto tracks = m_timelineNotifier->tracksVariant();
                    for (const auto& t : tracks) {
                        auto tm = t.toMap();
                        if (tm[QStringLiteral("type")].toInt() == 0) { // TrackType::Video
                            targetTrack = tm[QStringLiteral("index")].toInt();
                            break;
                        }
                    }
                } else if (mediaType == QStringLiteral("audio")) {
                    sourceType = 0; // audio clips use Video source type with audio track
                    // Find first Audio track
                    auto tracks = m_timelineNotifier->tracksVariant();
                    for (const auto& t : tracks) {
                        auto tm = t.toMap();
                        if (tm[QStringLiteral("type")].toInt() == 1) { // TrackType::Audio
                            targetTrack = tm[QStringLiteral("index")].toInt();
                            break;
                        }
                    }
                }
                if (targetTrack < 0) targetTrack = 1; // fallback to track index 1 (Video 1)
                // Default duration for images (no duration info yet)
                double clipDuration = duration > 0.0 ? duration : 5.0;
                qDebug() << "[ServiceLocator] -> addClip track:" << targetTrack
                         << "sourceType:" << sourceType << "duration:" << clipDuration;
                int clipId = m_timelineNotifier->addClip(targetTrack, sourceType, filePath,
                                            displayName, clipDuration);
                qDebug() << "[ServiceLocator] -> clip added, id:" << clipId;
            });

}

// Helper to set a context property only when the pointer is non-null.
// All notifier types are QObject subclasses but are only forward-declared
// in service_locator.h, so the compiler cannot implicitly convert to
// QObject* here.  We use reinterpret_cast because we know the layout
// (single-inheritance from QObject) and the full definitions are not
// visible in this translation unit.
static void setCtx(QQmlContext* ctx, const QString& name, void* ptr) {
    if (ptr)
        ctx->setContextProperty(name, reinterpret_cast<QObject*>(ptr));
}

void ServiceLocator::registerWithQml(QQmlApplicationEngine* engine) {
    auto* ctx = engine->rootContext();

    // Register video frame image provider and wire to timeline notifier
    auto* frameProvider = new video_editor::VideoFrameProvider();
    engine->addImageProvider(QStringLiteral("videopreview"), frameProvider);
    if (m_timelineNotifier) {
        m_timelineNotifier->setFrameProvider(frameProvider);
        qDebug() << "[ServiceLocator] videopreview image provider registered";
    }

    // Register video thumbnail provider for timeline clip thumbnails
    auto* thumbProvider = new video_editor::VideoThumbnailProvider();
    engine->addImageProvider(QStringLiteral("videothumbnail"), thumbProvider);
    qDebug() << "[ServiceLocator] videothumbnail image provider registered";

    // Register audio waveform provider for timeline clip waveforms
    auto* waveformProvider = new video_editor::AudioWaveformProvider();
    engine->addImageProvider(QStringLiteral("audiowaveform"), waveformProvider);
    qDebug() << "[ServiceLocator] audiowaveform image provider registered";

    // Core
    setCtx(ctx, QStringLiteral("router"), m_router.get());

    // Auth
    setCtx(ctx, QStringLiteral("authNotifier"), m_authNotifier.get());

    // Image Editor
    setCtx(ctx, QStringLiteral("canvasNotifier"), m_canvasNotifier.get());
    setCtx(ctx, QStringLiteral("toolNotifier"), m_toolNotifier.get());
    setCtx(ctx, QStringLiteral("undoRedoNotifier"), m_undoRedoNotifier.get());
    setCtx(ctx, QStringLiteral("filterNotifier"), m_filterNotifier.get());
    setCtx(ctx, QStringLiteral("adjustmentNotifier"), m_adjustmentNotifier.get());
    setCtx(ctx, QStringLiteral("photoImportNotifier"), m_photoImportNotifier.get());
    setCtx(ctx, QStringLiteral("textToolNotifier"), m_textToolNotifier.get());

    // Video Editor
    setCtx(ctx, QStringLiteral("timelineNotifier"), m_timelineNotifier.get());
    setCtx(ctx, QStringLiteral("mediaPoolNotifier"), m_mediaPoolNotifier.get());
    setCtx(ctx, QStringLiteral("editorLayoutNotifier"), m_editorLayoutNotifier.get());
    setCtx(ctx, QStringLiteral("exportNotifier"), m_exportNotifier.get());
    setCtx(ctx, QStringLiteral("projectListNotifier"), m_projectListNotifier.get());
    setCtx(ctx, QStringLiteral("proxyProvider"), m_proxyProvider.get());
    setCtx(ctx, QStringLiteral("thumbnailProvider"), m_thumbnailProvider.get());

    // Template Browser
    setCtx(ctx, QStringLiteral("homeNotifier"), m_homeNotifier.get());
    setCtx(ctx, QStringLiteral("templateListNotifier"), m_templateListNotifier.get());
    setCtx(ctx, QStringLiteral("templateSearchNotifier"), m_templateSearchNotifier.get());
    setCtx(ctx, QStringLiteral("downloadNotifier"), m_downloadNotifier.get());

    // GoCraft
    setCtx(ctx, QStringLiteral("goCraftNotifier"), m_goCraftNotifier.get());

    // Core engines
    setCtx(ctx, QStringLiteral("diagnosticsEngine"), m_diagnosticsEngine);
    setCtx(ctx, QStringLiteral("configEngine"), m_configEngine);
    setCtx(ctx, QStringLiteral("eventBusEngine"), m_eventBusEngine);
}

void ServiceLocator::setImageEditorEngine(rendering::ImageEditorEngine* engine) {
    m_imageEditorEngine = engine;
}

void ServiceLocator::setVideoTimelineEngine(rendering::VideoTimelineEngine* engine) {
    m_videoTimelineEngine = engine;
}

void ServiceLocator::setPlatformEngine(engines::PlatformCapabilityEngine* engine) {
    m_platformEngine = engine;
}

void ServiceLocator::setMemoryEngine(engines::MemoryManagementEngine* engine) {
    m_memoryEngine = engine;
}

void ServiceLocator::setLoggingEngine(engines::LoggingEngine* engine) {
    m_loggingEngine = engine;
}

void ServiceLocator::setConfigEngine(engines::ConfigurationEngine* engine) {
    m_configEngine = engine;
}

void ServiceLocator::setDiagnosticsEngine(engines::DiagnosticsEngine* engine) {
    m_diagnosticsEngine = engine;
}

void ServiceLocator::setEventBusEngine(engines::EventBusEngine* engine) {
    m_eventBusEngine = engine;
}

} // namespace gopost::core
