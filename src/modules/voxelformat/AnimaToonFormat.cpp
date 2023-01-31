/**
 * @file
 */

#include "AnimaToonFormat.h"
#include "core/Log.h"

namespace voxelformat {

bool AnimaToonFormat::loadGroupsRGBA(const core::String &filename, io::SeekableReadStream &stream,
									 SceneGraph &sceneGraph, const voxel::Palette &palette) {
	const int64_t size = stream.size();
	core::String str(size, ' ');
	if (!stream.readString((int)str.size(), str.c_str())) {
		Log::error("Failed to read string from stream");
		return false;
	}

	printf("%s\n", str.c_str());

	return false;
}

} // namespace voxelformat
