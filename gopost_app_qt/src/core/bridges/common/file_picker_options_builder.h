#pragma once

#include "core/interfaces/IPlatformBridge.h"
#include <string>
#include <vector>

namespace gopost::bridges {

class FilePickerOptionsBuilder {
public:
    FilePickerOptionsBuilder& title(std::string t) { opts_.title = std::move(t); return *this; }
    FilePickerOptionsBuilder& allowMultiple(bool v) { opts_.allowMultiple = v; return *this; }
    FilePickerOptionsBuilder& extensions(std::vector<std::string> exts) { opts_.allowedExtensions = std::move(exts); return *this; }

    [[nodiscard]] core::interfaces::FilePickerOptions build() const { return opts_; }

private:
    core::interfaces::FilePickerOptions opts_;
};

} // namespace gopost::bridges
