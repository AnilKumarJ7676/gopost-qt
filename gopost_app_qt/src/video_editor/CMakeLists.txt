# gopost_video_editor_lib module — Phase 7: Domain + Data layer
# Part of the gopost_app_qt project.

set(VIDEO_EDITOR_SOURCES
    # Domain models (.cpp)
    domain/models/video_project.cpp
    domain/models/editor_command.cpp

    # Data — datasources
    data/datasources/video_project_local_datasource.cpp

    # Data — repositories
    data/repositories/video_project_repository_impl.cpp

    # Data — services
    data/services/ffmpeg_runner.cpp
    data/services/ffmpeg_runner_cli.cpp
    data/services/ffmpeg_runner_kit.cpp
    data/services/ffmpeg_export_service.cpp
    data/services/decoder_pool_manager.cpp
    data/services/native_thumbnail_service.cpp
    data/services/proxy_generation_service.cpp
    data/services/thumbnail_service.cpp

    # Presentation — delegates
    presentation/delegates/playback_delegate.cpp
    presentation/delegates/track_clip_delegate.cpp
    presentation/delegates/effect_color_delegate.cpp
    presentation/delegates/keyframe_audio_delegate.cpp
    presentation/delegates/advanced_edit_delegate.cpp

    # Presentation — notifiers
    presentation/notifiers/timeline_notifier.cpp
    presentation/notifiers/media_pool_notifier.cpp
    presentation/notifiers/editor_layout_notifier.cpp
    presentation/notifiers/export_notifier.cpp
    presentation/notifiers/project_list_notifier.cpp
    presentation/notifiers/proxy_provider.cpp
    presentation/notifiers/thumbnail_provider.cpp
    presentation/notifiers/video_frame_provider.cpp
    presentation/notifiers/video_thumbnail_provider.cpp
)

set(VIDEO_EDITOR_HEADERS
    # Domain models (header-only or with .cpp)
    domain/models/video_project.h
    domain/models/video_effect.h
    domain/models/video_transition.h
    domain/models/video_keyframe.h
    domain/models/media_asset.h
    domain/models/playback_state.h
    domain/models/export_config.h
    domain/models/editor_command.h
    domain/models/editor_layout_state.h
    domain/models/timeline_drag_data.h
    domain/models/video_clip_info.h

    # Domain repositories
    domain/repositories/video_project_repository.h

    # Domain services (interfaces)
    domain/services/export_service.h
    domain/services/proxy_service.h
    domain/services/thumbnail_service.h

    # Data — datasources
    data/datasources/video_project_local_datasource.h

    # Data — repositories
    data/repositories/video_project_repository_impl.h

    # Data — services
    data/services/ffmpeg_runner.h
    data/services/ffmpeg_runner_cli.h
    data/services/ffmpeg_runner_kit.h
    data/services/ffmpeg_export_service.h
    data/services/decoder_pool_manager.h
    data/services/native_thumbnail_service.h
    data/services/proxy_generation_service.h
    data/services/thumbnail_service.h

    # Presentation — delegates
    presentation/delegates/timeline_operations.h
    presentation/delegates/playback_delegate.h
    presentation/delegates/track_clip_delegate.h
    presentation/delegates/effect_color_delegate.h
    presentation/delegates/keyframe_audio_delegate.h
    presentation/delegates/advanced_edit_delegate.h

    # Presentation — notifiers
    presentation/notifiers/timeline_state.h
    presentation/notifiers/timeline_notifier.h
    presentation/notifiers/media_pool_notifier.h
    presentation/notifiers/editor_layout_notifier.h
    presentation/notifiers/export_notifier.h
    presentation/notifiers/project_list_notifier.h
    presentation/notifiers/proxy_provider.h
    presentation/notifiers/thumbnail_provider.h
    presentation/notifiers/video_frame_provider.h
    presentation/notifiers/video_thumbnail_provider.h
)

add_library(gopost_video_editor_lib STATIC
    ${VIDEO_EDITOR_SOURCES}
    ${VIDEO_EDITOR_HEADERS}
)

target_include_directories(gopost_video_editor_lib PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/..
)

target_link_libraries(gopost_video_editor_lib PUBLIC
    Qt6::Core
    Qt6::Gui
    Qt6::Quick
    Qt6::Concurrent
)

target_compile_features(gopost_video_editor_lib PUBLIC cxx_std_20)
