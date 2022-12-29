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
// TODO: this is no single palette format - but currently implemented as one
class SMFormat : public RGBASinglePaletteFormat {
private:
	bool readSmd3(io::SeekableReadStream &stream, SceneGraph &sceneGraph, const core::Map<int, int>& blockPal);
	bool readSegment(io::SeekableReadStream &stream, SceneGraph &sceneGraph, const core::Map<int, int>& blockPal);
	bool loadGroupsRGBA(const core::String &filename, io::SeekableReadStream &stream, SceneGraph &sceneGraph, const voxel::Palette &palette) override;
	bool saveGroups(const SceneGraph &sceneGraph, const core::String &filename,
					io::SeekableWriteStream &stream, ThumbnailCreator thumbnailCreator) override {
		return false;
	}
public:
	size_t loadPalette(const core::String &filename, io::SeekableReadStream& stream, voxel::Palette &palette) override;
};

} // namespace voxelformat
