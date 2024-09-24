/**
 * @file
 */

#include "FilesystemEntry.h"
#include "core/Log.h"
#include "core/Path.h"
#include "core/StringUtil.h"
#include "system/System.h"

namespace io {

FilesystemEntry createFilesystemEntry(const core::Path &filename) {
	FilesystemEntry entry;
	entry.name = core::string::extractFilenameWithExtension(filename.str());
	entry.fullPath = filename;
	if (!fs_stat(entry.fullPath, entry)) {
		Log::trace("Could not stat '%s'", entry.fullPath.c_str());
	}
	return entry;
}

} // namespace io
