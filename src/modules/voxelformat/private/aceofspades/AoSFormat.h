/**
 * @file
 */

#pragma once

#include "AoSVXLFormat.h"

namespace voxelformat {

/**
 * @brief Ace of Spades Workshop container with VXL terrain and UGC JSON metadata.
 * @ingroup Formats
 */
class AoSFormat : public AoSVXLFormat {
protected:
	bool loadGroupsRGBA(const core::String &filename, const io::ArchivePtr &archive, scenegraph::SceneGraph &sceneGraph,
						const palette::Palette &palette, const LoadContext &ctx) override;
	bool saveGroups(const scenegraph::SceneGraph &sceneGraph, const core::String &filename,
					const io::ArchivePtr &archive, const SaveContext &ctx) override;

public:
	size_t loadPalette(const core::String &filename, const io::ArchivePtr &archive, palette::Palette &palette,
					   const LoadContext &ctx) override;

	static const io::FormatDescription &format() {
		static io::FormatDescription f{"AceOfSpades Workshop", "", {"aos"}, {{'V', 'X', 'L', '\0'}, {'U', 'G', 'C', '\0'}},
			VOX_FORMAT_FLAG_PALETTE_EMBEDDED | FORMAT_FLAG_SAVE | VOX_FORMAT_FLAG_RGB};
		return f;
	}
};

} // namespace voxelformat
