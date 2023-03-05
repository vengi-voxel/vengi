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
	bool loadGroupsRGBA(const core::String &filename, io::SeekableReadStream &stream, SceneGraph &sceneGraph, const voxel::Palette &palette, const LoadContext &ctx) override;
	bool saveGroups(const SceneGraph &sceneGraph, const core::String &filename,
					io::SeekableWriteStream &stream, const SaveContext &ctx) override {
		return false;
	}
public:
	size_t loadPalette(const core::String &filename, io::SeekableReadStream& stream, voxel::Palette &palette, const LoadContext &ctx) override;
};

} // namespace voxelformat
