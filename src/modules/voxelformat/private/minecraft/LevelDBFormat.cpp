/**
 * @file
 */

#include "LevelDBFormat.h"
#include "core/Log.h"
#include "core/ScopedPtr.h"

namespace voxelformat {

bool LevelDBFormat::loadGroupsPalette(const core::String &filename, const io::ArchivePtr &archive,
									  scenegraph::SceneGraph &sceneGraph, palette::Palette &palette,
									  const LoadContext &ctx) {
	core::ScopedPtr<io::SeekableReadStream> stream(archive->readStream(filename));
	if (!stream) {
		Log::error("Could not load file %s", filename.c_str());
		return false;
	}
	// TODO: VOXELFORMAT: support bedrock leveldb files (leveldb with zlib compression)
	// https://minecraft.fandom.com/wiki/Bedrock_Edition_level_format#Mojang_variant_LevelDB_format
	// https://github.com/JJJollyjim/leveldb-mcpe
	return false;
}

} // namespace voxelformat
