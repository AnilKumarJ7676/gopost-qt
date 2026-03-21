#include "core/platform/hardware_decode_detector.h"
#include <QtGlobal>

#ifdef Q_OS_WIN
#include <qt_windows.h>
#include <initguid.h>
#include <mfapi.h>
#include <mfidl.h>

// Build MF video-format GUIDs from FourCC codes
// Format: {FourCC-0000-0010-8000-00AA00389B71}
#define MK_MF_GUID(a,b,c,d) \
    { ((DWORD)(BYTE)(a)) | ((DWORD)(BYTE)(b) << 8) | ((DWORD)(BYTE)(c) << 16) | ((DWORD)(BYTE)(d) << 24), \
      0x0000, 0x0010, { 0x80, 0x00, 0x00, 0xAA, 0x00, 0x38, 0x9B, 0x71 } }

static const GUID kH264  = MK_MF_GUID('H','2','6','4');
static const GUID kHEVC  = MK_MF_GUID('H','E','V','C');
static const GUID kVP90  = MK_MF_GUID('V','P','9','0');
static const GUID kAV01  = MK_MF_GUID('A','V','0','1');
static const GUID kAPCH  = MK_MF_GUID('a','p','c','h');
#endif

namespace gopost::platform {

#ifdef Q_OS_WIN
namespace {

struct ComInit {
    ComInit()  { CoInitializeEx(nullptr, COINIT_MULTITHREADED); }
    ~ComInit() { CoUninitialize(); }
};

bool hasDecoder(const GUID& subtype) {
    MFT_REGISTER_TYPE_INFO inputType;
    inputType.guidMajorType = MFMediaType_Video;
    inputType.guidSubtype   = subtype;

    // Try HW-only first (includes async MFTs used by NVDEC/AMD AMF/Intel QSV)
    IMFActivate** ppActivate = nullptr;
    UINT32 count = 0;
    HRESULT hr = MFTEnumEx(
        MFT_CATEGORY_VIDEO_DECODER,
        MFT_ENUM_FLAG_HARDWARE | MFT_ENUM_FLAG_ASYNCMFT | MFT_ENUM_FLAG_SORTANDFILTER,
        &inputType, nullptr, &ppActivate, &count);

    if (SUCCEEDED(hr) && ppActivate) {
        for (UINT32 i = 0; i < count; ++i) ppActivate[i]->Release();
        CoTaskMemFree(ppActivate);
    }
    if (SUCCEEDED(hr) && count > 0) return true;

    // Fallback: probe all decoders (HW decoders on some drivers register as
    // async-only MFTs that MFT_ENUM_FLAG_HARDWARE alone doesn't match)
    ppActivate = nullptr; count = 0;
    hr = MFTEnumEx(
        MFT_CATEGORY_VIDEO_DECODER,
        MFT_ENUM_FLAG_ALL,
        &inputType, nullptr, &ppActivate, &count);

    if (SUCCEEDED(hr) && ppActivate) {
        for (UINT32 i = 0; i < count; ++i) ppActivate[i]->Release();
        CoTaskMemFree(ppActivate);
    }
    return SUCCEEDED(hr) && count > 0;
}

} // anonymous namespace
#endif

HwDecodeSupport HardwareDecodeDetector::detect() {
    HwDecodeSupport hw;

#ifdef Q_OS_WIN
    ComInit com;
    if (FAILED(MFStartup(MF_VERSION, MFSTARTUP_LITE)))
        return hw;

    hw.h264   = hasDecoder(kH264);
    hw.h265   = hasDecoder(kHEVC);
    hw.vp9    = hasDecoder(kVP90);
    hw.av1    = hasDecoder(kAV01);
    hw.prores = hasDecoder(kAPCH);

    MFShutdown();

#elif defined(Q_OS_MACOS) || defined(Q_OS_IOS)
    hw.h264 = true;
    hw.h265 = true;
#endif

    return hw;
}

} // namespace gopost::platform
