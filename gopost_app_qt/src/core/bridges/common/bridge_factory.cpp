#include "bridge_factory.h"

#include <QtGlobal>

#if defined(Q_OS_WIN)
#include "core/bridges/windows/windows_bridge.h"
#elif defined(Q_OS_MACOS)
#include "core/bridges/macos/macos_bridge.h"
#elif defined(Q_OS_IOS)
#include "core/bridges/ios/ios_bridge.h"
#elif defined(Q_OS_ANDROID)
#include "core/bridges/android/android_bridge.h"
#elif defined(Q_OS_LINUX)
#include "core/bridges/linux/linux_bridge.h"
#endif

namespace gopost::bridges {

std::unique_ptr<core::interfaces::IPlatformBridge> BridgeFactory::create() {
#if defined(Q_OS_WIN)
    return std::make_unique<windows::WindowsBridge>();
#elif defined(Q_OS_MACOS)
    return std::make_unique<macos::MacosBridge>();
#elif defined(Q_OS_IOS)
    return std::make_unique<ios::IosBridge>();
#elif defined(Q_OS_ANDROID)
    return std::make_unique<android::AndroidBridge>();
#elif defined(Q_OS_LINUX)
    return std::make_unique<linux_::LinuxBridge>();
#else
    return nullptr;
#endif
}

} // namespace gopost::bridges
