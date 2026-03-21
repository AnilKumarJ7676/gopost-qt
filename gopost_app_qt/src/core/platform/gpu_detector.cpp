#include "core/platform/gpu_detector.h"
#include <QtGlobal>

#ifdef Q_OS_WIN
#include <qt_windows.h>
#include <dxgi1_2.h>
#include <wrl/client.h>
using Microsoft::WRL::ComPtr;
#elif defined(Q_OS_MACOS)
// Metal detection would go here via Objective-C++
#elif defined(Q_OS_LINUX)
#include <fstream>
#include <string>
#endif

namespace gopost::platform {

GpuInfo GpuDetector::detect() {
    GpuInfo info;

#ifdef Q_OS_WIN
    ComPtr<IDXGIFactory1> factory;
    HRESULT hr = CreateDXGIFactory1(__uuidof(IDXGIFactory1),
                                     reinterpret_cast<void**>(factory.GetAddressOf()));
    if (SUCCEEDED(hr) && factory) {
        // Enumerate all adapters and pick the one with the most dedicated VRAM
        // (skips software adapters, prefers discrete GPU over integrated)
        ComPtr<IDXGIAdapter1> bestAdapter;
        DXGI_ADAPTER_DESC1 bestDesc{};
        int64_t bestVram = -1;

        for (UINT i = 0; ; ++i) {
            ComPtr<IDXGIAdapter1> adapter;
            if (factory->EnumAdapters1(i, adapter.GetAddressOf()) == DXGI_ERROR_NOT_FOUND)
                break;

            DXGI_ADAPTER_DESC1 desc{};
            if (FAILED(adapter->GetDesc1(&desc)))
                continue;

            // Skip software renderer (Microsoft Basic Render Driver)
            if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
                continue;

            auto vram = static_cast<int64_t>(desc.DedicatedVideoMemory);
            if (vram > bestVram) {
                bestVram = vram;
                bestAdapter = adapter;
                bestDesc = desc;
            }
        }

        if (bestAdapter) {
            info.name = QString::fromWCharArray(bestDesc.Description);
            info.vramBytes = bestVram;

            switch (bestDesc.VendorId) {
            case 0x10DE: info.vendor = QStringLiteral("NVIDIA");  break;
            case 0x1002: info.vendor = QStringLiteral("AMD");     break;
            case 0x8086: info.vendor = QStringLiteral("Intel");   break;
            default:     info.vendor = QStringLiteral("Unknown"); break;
            }

            info.supportsDx12 = true;
        }
    }

#elif defined(Q_OS_LINUX)
    // Try reading DRM sysfs for primary GPU
    std::ifstream vendorFile("/sys/class/drm/card0/device/vendor");
    if (vendorFile.is_open()) {
        std::string vendorStr;
        std::getline(vendorFile, vendorStr);
        if (vendorStr.find("0x10de") != std::string::npos)
            info.vendor = QStringLiteral("NVIDIA");
        else if (vendorStr.find("0x1002") != std::string::npos)
            info.vendor = QStringLiteral("AMD");
        else if (vendorStr.find("0x8086") != std::string::npos)
            info.vendor = QStringLiteral("Intel");
    }
    // Try nvidia-smi for NVIDIA GPU name/VRAM
    // (deferred to avoid process spawning during detection)

#elif defined(Q_OS_MACOS)
    // Metal device detection would go here with Objective-C++
    info.supportsMetal = true;
#endif

    return info;
}

} // namespace gopost::platform
