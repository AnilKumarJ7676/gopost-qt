#include "mmap_fallback_reader.h"

#include <fstream>

namespace gopost::mmap {

MmapFallbackReader::MmapFallbackReader(ILogger& logger) : logger_(logger) {}

MmapFallbackReader::~MmapFallbackReader() {
    std::lock_guard lock(mutex_);
    for (auto* buf : buffers_) delete buf;
    buffers_.clear();
}

Result<MappedRegion> MmapFallbackReader::readAsRegion(const std::string& path, uint64_t offset, size_t length) {
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file.is_open())
        return Result<MappedRegion>::failure("Fallback: cannot open file: " + path);

    auto fileSize = static_cast<uint64_t>(file.tellg());
    if (length == 0) length = static_cast<size_t>(fileSize - offset);
    if (offset + length > fileSize)
        length = static_cast<size_t>(fileSize - offset);

    file.seekg(static_cast<std::streamoff>(offset));

    auto* buf = new FallbackBuffer();
    buf->data.resize(length);
    buf->fileOffset = offset;
    file.read(reinterpret_cast<char*>(buf->data.data()), static_cast<std::streamsize>(length));
    auto bytesRead = static_cast<size_t>(file.gcount());
    buf->data.resize(bytesRead);

    {
        std::lock_guard lock(mutex_);
        buffers_.push_back(buf);
    }

    logger_.debug(QStringLiteral("Mmap"),
                  QStringLiteral("Fallback buffered read: %1 bytes from %2")
                      .arg(bytesRead).arg(QString::fromStdString(path)));

    MappedRegion region;
    region.data = buf->data.data();
    region.length = bytesRead;
    region.fileOffset = offset;
    region.isValid = true;
    return Result<MappedRegion>::success(region);
}

void MmapFallbackReader::release(const uint8_t* data) {
    std::lock_guard lock(mutex_);
    for (auto it = buffers_.begin(); it != buffers_.end(); ++it) {
        if ((*it)->data.data() == data) {
            delete *it;
            buffers_.erase(it);
            return;
        }
    }
}

} // namespace gopost::mmap
