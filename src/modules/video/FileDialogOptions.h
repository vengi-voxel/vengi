/**
 * @file
 */

#pragma once

#include "OpenFileMode.h"
#include "core/String.h"
#include "io/FormatDescription.h"

#include <functional>

namespace io {
struct FormatDescription;
}

namespace video {

using FileDialogSelectionCallback = std::function<void(const core::String &)>;
using FileDialogOptions = std::function<void(video::OpenFileMode mode, const io::FormatDescription *desc)>;

} // namespace video
