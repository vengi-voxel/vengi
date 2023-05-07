/**
 * @file
 */

#pragma once

#include "Format.h"

namespace voxelformat {

/**
 * The Minetest Schematic File Format (minetest)
 *
 * @note https://dev.minetest.net/Minetest_Schematic_File_Format
 *
 * @ingroup Formats
 */
class MTSFormat : public PaletteFormat {
public:
	bool loadGroupsPalette(const core::String &filename, io::SeekableReadStream &stream,
						   scenegraph::SceneGraph &sceneGraph, voxel::Palette &palette,
						   const LoadContext &ctx) override;
	bool saveGroups(const scenegraph::SceneGraph &sceneGraph, const core::String &filename,
					io::SeekableWriteStream &stream, const SaveContext &ctx) override;
};

} // namespace voxelformat
