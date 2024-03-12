/**
 * @file
 */

#pragma once

#include "voxelformat/Format.h"
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
	// Starmade Region Data file
	// StarMade v0.199.257
	bool readSmd3(io::SeekableReadStream &stream, scenegraph::SceneGraph &sceneGraph,
				  const core::Map<int, int> &blockPal, const glm::ivec3 &position, const palette::Palette &palette);
	// Starmade Region Data file
	bool readSmd2(io::SeekableReadStream &stream, scenegraph::SceneGraph &sceneGraph,
				  const core::Map<int, int> &blockPal, const glm::ivec3 &position, const palette::Palette &palette);
	bool readSegment(io::SeekableReadStream &stream, scenegraph::SceneGraph &sceneGraph,
					 const core::Map<int, int> &blockPal, int headerVersion, int fileVersion,
					 const palette::Palette &palette);
	// StarMade Blueprint Header file
	// https://starmadepedia.net/wiki/Blueprint_File_Formats#smbph.bt
	bool readHeader(io::SeekableReadStream &stream) const;
	// StarMade Blueprint Meta file
	// https://starmadepedia.net/wiki/Blueprint_File_Formats#smbpm.bt
	bool readMeta(io::SeekableReadStream &stream) const;
	// StarMade Blueprint Logic file
	// https://starmadepedia.net/wiki/Blueprint_File_Formats#smbpl.bt
	bool readLogic(io::SeekableReadStream &stream) const;
	bool loadGroupsRGBA(const core::String &filename, io::SeekableReadStream &stream,
						scenegraph::SceneGraph &sceneGraph, const palette::Palette &palette,
						const LoadContext &ctx) override;
	bool saveGroups(const scenegraph::SceneGraph &sceneGraph, const core::String &filename,
					io::SeekableWriteStream &stream, const SaveContext &ctx) override {
		return false;
	}

public:
	size_t loadPalette(const core::String &filename, io::SeekableReadStream &stream, palette::Palette &palette,
					   const LoadContext &ctx) override;
};

} // namespace voxelformat
