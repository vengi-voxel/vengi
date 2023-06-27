/**
 * @file
 */

#pragma once

#include "Format.h"

namespace voxelformat {

/**
 * @brief VoxelBuilder format
 *
 * The format is ini-based and supports embedded glb files
 *
 * @ingroup Formats
 */
class VBXFormat : public RGBASinglePaletteFormat {
private:
	bool loadGLB(const core::String &voxels, scenegraph::SceneGraph &sceneGraph, const LoadContext &ctx) const;

protected:
	bool loadGroupsRGBA(const core::String &filename, io::SeekableReadStream &stream,
						scenegraph::SceneGraph &sceneGraph, const voxel::Palette &palette,
						const LoadContext &ctx) override;

public:
	size_t loadPalette(const core::String &filename, io::SeekableReadStream &stream, voxel::Palette &palette,
					   const LoadContext &ctx) override;
	bool saveGroups(const scenegraph::SceneGraph &sceneGraph, const core::String &filename,
					io::SeekableWriteStream &stream, const SaveContext &ctx) override {
		return false;
	}
};

} // namespace voxelformat
