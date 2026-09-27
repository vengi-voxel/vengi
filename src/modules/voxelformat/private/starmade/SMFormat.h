/**
 * @file
 */

#pragma once

#include <glm/vec3.hpp>
#include "voxelformat/Format.h"
#include "core/collection/Map.h"

namespace voxelformat {

/**
 * @brief StarMade (*.sment - zip archives)
 *
 * @li https://www.star-made.org
 * @li https://starmadepedia.net/wiki/Blueprint_File_Formats
 * @li https://starmadepedia.net/wiki/Blueprint_File_Format_Metadata
 * @li https://starmadepedia.net/wiki/Blueprint_File_Format_Block_Data#Orientation
 *
 * Docked ATTACHED_N entities are placed from meta.smbpm tag 3 (old docks) and
 * tag 4 (rail docks). Rail TRS matches StarMade RailRelation.getBlockTransform:
 * movingAtDock * Oriencube(rail, move=1) * inverse(Oriencube(docker mirrored)).
 *
 * @ingroup Formats
 */
class SMFormat : public RGBAFormat {
private:
	// Starmade Region Data file
	// StarMade v0.199.257
	bool readSmd3(io::SeekableReadStream &stream, scenegraph::SceneGraph &sceneGraph,
				  const core::Map<int, int> &blockPal, const glm::ivec3 &position, const palette::Palette &palette,
				  int parent);
	// Starmade Region Data file
	bool readSmd2(io::SeekableReadStream &stream, scenegraph::SceneGraph &sceneGraph,
				  const core::Map<int, int> &blockPal, const glm::ivec3 &position, const palette::Palette &palette,
				  int parent);
	bool readSegment(io::SeekableReadStream &stream, scenegraph::SceneGraph &sceneGraph,
					 const core::Map<int, int> &blockPal, int headerVersion, int fileVersion,
					 const palette::Palette &palette, int parent, const core::String &name);
	bool loadGroupsRGBA(const core::String &filename, const io::ArchivePtr &archive,
						scenegraph::SceneGraph &sceneGraph, const palette::Palette &palette,
						const LoadContext &ctx) override;
	bool saveGroups(const scenegraph::SceneGraph &sceneGraph, const core::String &filename,
					const io::ArchivePtr &archive, const SaveContext &ctx) override {
		return false;
	}

public:
	size_t loadPalette(const core::String &filename, const io::ArchivePtr &archive, palette::Palette &palette,
					   const LoadContext &ctx) override;

	static const io::FormatDescription &format() {
		static io::FormatDescription f{
			"StarMade Blueprint", "", {"sment", "smd2", "smd3"}, {}, VOX_FORMAT_FLAG_PALETTE_EMBEDDED | VOX_FORMAT_FLAG_RGB};
		return f;
	}
};

} // namespace voxelformat
