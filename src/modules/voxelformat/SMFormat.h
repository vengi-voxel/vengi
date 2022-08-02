/**
 * @file
 */

#pragma once

#include "Format.h"
#include "core/collection/Map.h"

namespace voxelformat {

/**
 * @brief StarMade (*.sment - zip archives)
 *
 * @li https://www.star-made.org
 * @li https://starmadepedia.net/wiki/Blueprint_File_Formats
 *
 * @ingroup Formats
 */
class SMFormat : public RGBAFormat {
private:
	bool readSmd3(io::SeekableReadStream &stream, SceneGraph &sceneGraph, const core::Map<int, int>& blockPal);
	bool readSegment(io::SeekableReadStream &stream, SceneGraph &sceneGraph, const core::Map<int, int>& blockPal);
public:
	bool loadGroups(const core::String &filename, io::SeekableReadStream &stream, SceneGraph &sceneGraph) override;
	bool saveGroups(const SceneGraph &sceneGraph, const core::String &filename,
					io::SeekableWriteStream &stream) override {
		return false;
	}
};

} // namespace voxelformat
