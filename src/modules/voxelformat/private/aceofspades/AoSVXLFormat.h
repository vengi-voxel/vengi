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
	bool loadGroupsRGBA(const core::String &filename, const io::ArchivePtr &archive,
						scenegraph::SceneGraph &sceneGraph, const palette::Palette &palette,
						const LoadContext &ctx) override;
	bool saveGroups(const scenegraph::SceneGraph &sceneGraph, const core::String &filename,
					const io::ArchivePtr &archive, const SaveContext &ctx) override;
	glm::ivec3 maxSize() const override;

public:
	size_t loadPalette(const core::String &filename, const io::ArchivePtr &archive, palette::Palette &palette,
					   const LoadContext &ctx) override;
};

} // namespace voxelformat
