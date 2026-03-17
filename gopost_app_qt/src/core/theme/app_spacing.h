#pragma once

#include <chrono>

namespace gopost::core {

namespace AppSpacing {

inline constexpr double xxs = 2.0;
inline constexpr double xs = 4.0;
inline constexpr double sm = 8.0;
inline constexpr double md = 12.0;
inline constexpr double base = 16.0;
inline constexpr double lg = 20.0;
inline constexpr double xl = 24.0;
inline constexpr double xxl = 32.0;
inline constexpr double xxxl = 48.0;
inline constexpr double xxxxl = 64.0;

} // namespace AppSpacing

namespace AppRadius {

inline constexpr double sm = 8.0;
inline constexpr double md = 12.0;
inline constexpr double lg = 16.0;
inline constexpr double xl = 24.0;
inline constexpr double full = 999.0;

} // namespace AppRadius

namespace AppDurations {

inline constexpr std::chrono::milliseconds quick{150};
inline constexpr std::chrono::milliseconds normal{300};
inline constexpr std::chrono::milliseconds slow{500};

} // namespace AppDurations

namespace AppBreakpoints {

inline constexpr double compact = 600.0;
inline constexpr double medium = 840.0;
inline constexpr double expanded = 1200.0;
inline constexpr double large = 1600.0;
inline constexpr double xlarge = 1920.0;

} // namespace AppBreakpoints

} // namespace gopost::core
