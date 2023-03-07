/**
 * @file
 */

#pragma once

#include "Format.h"

namespace voxelformat {

/**
 * @brief CubeWorld cub format
 *
 * The first 12 bytes of the file are the width, depth and height of the volume (uint32_t little endian).
 * The remaining parts are the RGB values (3 bytes)
 *
 * @ingroup Formats
 */
class CubFormat : public RGBASinglePaletteFormat {
protected:
	bool loadGroupsRGBA(const core::String &filename, io::SeekableReadStream& stream, scenegraph::SceneGraph& sceneGraph, const voxel::Palette &palette, const LoadContext &ctx) override;
	bool saveGroups(const scenegraph::SceneGraph& sceneGraph, const core::String &filename, io::SeekableWriteStream& stream, const SaveContext &ctx) override;
public:
	size_t loadPalette(const core::String &filename, io::SeekableReadStream& stream, voxel::Palette &palette, const LoadContext &ctx) override;
};

}
