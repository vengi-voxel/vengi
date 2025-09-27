/**
 * @file
 */

#pragma once

#include "voxelformat/Format.h"

namespace voxelformat {

/**
 * @brief Gox txt file format
 *
 * Simple txt format with # as comments and one line per voxel:
 *
 * @code
 * # Goxel x.y.z
 * # One line per voxel
 * # X Y Z RRGGBB
 * X Y Z RRGGBB
 * X Y Z RRGGBB
 * @endcode
 *
 * @ingroup Formats
 */
class GoxTxtFormat : public RGBAFormat {
protected:
	bool loadGroupsRGBA(const core::String &filename, const io::ArchivePtr &archive, scenegraph::SceneGraph &sceneGraph,
						const palette::Palette &palette, const LoadContext &ctx) override;
	bool saveGroups(const scenegraph::SceneGraph &sceneGraph, const core::String &filename,
					const io::ArchivePtr &archive, const SaveContext &ctx) override;

public:
	bool singleVolume() const override {
		return true;
	}

	size_t loadPalette(const core::String &filename, const io::ArchivePtr &archive, palette::Palette &palette,
					   const LoadContext &ctx) override;

	static const io::FormatDescription &format() {
		static io::FormatDescription f{"Goxel txt", {"txt"}, {"# Go"}, FORMAT_FLAG_SAVE};
		return f;
	}
};

} // namespace voxelformat
