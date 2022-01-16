/**
 * @file
 */

#pragma once

#include "Format.h"

namespace voxel {

/**
 * @brief MagicaVoxel vox format load and save functions
 *
 * z is pointing upwards
 *
 * https://github.com/ephtracy/voxel-model.git
 * https://github.com/ephtracy/voxel-model/blob/master/MagicaVoxel-file-format-vox-extension.txt
 * https://ephtracy.github.io/
 */
class VoxFormat : public Format {
private:
	int findClosestPaletteIndex();
public:
	VoxFormat();
	size_t loadPalette(const core::String &filename, io::SeekableReadStream& stream, core::Array<uint32_t, 256> &palette) override;
	bool loadGroups(const core::String &filename, io::SeekableReadStream& stream, SceneGraph& sceneGraph) override;
	bool saveGroups(const SceneGraph& sceneGraph, const core::String &filename, io::SeekableWriteStream& stream) override;
};

}
