/**
 * @file
 */

#pragma once

#include "Format.h"

namespace voxel {

/**
 * @brief CubeWorld cub format
 *
 * The first 12 bytes of the file are the width, depth and height of the volume (uint32_t little endian).
 * The remaining parts are the RGB values (3 bytes)
 */
class CubFormat : public Format {
public:
	size_t loadPalette(const core::String &filename, io::SeekableReadStream& stream, Palette &palette) override;
	bool loadGroups(const core::String &filename, io::SeekableReadStream& stream, SceneGraph& sceneGraph) override;
	bool saveGroups(const SceneGraph& sceneGraph, const core::String &filename, io::SeekableWriteStream& stream) override;
};

}
