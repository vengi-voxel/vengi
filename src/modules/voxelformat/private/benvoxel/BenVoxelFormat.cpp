/**
 * @file
 */

#include "BenVoxelFormat.h"
#include "BenBinary.h"
#include "BenJson.h"
#include "core/FourCC.h"
#include "core/Log.h"
#include "core/ScopedPtr.h"
#include "core/StringUtil.h"
#include "io/Archive.h"
#include "io/BufferedReadWriteStream.h"
#include "io/ZipReadStream.h"
#include "scenegraph/SceneGraph.h"

namespace voxelformat {

int BenVoxelFormat::emptyPaletteIndex() const {
	return 0;
}

bool BenVoxelFormat::loadGroupsPalette(const core::String &filename, const io::ArchivePtr &archive,
									   scenegraph::SceneGraph &sceneGraph, palette::Palette &palette,
									   const LoadContext &ctx) {
	core::ScopedPtr<io::SeekableReadStream> stream(archive->readStream(filename));
	if (!stream) {
		Log::error("Failed to open stream for file: %s", filename.c_str());
		return false;
	}

	if (core::string::endsWith(filename, "ben.json")) {
		core::String jsonStr;
		if (!stream->readString(stream->size(), jsonStr)) {
			Log::error("Failed to read json file");
			return false;
		}
		return benv::loadJson(sceneGraph, palette, jsonStr);
	} else if (core::string::endsWith(filename, "ben")) {
		return benv::loadBinary(sceneGraph, palette, *stream);
	}
	return false;
}

bool BenVoxelFormat::saveGroups(const scenegraph::SceneGraph &sceneGraph, const core::String &filename,
								const io::ArchivePtr &archive, const SaveContext &ctx) {
	core::ScopedPtr<io::SeekableWriteStream> stream(archive->writeStream(filename));
	if (!stream) {
		Log::error("Failed to open stream for file: %s", filename.c_str());
		return false;
	}
	if (core::string::endsWith(filename, "ben.json")) {
		if (!benv::saveJson(sceneGraph, *stream)) {
			Log::error("Failed to save json");
			return false;
		}
	} else {
		if (!benv::saveBinary(sceneGraph, *stream)) {
			Log::error("Failed to save binary");
			return false;
		}
	}

	return true;
}

} // namespace voxelformat
