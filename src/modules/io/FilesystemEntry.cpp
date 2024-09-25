/**
 * @file
 */

#include "FilesystemEntry.h"
#include "core/Log.h"
#include "core/StringUtil.h"
#include "system/System.h"

namespace io {

FilesystemEntry createFilesystemEntry(const core::String &filename) {
	FilesystemEntry entry;
	entry.name = core::string::extractFilenameWithExtension(filename);
	entry.fullPath = filename;
	if (!fs_stat(filename.c_str(), entry)) {
		Log::trace("Could not stat '%s'", filename.c_str());
	}
	return entry;
}

} // namespace io
