/**
 * @file
 */

#pragma once

#include "OpenFileMode.h"
#include "core/String.h"
#include "io/FilesystemEntry.h"
#include "io/FormatDescription.h"

#include <functional>

namespace io {
struct FormatDescription;
}

namespace video {

using FileDialogSelectionCallback = std::function<void(const core::String &, const io::FormatDescription *desc)>;
using FileDialogOptions =
	std::function<bool(video::OpenFileMode mode, const io::FormatDescription *desc, const io::FilesystemEntry &entry)>;

} // namespace video
