#include "core/platform/storage_detector.h"
#include <QtGlobal>

#include <QCoreApplication>

#ifdef Q_OS_WIN
#include <qt_windows.h>
#include <winioctl.h>
#elif defined(Q_OS_LINUX)
#include <fstream>
#include <string>
#include <cstring>
#include <sys/stat.h>
#endif

namespace gopost::platform {

StorageType StorageDetector::detect(const QString& path) {
    Q_UNUSED(path)

#ifdef Q_OS_WIN
    // Resolve the drive letter from the app path
    QString appPath = path.isEmpty() ? QCoreApplication::applicationDirPath() : path;
    if (appPath.size() < 2) return StorageType::Unknown;

    QChar driveLetter = appPath.at(0).toUpper();
    if (!driveLetter.isLetter()) return StorageType::Unknown;

    // Open the drive volume
    QString volumePath = QStringLiteral("\\\\.\\%1:").arg(driveLetter);
    HANDLE hDevice = CreateFileW(
        reinterpret_cast<LPCWSTR>(volumePath.utf16()),
        0, // No access needed for query
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        nullptr,
        OPEN_EXISTING,
        0,
        nullptr);

    if (hDevice == INVALID_HANDLE_VALUE)
        return StorageType::Unknown;

    // Query seek penalty (SSD vs HDD)
    bool isSsd = false;
    {
        STORAGE_PROPERTY_QUERY query{};
        query.PropertyId = StorageDeviceSeekPenaltyProperty;
        query.QueryType  = PropertyStandardQuery;

        DEVICE_SEEK_PENALTY_DESCRIPTOR desc{};
        DWORD returned = 0;
        if (DeviceIoControl(hDevice, IOCTL_STORAGE_QUERY_PROPERTY,
                            &query, sizeof(query),
                            &desc, sizeof(desc),
                            &returned, nullptr)) {
            isSsd = !desc.IncursSeekPenalty;
        }
    }

    // Check bus type for NVMe
    bool isNvme = false;
    {
        STORAGE_PROPERTY_QUERY query{};
        query.PropertyId = StorageAdapterProperty;
        query.QueryType  = PropertyStandardQuery;

        BYTE buf[1024]{};
        DWORD returned = 0;
        if (DeviceIoControl(hDevice, IOCTL_STORAGE_QUERY_PROPERTY,
                            &query, sizeof(query),
                            buf, sizeof(buf),
                            &returned, nullptr)) {
            auto* adapterDesc = reinterpret_cast<STORAGE_ADAPTER_DESCRIPTOR*>(buf);
            if (adapterDesc->BusType == BusTypeNvme)
                isNvme = true;
        }
    }

    CloseHandle(hDevice);

    if (isNvme)  return StorageType::NVMe;
    if (isSsd)   return StorageType::SSD;
    return StorageType::HDD;

#elif defined(Q_OS_LINUX)
    // Resolve the block device for the root filesystem
    struct stat st;
    if (stat("/", &st) != 0) return StorageType::Unknown;

    unsigned int major = (st.st_dev >> 8) & 0xFF;
    unsigned int minor = st.st_dev & 0xFF;

    // Try /sys/dev/block/<major>:<minor>/queue/rotational
    std::string sysPath = "/sys/dev/block/"
                          + std::to_string(major) + ":" + std::to_string(minor);

    // Follow the symlink to get the actual device
    char resolvedBuf[PATH_MAX];
    ssize_t len = readlink(sysPath.c_str(), resolvedBuf, sizeof(resolvedBuf) - 1);
    if (len > 0) {
        resolvedBuf[len] = '\0';
        std::string resolved(resolvedBuf);
        // Check if the device path contains "nvme"
        if (resolved.find("nvme") != std::string::npos)
            return StorageType::NVMe;
    }

    // Check rotational flag on parent block device
    std::string rotPath = sysPath + "/queue/rotational";
    std::ifstream rotFile(rotPath);
    if (!rotFile.is_open()) {
        // Try parent device (e.g., sda from sda1)
        rotPath = sysPath + "/../queue/rotational";
        rotFile.open(rotPath);
    }
    if (rotFile.is_open()) {
        int rotational = -1;
        rotFile >> rotational;
        if (rotational == 0) return StorageType::SSD;
        if (rotational == 1) return StorageType::HDD;
    }
    return StorageType::Unknown;

#elif defined(Q_OS_MACOS)
    // macOS: would use diskutil info / or IOKit
    // Assume SSD for modern Macs
    return StorageType::SSD;
#else
    return StorageType::Unknown;
#endif
}

} // namespace gopost::platform
