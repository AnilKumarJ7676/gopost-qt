# GOPOST-QT

Cross-platform media editing application with Qt6/QML frontend, C++ video/image processing engines, and Go backend.

## Project Structure

```
gopost-qt/
├── gopost_app_qt/       # Qt6/QML frontend application
│   ├── qml/             # QML UI files
│   ├── src/             # C++ application source
│   └── CMakeLists.txt
├── native/              # C++ media processing engines
│   ├── gopost_core/     # Core utilities and shared types
│   ├── gopost_image_engine/  # Image processing engine
│   ├── gopost_video_engine/  # Video decode/playback engine
│   ├── tests/           # Engine unit tests (GoogleTest)
│   └── CMakeLists.txt
├── gopost-backend/      # Go REST API backend
│   ├── cmd/             # Entry points
│   ├── internal/        # Business logic
│   ├── pkg/             # Shared packages
│   └── migrations/      # Database migrations
├── docs/                # Documentation
└── docker-compose.yml   # Docker services
```

## Prerequisites

- **Qt 6.8+** with Qt Multimedia, Qt Quick
- **CMake 3.21+**
- **C++17** compiler (MSVC 2022, GCC 12+, Clang 15+)
- **Go 1.21+**
- **FFmpeg** (runtime, for video decoding)

## Building

### Qt Frontend + C++ Engines

```bash
cd gopost_app_qt
cmake -B build -DCMAKE_PREFIX_PATH=/path/to/Qt/6.x.x/compiler
cmake --build build
```

### Go Backend

```bash
cd gopost-backend
go build -o bin/gopost-server ./cmd/server
```

### Running Tests

```bash
cd gopost_app_qt/build
ctest -C Debug --output-on-failure
```

## License

Proprietary
