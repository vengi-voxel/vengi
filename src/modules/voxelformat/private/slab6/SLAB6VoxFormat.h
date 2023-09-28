/**
 * @file
 */

#pragma once

#include "voxelformat/Format.h"

namespace voxelformat {

/**
 * @brief SLAB6 vox format
 *
 * @ingroup Formats
 */
class SLAB6VoxFormat : public PaletteFormat {
protected:
	bool readColor(io::SeekableReadStream &stream, core::RGBA &color) const;
	bool writeColor(io::SeekableWriteStream &stream, core::RGBA color) const;

	bool loadGroupsPalette(const core::String &filename, io::SeekableReadStream &stream,
						   scenegraph::SceneGraph &sceneGraph, voxel::Palette &palette,
						   const LoadContext &ctx) override;
	bool saveGroups(const scenegraph::SceneGraph &sceneGraph, const core::String &filename,
					io::SeekableWriteStream &stream, const SaveContext &ctx) override;

	bool singleVolume() const override {
		return true;
	}
};

} // namespace voxelformat
