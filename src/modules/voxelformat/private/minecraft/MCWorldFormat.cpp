/**
 * @file
 */

#include "MCWorldFormat.h"
#include "core/Log.h"
#include "core/ScopedPtr.h"
#include "io/ZipArchive.h"
#include "voxelformat/private/minecraft/DatFormat.h"

namespace voxelformat {

bool MCWorldFormat::loadGroupsPalette(const core::String &filename, const io::ArchivePtr &archive,
									  scenegraph::SceneGraph &sceneGraph, palette::Palette &palette,
									  const LoadContext &ctx) {
	core::ScopedPtr<io::SeekableReadStream> stream(archive->readStream(filename));
	if (!stream) {
		Log::error("Could not load file %s", filename.c_str());
		return false;
	}
	io::ArchivePtr zipArchive = io::openZipArchive(stream);
	DatFormat datFormat;
	if (!datFormat.load("level.dat", zipArchive, sceneGraph, ctx)) {
		Log::error("Failed to load level.dat or regions from '%s'", filename.c_str());
		return false;
	}
	return true;
}

} // namespace voxelformat
