/**
 * @file
 */

#pragma once

#include "voxelformat/Format.h"

namespace voxelformat {

/**
 * @brief Starmade Template format
 *
 * @ingroup Formats
 */
class SMTPLFormat : public PaletteFormat {
protected:
	bool loadGroupsPalette(const core::String &filename, io::SeekableReadStream &stream,
						   scenegraph::SceneGraph &sceneGraph, palette::Palette &palette,
						   const LoadContext &ctx) override;
	bool saveGroups(const scenegraph::SceneGraph &sceneGraph, const core::String &filename,
					io::SeekableWriteStream &stream, const SaveContext &ctx) override;
public:
	static void loadPalette(palette::Palette &palette);
	bool singleVolume() const override {
		return true;
	}

	size_t loadPalette(const core::String &filename, io::SeekableReadStream &stream, palette::Palette &palette,
					   const LoadContext &ctx) override;
};

} // namespace voxelformat
