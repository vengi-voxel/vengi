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
		uint32_t magic = 0;
		if (stream->readUInt32(magic) != 0) {
			Log::error("Failed to read magic");
			return false;
		}
		if (magic != FourCC('B', 'E', 'N', 'V')) {
			uint8_t buf[4];
			FourCCRev(buf, magic);
			Log::error("Invalid magic found - no binary benv file: %c%c%c%c", buf[0], buf[1], buf[2], buf[3]);
			return false;
		}

		uint32_t totalLength;
		if (stream->readUInt32(totalLength) != 0) {
			Log::error("Failed to read total length");
			return false;
		}

		core::String version;
		if (!stream->readPascalStringUInt8(version)) {
			Log::error("Failed to read version");
			return false;
		}
		scenegraph::SceneGraphNode &root = sceneGraph.node(0);
		root.setProperty("version", version.c_str());

		io::ZipReadStream zipStream(*stream, stream->remaining());
		io::BufferedReadWriteStream wrapper(zipStream);
		return benv::loadBinary(sceneGraph, palette, wrapper);
	}
	return false;
}

size_t BenVoxelFormat::loadPalette(const core::String &filename, const io::ArchivePtr &archive,
								   palette::Palette &palette, const LoadContext &ctx) {
	return 0;
}

bool BenVoxelFormat::saveGroups(const scenegraph::SceneGraph &sceneGraph, const core::String &filename,
								const io::ArchivePtr &archive, const SaveContext &ctx) {
	return false;
}

} // namespace voxelformat
