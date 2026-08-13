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
	ctx.setProgressText("Minecraft world");
	ctx.setProgress(0.0f);
	core::ScopedPtr<io::SeekableReadStream> stream(archive->readStream(filename));
	if (!stream) {
		Log::error("Could not load file %s", filename.c_str());
		return false;
	}
	io::ArchivePtr zipArchive = io::openZipArchive(stream);
	DatFormat datFormat;
	if (!datFormat.load("level.dat", zipArchive, sceneGraph, ctx)) {
		Log::error("Failed to load level.dat from %s", filename.c_str());
		return false;
	}
	ctx.setProgress(1.0f);
	return true;
}

} // namespace voxelformat
