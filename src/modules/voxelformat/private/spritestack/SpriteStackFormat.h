/**
 * @file
 */

#pragma once

#include "io/Archive.h"
#include "io/FormatDescription.h"
#include "io/Stream.h"
#include "voxelformat/Format.h"

namespace voxelformat {

/**
 * @brief SpriteStack (*.zip)
 *
 * @ingroup Formats
 */
class SpriteStackFormat : public PaletteFormat {
private:
	bool loadGroupsPalette(const core::String &filename, const io::ArchivePtr &archive,
						   scenegraph::SceneGraph &sceneGraph, palette::Palette &palette,
						   const LoadContext &ctx) override;

public:
	bool saveGroups(const scenegraph::SceneGraph &sceneGraph, const core::String &filename,
					const io::ArchivePtr &archive, const SaveContext &ctx) override;

	size_t loadPalette(const core::String &filename, const io::ArchivePtr &archive, palette::Palette &palette,
					   const LoadContext &ctx) override;

	bool singleVolume() const override {
		return true;
	}

	static const io::FormatDescription &format() {
		static io::FormatDescription f{"SpriteStack", {"zip"}, {}, VOX_FORMAT_FLAG_PALETTE_EMBEDDED | FORMAT_FLAG_SAVE};
		return f;
	}
};

} // namespace voxelformat
