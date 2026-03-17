#pragma once

#include <QPalette>

namespace gopost::core {

class AppTheme {
public:
    AppTheme() = delete;

    static QPalette light();
    static QPalette dark();
};

} // namespace gopost::core
