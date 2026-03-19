/**
 * @file
 */

#pragma once

#include "OpenFileMode.h"
#include "core/String.h"
#include "io/FilesystemEntry.h"
#include "io/FormatDescription.h"

#include "core/Function.h"

namespace io {
struct FormatDescription;
}

namespace video {

using FileDialogSelectionCallback = core::Function<void(const core::String &, const io::FormatDescription *desc)>;
using FileDialogOptions =
	core::Function<bool(video::OpenFileMode mode, const io::FormatDescription *desc, const io::FilesystemEntry &entry)>;

} // namespace video
