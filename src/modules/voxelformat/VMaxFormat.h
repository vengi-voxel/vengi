/**
 * @file
 */

#pragma once

#include "Format.h"

namespace voxelformat {

/**
 * @brief VoxelMax (*.vmax, *.vmax.zip)
 *
 * @ingroup Formats
 */
class VMaxFormat : public PaletteFormat {
private:
	bool loadGroupsPalette(const core::String &filename, io::SeekableReadStream &stream,
						   scenegraph::SceneGraph &sceneGraph, voxel::Palette &palette,
						   const LoadContext &ctx) override;

public:
	bool saveGroups(const scenegraph::SceneGraph& sceneGraph, const core::String &filename, io::SeekableWriteStream& stream, const SaveContext &ctx) override {
		return false;
	}
	image::ImagePtr loadScreenshot(const core::String &filename, io::SeekableReadStream &stream,
								   const LoadContext &ctx) override;
	size_t loadPalette(const core::String &filename, io::SeekableReadStream &stream, voxel::Palette &palette,
					   const LoadContext &ctx) override;
};

} // namespace voxelformat
