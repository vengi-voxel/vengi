/**
 * @file
 */

#pragma once

#include "voxelformat/Format.h"

namespace voxelformat {

/**
 * @brief AceOfSpades VXL format
 *
 * https://silverspaceship.com/aosmap/
 *
 * @ingroup Formats
 */
class AoSVXLFormat : public RGBASinglePaletteFormat {
protected:
	void loadMetadataTxt(scenegraph::SceneGraphNode &node, const core::String &filename,
						 const io::ArchivePtr &archive) const;
	void loadMetadataTxt(scenegraph::SceneGraphNode &node, const core::String &filename,
						 io::SeekableReadStream *stream) const;
	bool loadGroupsRGBA(const core::String &filename, const io::ArchivePtr &archive, scenegraph::SceneGraph &sceneGraph,
						const palette::Palette &palette, const LoadContext &ctx) override;
	bool saveGroups(const scenegraph::SceneGraph &sceneGraph, const core::String &filename,
					const io::ArchivePtr &archive, const SaveContext &ctx) override;
	bool saveMetadataTxt(const scenegraph::SceneGraph &sceneGraph, const core::String &filename,
						 const io::ArchivePtr &archive) const;
	glm::ivec3 maxSize() const override;

public:
	bool singleVolume() const override;
	size_t loadPalette(const core::String &filename, const io::ArchivePtr &archive, palette::Palette &palette,
					   const LoadContext &ctx) override;

	static const io::FormatDescription &format() {
		static io::FormatDescription f{
			"AceOfSpades", {"vxl"}, {}, VOX_FORMAT_FLAG_PALETTE_EMBEDDED | FORMAT_FLAG_SAVE | VOX_FORMAT_FLAG_RGB};
		return f;
	}
};

} // namespace voxelformat
