#pragma once

#include <QObject>
#include <QQmlApplicationEngine>
#include <memory>

namespace gopost::core {
class SecureStorageService;
class AuthInterceptor;
class HttpClient;
class Router;
} // namespace gopost::core

namespace gopost::auth {
class AuthNotifier;
} // namespace gopost::auth

namespace gopost::rendering {
class ImageEditorEngine;
class VideoTimelineEngine;
} // namespace gopost::rendering

namespace gopost::platform {
class PlatformCapabilityEngine;
} // namespace gopost::platform

namespace gopost::core::engines {
class MemoryManagementEngine;
class LoggingEngine;
class ConfigurationEngine;
class DiagnosticsEngine;
class EventBusEngine;
} // namespace gopost::core::engines

namespace gopost::image_editor {
class CanvasNotifier;
class ToolNotifier;
class UndoRedoNotifier;
class FilterNotifier;
class AdjustmentNotifier;
class PhotoImportNotifier;
class TextNotifier;
} // namespace gopost::image_editor

namespace gopost::video_editor {
class TimelineNotifier;
class MediaPoolNotifier;
class EditorLayoutNotifier;
class ExportNotifier;
class ProjectListNotifier;
class ProxyProvider;
class ThumbnailProvider;
class VideoFrameProvider;
} // namespace gopost::video_editor

namespace gopost::template_browser {
class TemplateListNotifier;
class HomeNotifier;
class TemplateSearchNotifier;
class DownloadNotifier;
} // namespace gopost::template_browser

namespace gopost::go_craft {
class GoCraftNotifier;
} // namespace gopost::go_craft

namespace gopost::core {

class ServiceLocator : public QObject {
    Q_OBJECT
public:
    explicit ServiceLocator(QObject* parent = nullptr);
    ~ServiceLocator() override;

    void initialize();
    void registerWithQml(QQmlApplicationEngine* engine);

    // Core services
    SecureStorageService* secureStorage() const { return m_secureStorage.get(); }
    AuthInterceptor* authInterceptor() const { return m_authInterceptor.get(); }
    HttpClient* httpClient() const { return m_httpClient.get(); }
    Router* router() const { return m_router.get(); }

    // Rendering engines
    void setImageEditorEngine(rendering::ImageEditorEngine* engine);
    void setVideoTimelineEngine(rendering::VideoTimelineEngine* engine);
    rendering::ImageEditorEngine* imageEditorEngine() const { return m_imageEditorEngine; }
    rendering::VideoTimelineEngine* videoTimelineEngine() const { return m_videoTimelineEngine; }

    // Core engines (not owned)
    void setPlatformEngine(platform::PlatformCapabilityEngine* engine);
    void setMemoryEngine(engines::MemoryManagementEngine* engine);
    void setLoggingEngine(engines::LoggingEngine* engine);
    void setConfigEngine(engines::ConfigurationEngine* engine);
    void setDiagnosticsEngine(engines::DiagnosticsEngine* engine);
    void setEventBusEngine(engines::EventBusEngine* engine);
    platform::PlatformCapabilityEngine* platformEngine() const { return m_platformEngine; }
    engines::MemoryManagementEngine* memoryEngine() const { return m_memoryEngine; }
    engines::LoggingEngine* loggingEngine() const { return m_loggingEngine; }
    engines::ConfigurationEngine* configEngine() const { return m_configEngine; }
    engines::DiagnosticsEngine* diagnosticsEngine() const { return m_diagnosticsEngine; }
    engines::EventBusEngine* eventBusEngine() const { return m_eventBusEngine; }

    // Auth
    auth::AuthNotifier* authNotifier() const { return m_authNotifier.get(); }

    // Image Editor
    image_editor::CanvasNotifier* canvasNotifier() const { return m_canvasNotifier.get(); }
    image_editor::ToolNotifier* toolNotifier() const { return m_toolNotifier.get(); }
    image_editor::UndoRedoNotifier* undoRedoNotifier() const { return m_undoRedoNotifier.get(); }

    // Video Editor
    video_editor::TimelineNotifier* timelineNotifier() const { return m_timelineNotifier.get(); }
    video_editor::MediaPoolNotifier* mediaPoolNotifier() const { return m_mediaPoolNotifier.get(); }
    video_editor::EditorLayoutNotifier* editorLayoutNotifier() const { return m_editorLayoutNotifier.get(); }

    // Template Browser
    template_browser::HomeNotifier* homeNotifier() const { return m_homeNotifier.get(); }
    template_browser::TemplateListNotifier* templateListNotifier() const { return m_templateListNotifier.get(); }

    // GoCraft
    go_craft::GoCraftNotifier* goCraftNotifier() const { return m_goCraftNotifier.get(); }

private:
    // Core
    std::unique_ptr<SecureStorageService> m_secureStorage;
    std::unique_ptr<AuthInterceptor> m_authInterceptor;
    std::unique_ptr<HttpClient> m_httpClient;
    std::unique_ptr<Router> m_router;

    // Rendering engines (not owned)
    rendering::ImageEditorEngine* m_imageEditorEngine = nullptr;
    rendering::VideoTimelineEngine* m_videoTimelineEngine = nullptr;

    // Core engines (not owned)
    platform::PlatformCapabilityEngine* m_platformEngine = nullptr;
    engines::MemoryManagementEngine* m_memoryEngine = nullptr;
    engines::LoggingEngine* m_loggingEngine = nullptr;
    engines::ConfigurationEngine* m_configEngine = nullptr;
    engines::DiagnosticsEngine* m_diagnosticsEngine = nullptr;
    engines::EventBusEngine* m_eventBusEngine = nullptr;

    // Auth
    std::unique_ptr<auth::AuthNotifier> m_authNotifier;

    // Image Editor
    std::unique_ptr<image_editor::CanvasNotifier> m_canvasNotifier;
    std::unique_ptr<image_editor::ToolNotifier> m_toolNotifier;
    std::unique_ptr<image_editor::UndoRedoNotifier> m_undoRedoNotifier;
    std::unique_ptr<image_editor::FilterNotifier> m_filterNotifier;
    std::unique_ptr<image_editor::AdjustmentNotifier> m_adjustmentNotifier;
    std::unique_ptr<image_editor::PhotoImportNotifier> m_photoImportNotifier;
    std::unique_ptr<image_editor::TextNotifier> m_textToolNotifier;

    // Video Editor
    std::unique_ptr<video_editor::TimelineNotifier> m_timelineNotifier;
    std::unique_ptr<video_editor::MediaPoolNotifier> m_mediaPoolNotifier;
    std::unique_ptr<video_editor::EditorLayoutNotifier> m_editorLayoutNotifier;
    std::unique_ptr<video_editor::ExportNotifier> m_exportNotifier;
    std::unique_ptr<video_editor::ProjectListNotifier> m_projectListNotifier;
    std::unique_ptr<video_editor::ProxyProvider> m_proxyProvider;
    std::unique_ptr<video_editor::ThumbnailProvider> m_thumbnailProvider;

    // Template Browser
    std::unique_ptr<template_browser::HomeNotifier> m_homeNotifier;
    std::unique_ptr<template_browser::TemplateListNotifier> m_templateListNotifier;
    std::unique_ptr<template_browser::TemplateSearchNotifier> m_templateSearchNotifier;
    std::unique_ptr<template_browser::DownloadNotifier> m_downloadNotifier;

    // GoCraft
    std::unique_ptr<go_craft::GoCraftNotifier> m_goCraftNotifier;
};

} // namespace gopost::core
